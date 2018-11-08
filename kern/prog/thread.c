/**
 * @file thread.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Thread management related functions
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include <x86/cr.h>	
#include <x86/asm.h>	
#include <x86/page.h>

#include <errors.h>
#include <types.h>
#include <drivers.h>
#include <thrlist.h>
#include <thrhash.h>
#include <thread.h>

/**
 * These double linked lists are used to categorize different threads 
 * given their states. Each of them has unique properties :
 * 
 * 'running' represents the list of all runnable threads, which could get 
 * the CPU.
 * A very important property is that the head of the running list is
 * ALWAYS the thread currently running.*/ 
static thrlist_t running;

/* 'sleeping' represents all the threads which made a call to 'sleep()'.
 * It is maintained ordered increasingly according to thread->wake to make
 * the awakening of threads during the timer handler as fast as possible.
 */
static thrlist_t sleeping;

/* A read/write lock for the hash table, which contains all threads */
static rwlock_t hash_lock;

/* A simple mutex to make the next_tid() function atomic */
static mutex_t tid_lock;
static unsigned int tid = THREAD_INITIAL_TID;

/* Pointers to the idle and init thread */
static thread_t *init_thread = NULL;
static thread_t *idle_thread = NULL;

/* Lock used to make the malloc functions thread safe */
mutex_t mem_lock;


/**
 * @brief Initializes the thread management library
 *
 * This will initialize all the data structures required by the kernel to 
 * keep track of all the threads and their statuses.
 */
void thread_init() {

	thrlist_init(&running);
	thrlist_init(&sleeping);	
	rwlock_init(&hash_lock);
	mutex_init(&tid_lock);
	mutex_init(&mem_lock);
}


/**
 * @brief Unsets the current state of a thread
 *
 * This is used to remove a thread from the data structures assossiated 
 * with his current state, such as the 'running' list.
 *
 * Since all states except 'THR_BLOCKED' use lists, we only need to remove
 * the thread from the list he currently belongs to.
 *
 * @param thread the thread which will lose his current state
 * @return 0 on sucess, a negative error code otherwise
 */
int unset_state(thread_t *thread) {

	// Verify that the thread is not null
	if (thread == NULL) return ERR_ARG_NULL;

	// Remove the thread from its list
	int err = thrlist_remove(thread);
	
	if (err >= 0) thread->state = THR_ZOMBIE;	
	return err;
}	

/**
 * @brief Sets the given thread to be running
 * 
 * This simply adds the thread at the beginning of the running list.
 * 
 * This function also makes the important set_esp0 call, so that our mode
 * switch operates correctly in this new thread. 
 *
 * This function is called from the context_switch procedure during the
 * transition period.
 *
 * @param thread the thread to transfer execution to
 * @return 0 on success, a negative error code otherwise
 * -----
 * last edited 2015/09/26
 */
int set_running(thread_t *thread) {

	if (thread == NULL) return ERR_ARG_NULL;

	int err = unset_state(thread);
	if (err < 0) return err;

	err = thrlist_add_head(thread, &running);
	if (err < 0) return err;

	thread->state = THR_RUNNING;

	set_esp0(thread->esp0);
	set_cr3((uint32_t)(thread->process->cr3));

	return 0;
}

/**
 * @brief Sets the given thread to be runnable
 *
 * We add the given thread to the end of the running list, not to the 
 * beginning as does the set_running function.
 *
 * @param thread the thread we should make runnable
 * @return 0 on success, a negative error code otherwise
 */
int set_runnable(thread_t *thread) {

	if (thread == NULL) return ERR_ARG_NULL;

	int err = unset_state(thread);
	if (err < 0) return err;

	thread->state = THR_RUNNING;

	err = thrlist_add_tail(thread, &running);
	return err;
}

/**
 * @brief Sets the given thread to be blocked
 *
 * A blocked thread only belongs to the hash table containing all threads,
 * but to no list like running, sleeping or waiting.
 *
 * @param thread the thread to be blocked
 * @return 0 on success, a negative error code otherwise
 */
int set_blocked(thread_t *thread) {

	if (thread == NULL) return ERR_ARG_NULL;

	int err = unset_state(thread);
	if (err < 0) return err;

	thread->state = THR_BLOCKED;
	return 0;
}

/**
 * @brief Adds a thread to the list of sleeping threads in a sorted way
 *
 * The 'sleeping' list is sorted increasingly in the "vake" value which 
 * specifies the time in number of ticks of their awakening.
 *
 * @param thread the thread to put to sleep
 * @return 0 on sucess, a negative error code otherwise
 */
int set_sleeping(thread_t *thread, unsigned int sleep) {

	if (thread == NULL) return ERR_ARG_NULL;

	int err = unset_state(thread);
	if (err < 0) return err;
	
	// Set the wake up time for the thread
	unsigned int time = get_time();
	thread->wake = time + sleep;

	thread->state = THR_SLEEPING;

	err = thrlist_add_sorted(thread, &sleeping);

	return err;
}

/**
 * @brief Adds a thread to the list of waiting threads
 *
 * @param thread the thread which waits
 * @retrun 0 on success, a negative error code otherwise
 */
int set_waiting(thread_t *thread) {

	if (thread == NULL) return ERR_ARG_NULL;

	process_t *process = thread->process;
	if (process == NULL) return ERR_NO_PROCESS;

	int err = unset_state(thread);
	if (err < 0) return err;

	thread->state = THR_WAITING;

	err = thrlist_add_tail(thread, process->waiting);

	return err;
}


/**
 * @breif Returns the head of the running list.
 * @return the tcb of the thread at the head of running.
 */
thread_t *get_running() {
	return running.head;
}

/**
 * @brief Returns the thread currently running.
 *
 * The 'running' list is defined to containg the currently running thread 
 * as its head. Since we want to obtain our own thread control block 
 * though this function we call a kernel panic is the head is NULL.
 *
 * @return our thread control block
 */
thread_t *get_self() {
	
	thread_t *self = get_running();
	if (self == NULL) kernel_panic("Running list incoherance");
	return self;
}

/**
 * @brief Finds a thread given his tid
 *
 * We look for the thread of given tid in the hash table of all threads. 
 * This will give us O(1) access, very useful for yield.
 *
 * We protect the hash table with read/write locks.
 *
 * @param tid the ID of the thread to find
 * @return the thread we are looking for
 */
thread_t *get_thread(unsigned int tid) {

	rwlock_lock(&hash_lock, RWLOCK_READ);
	thread_t *thread = thrhash_find(tid);
	rwlock_unlock(&hash_lock);

	return thread;
}


/**
 * @brief Returns the head of the sleeping list
 * @return the thread with closest waking time
 */
thread_t *get_sleeping(void) {	
	return sleeping.head;
}

/**
 * @brief returns the head of the waiting list
 *
 * Waiting threads are saved in the 'waiting' list of their process.
 *
 * @param parent the parent process of the invoking process
 */
thread_t *get_waiting(process_t *parent) {

	if (parent == NULL) return NULL;
	else return parent->waiting->head;
}


/**
 * @brief Returns the number of thread ready to be run*
 * @return the number of threads ready to resume execution
 */
unsigned int num_runnable() {
	return running.size;
}

/**
 * @brief Create a new thread
 *
 * This is the birth place of any thread. We create a thread control block
 * and add it the hash table.
 *
 * On error we propagate the error to the caller by returning NULL.
 *
 * @param process the process the thread belongs to
 * @return the thread control block of the new thread
 */
thread_t *create_thread(process_t *parent) {

	thread_t *thread = calloc(1, sizeof(thread_t));
	if (thread == NULL) return NULL;

	thread->tid = next_tid();
	thread->state = THR_ZOMBIE;
	thread->esp = -1;

	// Create a new kernel stack
	uint32_t kstack = (uint32_t)smemalign(PAGE_SIZE,
			THREAD_KERNEL_SIZE*PAGE_SIZE);

	if (kstack == 0x0) {
		free(thread);
		return NULL;
	}

	// Set the esp to point to the highest address
	thread->esp0 = kstack + THREAD_KERNEL_SIZE*PAGE_SIZE
		- sizeof(uint32_t);

	thread->esp3 = 0xfffffffc;
	thread->process = parent;
	parent->threads += 1;
	
	// Is this the original thread of the process?
	if (parent->original_tid == -1)
		parent->original_tid = thread->tid;

	thread->younger_sibling = NULL;
	thread->older_sibling = NULL;

	// Now update the thread family relations
	if (parent->youngest_thread) {
		thread->older_sibling = parent->youngest_thread;
		parent->youngest_thread->younger_sibling = thread;
	}
	parent->youngest_thread = thread; 

	// Intialize list and swexn values
	thread->list = NULL;
	thread->next = NULL;
	thread->prev = NULL;
	thread->swexn_eip = 0x0;
	thread->swexn_esp = 0x0;
	thread->swexn_arg = NULL;

	// Create thread locks
	thread->thread_lock = calloc(1, sizeof(mutex_t));
	if (thread->thread_lock == NULL) {
		sfree((void *)kstack, PAGE_SIZE);		
		free(thread);
		return NULL;
	}
	mutex_init(thread->thread_lock);
	thread->acquired_lock = NULL;

	// Add thread to hash table
	rwlock_lock(&hash_lock, RWLOCK_WRITE);
	thrhash_add(thread);
	rwlock_unlock(&hash_lock);

	return thread;

}

/**
 * @brief Copies a given thread
 *
 * @param process the process we belong to
 * @param target the thread to clone
 * @param handler wether the swexn handlers are transfered
 * @return the new thread
 */
thread_t *copy_thread(process_t *process, thread_t *target, boolean_t handler) {

	// Allocate the memory for the control block (on the thread space)
	thread_t *thread = create_thread(process);
	if (thread == NULL) return NULL;
	
	thread->esp = target->esp;
	thread->esp3 = target->esp3;
	thread->process = process;

	// Copy software exception handler
	if (handler) {
		thread->swexn_eip = target->swexn_eip;
		thread->swexn_esp = target->swexn_esp;
		thread->swexn_arg = target->swexn_arg;
	}	

	return thread;
} 

/**
 * @brief Vanishes the calling thread, making him unexecutable.
 *
 *
 * We can't free the kernel stack since that's where we currently are. 
 * Thus we need to keep the family relations in the tcb. Another task 
 * calling wait() will then clean up.
 *
 * Since we are not supposed to run anymore we are removed from all the 
 * lists, remaining only in the tasks internal thread family.
 *
 * We also release all acquired locks.
 *
 * @param thread the thread to be destroyed
 * @return 0 on succes, otherwise a negative error code
 */
int vanish_thread(void) {

	thread_t *self = get_self();

	// Release all mutexes 
	while (self->acquired_lock) {
		mutex_unlock(self->acquired_lock);
	}

	// Remove the thread from any list
	int err = unset_state(self);
	if (err < 0) return err;

	// Get the process and update its thread count
	process_t *task = self->process;
	if (task == NULL) return ERR_NO_PROCESS;
	task->threads -= 1;

	return 0;
}

/**
 * @brief Destroyes a thread completely
 *
 * This function takes care of cleaning up after a vanished thread.
 * We free the kernel stack of the given thread, remove him from the list 
 * of threads of the process and remove him from the hash table.
 *
 * This functions is thus called by another thread.
 * 
 * @param thread the thread to destroy
 * @return 0 on success, a negative error code otherwise
 */
int destroy_thread(thread_t *thread) {

	// Verify the thread pointer
	if (thread == NULL) return ERR_ARG_NULL;

	// Get the process of the thread
	process_t *task = thread->process;

	// Update the family relations
	thread_t *older = thread->older_sibling;
	thread_t *younger = thread->younger_sibling;
	if (older) older->younger_sibling = thread->younger_sibling;
	if (younger) younger->older_sibling = thread->older_sibling;
	else if (task) task->youngest_thread = thread->older_sibling;

	thread->older_sibling = NULL;
	thread->younger_sibling = NULL;

	// remove the kernel stack here	
	void *stack = (void*)(thread->esp0
			- THREAD_KERNEL_SIZE*PAGE_SIZE + sizeof(uint32_t));
	sfree(stack, THREAD_KERNEL_SIZE*PAGE_SIZE);

	// remove the thread frm the hash table and free remaining resources
	rwlock_lock(&hash_lock, RWLOCK_WRITE);
	thrhash_remove(thread);
	rwlock_unlock(&hash_lock);
	free(thread);

	return 0;
}


/**
 * @breif returns the next thread ID atomically
 * @return next available thread ID
 */
unsigned int next_tid(void) {

	mutex_lock(&tid_lock);
	unsigned int new_tid = tid++;
	mutex_unlock(&tid_lock);

	return new_tid;
}

/**
 * @brief Sets up the idle thread
 *
 * The calling function is responsible for handeling an error. But it will
 * most likely panic, since idle is essential to the kernel.
 *
 * We assume that the idle thread is the first child thread of the current
 * process.
 *
 * @param idle the idle thread
 * @return 0 on success, a negative error code otherwise
 */
int set_idle(thread_t *idle) {
	
	if (idle == NULL) return ERR_ARG_NULL;
	idle_thread = idle;

	// Remove all family relations
	process_t *process = idle->process;
	process->original_tid = -1;

	if (process->parent) {
		process->parent->children -= 1;
		process->parent->youngest_child = NULL;
		process->parent->original_tid = -1;
		process->parent = NULL;
	}

	return 0;
}

/**
 * @brief Sets up the init thread
 *
 * Like for set_idle, the caller has to respond accordingly to an error.
 *
 * @param init the init thread
 * @return 0 on sucess, a negative error code otherwise.
 */
int set_init(thread_t *init) {
	if (init == NULL) return ERR_ARG_NULL;
	init_thread = init;
	return 0;
}

/**
 * @return returns the idle thread
 */
thread_t *idle(void) {
	return idle_thread;
}

/**
 * @return returns the init thread
 */
thread_t *init(void) {
	return init_thread;
}

/**
 * @breif Verifies if the given thread is the idle thread.
 *
 * @param thread a thread suspicious of being idle
 * @return Returns wether the given thread is the idle thread
 */
int is_idle(thread_t *thread) {
	return (thread == idle_thread);
}

