/**
 * @file p2thread.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief The thread management functions of the thread library
 * 
 * @section Functionality
 * This file is responsible for all thread management functions such as 
 * creating, exiting and joining.
 *
 * @section Architecture
 * Threads are defined by what we call "threads descriptors". These data 
 * structures containg essential information about any living or vanished 
 * thread, such as their kernel and user ID, whether they have called exit yet, 
 * their exit status, the base of their stack region, a condition variable to 
 * signal their death to any joined thread, and a mutex to ensure only one 
 * thread, may it be another one or itself, reads or writes to the desciptor.  
 * It is also used by any thread which calls thr_create to ensure that the child 
 * thread will not run before we have succesfully created the new thread 
 * descriptor.
 *
 * @bugs None
 */

#include <syscall.h>		/* gettid, new_pages */
#include <stdio.h>
#include <simics.h>
#include <assert.h>
#include <stdlib.h>

#include <malloc.h>
#include <spawn_thread.h>
#include <mutex.h>
#include <cond.h>
#include <thread.h>
#include <p2thrlist.h>		/* thr_get, thr_insert */
#include <p2thread.h>
#include "exception.h"


/**
 * threads is a double linked list of thread descriptors.
 * @see p2thrlist.h
 *
 * Each thread should have a single thread descriptor at all time, allocated
 * on the heap. The descriptor contains various information such as kernel ID,
 * user thread ID, the thread mutex, state flags and its frame base.*
 *
 * All "alive" threads should be in the following list. When a thread requires 
 * information about itself or another thread it should look in that list. It 
 * can do so using either its kernel ID or its user ID.
 *
 * The list will be initialized during thr_init().
 * Since that thread list is a shared resource we use a mutex "threads_mutex" 
 * for any interaction with the list.
 */
static thrlist_t *threads = NULL;
static mutex_t *threads_mutex = NULL;

/**
 * To ensure thread safety for the malloc family function (malloc, calloc, free 
 * and realloc) we use a simply boolean on which we spin before calling one of 
 * these function.
 *
 * Using a mutex would not work for the following reason : mutexes have 
 * waiting_lists and when a new node is added to the list, we need to malloc is 
 * first. So malloc can not be protected by a mechanism which uses malloc 
 * itself.
 */
boolean_t mem_lock = FALSE;

/**
 * A user thread ID counter. It get's incremented everytime a thread is created.  
 * Very primitive. The initial value is MIN_THREAD_ID defined in the header 
 * file. We assume that we will never create enough threads to overflow that 
 * integer.
 */
static int nextID = MIN_THREAD_ID;

/**
 * nextbase represents the base of the next available stack region. When a 
 * thread is created is saves the current value of nextbase so that it can 
 * remove its stack page(s) once it terminates (actually joined threads will do 
 * that clean-up).
 *
 * We use an unsigned int instead of a (void *) to make the decrementation to 
 * the next available stack space easier.
 */
static unsigned int nextbase;


/**
 * The amount of pages required by each thread. This is simple computed as
 * = 1 + (stack space)/(PAGE_SIZE).
 *
 * This makes it easier to compute a page aligned base for the new_pages() 
 * function later.
 */
static unsigned int stackpages;



/**
 * @brief Initializes the thread library
 *
 * This function call, which should be made exactly once before any further
 * use of the thread library will initialize the "threads" list, as well as
 * setting the next available stack slot and the stackpages value.
 *
 * To get the next available stack slot we start at the highest address which is 
 * page aligned, and then try allocating pages going downwards until we don't 
 * receive a negative error message. Don't forget to remove that page 
 * afterwards.
 *
 * @param size the size of the thread stack frames
 * @return 0 on sucess, otherwise a negative error code
 */
int thr_init(unsigned int size) {
	/**
	 * We revoke the autostack library and register our own exception 
	 * handler
	 */
	void *exception_stack = calloc(PAGE_SIZE, sizeof(char));
	swexn(exception_stack, multi_swexn_handler, exception_stack, NULL);

	/**
	 * Compute the amount of pages required for the stack space of threads.  
	 * As descibed above, this makes the computation of page-aligned 
	 * addresses passed to new_pages easier.
	 */
	if (size%PAGE_SIZE == 0) stackpages = size/PAGE_SIZE;
	else stackpages = (size/PAGE_SIZE + 1);


	// Initialize the "threads" list.
	threads = thrlist_init();

	// Initialize the corresponding mutex
	threads_mutex = malloc(sizeof(mutex_t));
	mutex_init(threads_mutex);

	/**
	 * Create a thread descriptor for the current/main thread.
	 */
	thrdesc_t *desc = thr_newdesc(gettid(), 0);
	thrlist_addFirst(threads, desc);

	/** Now we need to find the first available stack space slot. We 
	 * start from the very top of the address space WHICH IS PAGE ALIGNED
	 * Starting from 0xffffffff would result in -7 error codes until we reach
	 * the bottom of the address space ... which we want to avoid.
	 */
	int pages = 0xffffffff/PAGE_SIZE;
	unsigned int top = pages * PAGE_SIZE;
	int err = -1;

	while (err != 0) { 

		// Update top and allocate a new page
		top -= PAGE_SIZE;
		err = new_pages((void *)top, PAGE_SIZE);

		/**
		 * If we provided an unaligned-page address, which will 
		 * propagate until the end of the address space, we need to exit 
		 * the loop.
		 * (Error code -7 is an unaligned-page address)
		 */		
		if (err == -7) return -1;
	}

	/**
	 * We have successfully allocated a page, yet we don't need it now, so 
	 * update our nextbase value, and deallocate the page.
	 */
	nextbase = top - PAGE_SIZE;
	err = remove_pages((void *)top);

	/**
	 * If removing that page failed, something really wrong happened.
	 * We return -1 as well.
	 */
	if (err != 0) return -1;
	return 0;

}



/**
 * @brief Creates a thread to run func with the given argument
 *
 * This creates a new thread using the spawn_thread method written in assembly 
 * which sets up the stack for the child thread correctly. That function has the 
 * convienient property of never returning to the child, thus the only thread 
 * inside the thr_create will be the parent.
 * @see spawn_thread.S
 *
 * Each step of the thread creation is explained in greated detail bellow.
 * 
 * @param func the function we want the child thread to execute
 * @param arg the argument for the function func
 * @return 0 on success, otherwise a negative error code
 */
int thr_create(void *(*func)(void *), void *arg ) {

	// Get our thread descriptor to obtain our mutex
	thrdesc_t *self = thr_getdesc();

	/**
	 * We lock ourselves so that the child, which will try to aquire the 
	 * same mutex for a very short time before calling func, runs after we 
	 * are done setting everything up in the parent/
	 */ 
	mutex_lock(self->mutex);

	/**
	 * Compute the next available stack space slot. This will also be the 
	 * argument passed to the new_pages() function as it allocates memory 
	 * growing upwards, while we count 'nextbase' downwards
	 *
	 * We need to remember the current value of nextbase before updating it 
	 * as the child will have that value as his stack base pointer in his 
	 * descriptor.
	 */
	void *base = (void *)nextbase;
	nextbase = nextbase - stackpages*PAGE_SIZE;

	/**
	 * Now we can allocate the new stack for the child
	 * The nbase needs to be page alligned.
	 */	
	int err = new_pages((void *)nextbase, stackpages*PAGE_SIZE);

	/**
	 * check if the stack was sucessfully allocated. If not we simply 
	 * propagate the error message we received from new_pages();
	 */
	if (err) return err;

	/**
	 * Now we "spawn" the new child thread. We do so in Assembly to ensure 
	 * that the stack pointer (esp) points to the base of the child stack 
	 * when the child thread starts running.
	 *
	 * Note that spawn_thread never returns to the child thread, since 
	 * thr_exit() will be called before the ret instructions.
	 *
	 * Thus after the spawn_thread we are always in the parent thread, which 
	 * is a convienient property of spawn_thread. Additionnaly the stack 
	 * pointer of the parent stack should be unchanged.
	 *
	 * The return value will be child thread id, which we then need to save 
	 * in a new thread descriptor.
	 *
	 * Since the child might want to know its user ID it is possible that 
	 * the parent did not have time to create the child thread descriptor.	
	 * To make sure the descriptor is present in the "threads" list when the 
	 * child gains the CPU we pass the parent mutex along which the child 
	 * will need to aquire before continuing its excecution.  
	 */
	int childid = thr_spawn(base, func, arg, self->mutex);

	/**
	 * Now we need to create a new thread descriptor.
	 * Note that the child id returned by spawn_thread is the kernel ID of 
	 * the child thread. We will generate our own thread ID.
	 */
	thrdesc_t* desc = thr_newdesc(childid, nextbase);
	assert(desc != NULL);

	/**
	 * Save the thread descriptor in the list "threads".
	 * We add it to the head since the child is likely to look for it's own 
	 * thread descriptor soon.
	 */
	thrlist_addFirst(threads, desc);
	mutex_unlock(threads_mutex);

	/**
	 * Now the parent should be done.
	 * Return the user (not kernel) ID of the child thread.
	 */ 
	mutex_unlock(self->mutex);
	return desc->tid;
}


/**
 * @brief Creates a new thread descriptor
 *
 * We generate a new user thread ID and then allocate a new thread descriptor
 * on the heap, as there should be only one copy per thread getting passed
 * arround.
 *
 * @param childid the kernel ID of the new thread
 * @param base the base of the stack region for the child thread
 * @return a pointer to the thread descriptor
 */
thrdesc_t *thr_newdesc(int childid, unsigned int base) {

	// Allocate the descriptor on the heap
	thrdesc_t *desc = (thrdesc_t *)malloc(sizeof(thrdesc_t));
	assert(desc != NULL);

	/**
	 * Generate a new user thread ID and set the attributes and flags to the 
	 * default values. We also initialize the mutex and death condition 
	 * variable.
	 */
	desc->kid = childid;
	desc->sbase = base;
	desc->tid = nextID++;
	desc->zombie = 0;
	desc->statusp = NULL;

	desc->mutex = calloc(1, sizeof(mutex_t));
	assert(desc->mutex != NULL);
	assert(mutex_init(desc->mutex) == 0);

	desc->death = calloc(1, sizeof(cond_t));
	assert(desc->death != NULL);
	assert(cond_init(desc->death) == 0);;
	desc->joined = 0;

	return desc;

}



/**
 * @brief Calls a given function with some argument
 *
 * launch_thread is called by spawn_thread by the child thread. It will simply 
 * call the function which the child thread has to execute before terminating 
 * the thread. Thus launch_thread never returns.
 *
 * Note that a thread which returns from its body function func instead of 
 * calling thr_exit() will do so when it returns to launch_thread.
 *
 * @param func the function to be executed by the child thread
 * @param arg the argument for the function
 */
void thr_launch(void *(*func)(void *), void *arg, mutex_t *parent_mutex) {

	// Make sure the parent is done and our thread desciptor exists
	mutex_lock(parent_mutex);
	mutex_unlock(parent_mutex);

	/**
	 * Register our own exception handler. We need our own exception stack 
	 * in
	 * case another thread faults simultaneously
	 */
	void *exception_stack = calloc(PAGE_SIZE, sizeof(char));
	swexn(exception_stack, multi_swexn_handler, exception_stack, NULL);

	/**
	 * Make sur the function is not NULL. The argument may be NULL.
	 * If that is the case we need to exit the thread.
	 */
	void *status = 0;
	if (func != NULL) status = func(arg);
	thr_exit(status);

}


/**
 * @brief Returns the user thread ID of the calling thread
 *
 * We use the system call gettid to get the kernel ID of the calling thread
 * and then find the corresponding thread descriptor.
 *
 * @return the user thread ID
 */
int thr_getid(void) {
	thrdesc_t *desc = thr_getdesc();
	return desc->tid;
}

/**
 * @brief Finds the thread descriptor of the currently running tread
 *
 * @return the thread descriptor of the calling thread
 */
thrdesc_t *thr_getdesc(void) {

	// Lock the "threads" mutex before interacting with the list
	mutex_lock(threads_mutex);
	thrdesc_t *desc = thrlist_findkern(threads, gettid());
	assert(desc != NULL);
	mutex_unlock(threads_mutex);
	return desc;
}


/**
 * @brief Exist the current thread with the given status 
 *
 * To signal our death to any thread which may have joined itself on us we use 
 * the condition variable "death".
 *
 * @param status a pointer-sized opaque data type representing the exit status
 */
void thr_exit(void *status) {

	//  Get our own description	
	thrdesc_t *self = thr_getdesc();

	// We lock the thread so no one can join at the same time
	mutex_lock(self->mutex);

	// Set out status and state to Zombie
	self->statusp = &status;
	self->zombie = 1;

	// Signal our death
	mutex_unlock(self->mutex);
	cond_signal(self->death);

	vanish();

}

/**
 * @brief Suspends the calling thread until the given thread terminates
 *
 * @param tid the user ID of the thread to be joined
 * @param statusp placeholder for the exit status of the joined thread
 * @return 0 on success, a negative error code otherwise
 */
int thr_join(int tid, void **statusp) {


	// First we check if the thread tid exists
	mutex_lock(threads_mutex);
	thrdesc_t *target = thrlist_finduser(threads, tid);
	mutex_unlock(threads_mutex);

	// If the entry does not exists return an error code
	if (target == NULL) return -1;

	// Lock the thread
	mutex_lock(target->mutex);

	// Check if the thread has called exit yet
	while (!target->zombie) {

		// Wait for the death
		target->joined = 1;
		cond_wait(target->death, target->mutex);
	 } 

	/**
	 * At thus point the thread we joined on has called exit and 
	 * given an exit status. If statusp is not null, we place that 
	 * status there.
	 */
	if (statusp != NULL) *statusp = *(target->statusp);

	mutex_unlock(target->mutex);
	return 0;

}


/**
 * @brief Yields to the given thread
 *
 * We simply translate the given user ID into the corresponding kernel ID using 
 * the threads list and then call the system call yield().
 *
 * @param tid the user ID of the thread to yield to
 * @return 0 on sucess, otherwise a negative error code
 */
int thr_yield(int tid) {

	if (tid == -1) {
		yield(-1);
		return 0;
	} else {
		mutex_lock(threads_mutex);
		int kid = (thrlist_finduser(threads, tid))->tid;
		mutex_unlock(threads_mutex);
		return yield(kid);
	}
	return -1;
}

