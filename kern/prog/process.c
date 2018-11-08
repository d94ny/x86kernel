/**
 * @file process.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Process management related functions
 */

#include <x86/cr.h>
#include <simics.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <page.h>
#include <x86/page.h>
#include <thrlist.h>
#include <errors.h>
#include <lock.h>

/** A mutex to make the next_pid() function atomic */
static mutex_t pid_lock;
static unsigned int pid = PROCESS_INITIAL_PID;


/**
 * @brief Create a new process
 *
 * This is the birth point of every process. We allocate his control 
 * block, initialize his kernel page tables and the memory region 
 * tracking. 
 *
 * @return the process control block of the new process, NULL on error
 */
process_t *create_process(void) {

	// Allocate the process control block
	process_t *process = calloc(1, sizeof(process_t));
	if (process == NULL) return NULL;	

	// Create the page directory and page tables for the kernel
	process->cr3 = init_paging();
	if (process->cr3 == NULL) {
		free(process);	
		return NULL;
	}

	// Create a new process id
	process->pid = next_pid();
	process->exit_status = -1;
	process->state = RUNNING;
	
	// Memory region tracking
	process->memregions = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (process->memregions == NULL) {
		destroy_paging(process);
		free(process);
		return NULL;
	}
	memset(process->memregions, 0, PAGE_SIZE);
	process->next_memregion_idx = 0;

	// Leave family NULL
	process->parent = NULL;
	process->youngest_child = NULL;
	process->older_sibling = NULL;
	process->younger_sibling = NULL;
	process->original_tid = -1;
	process->youngest_thread = NULL;
	process->children = 0;
	process->threads = 0;

	// Create the waiting list
	process->waiting = calloc(1, sizeof(thrlist_t));
	if (process->waiting == NULL) {
		sfree(process->memregions, PAGE_SIZE);	
		destroy_paging(process);
		free(process);
		return NULL;
	}

	return process;

}

/**
 * @brief Creates the first process (or "god" process).
 *
 * This function differs from the regular create_process because we need 
 * to allocate a user stack for the first process, which we don't when we 
 * call copy_process.
 *
 * @return the god process
 */
process_t *create_god_process() {

	// Initialize the pid_mutex
	mutex_init(&pid_lock);

	// Create a normal process (without user stack)
	process_t *god = create_process();
	if (god == NULL) return NULL;

	// Setup the page directory so that we can create the user stack
	set_cr3((uint32_t) god->cr3);
	set_cr0(get_cr0() | CR0_PG);

	// Create new user stack
	int err = create_page(PAGE_ADDR(0xfffffffc), MEM_TYPE_STACK, NULL);
	if (err < 0) {
		free(god);
		return NULL;
	}

	return god;
}


/**
 * @brief Creates a copy of the given process
 *
 * This is used by fork, and will produce a new task receiving an exact,  
 * coherent copy of all memory regions of the invoking task.
 *
 * @param parent the invoking process
 * @return the copied process
 */
process_t *copy_process(process_t *parent) {

	if (parent == NULL) return NULL;

	// Create a new process
	process_t *process = create_process();
	if (process == NULL) return NULL;
	
	// Copy the memory region
	int err = copy_paging(parent, process);		
	if (err < 0) {
		destroy_process(process);	
		return NULL;
	}

	// Update Family relations
	process->parent = parent;

	if (parent->youngest_child != NULL) {
		parent->youngest_child->younger_sibling = process;
		process->older_sibling = parent->youngest_child;
	}
	parent->youngest_child = process;
	parent->children += 1;

	// Now we should be ready
	return process;
}

/**
 * @brief Returns an exited child
 *
 * @param parent the parent process looking for an exited child
 * @return an exited child or NULL if none exist
 */
process_t *exited_child(process_t *parent) {

	if (parent == NULL || parent->children == 0) return NULL;
	
	// Find a child which EXITED already
	process_t *child = parent->youngest_child;
	while (child != NULL && child->state != EXITED) {
		child = child->older_sibling;
	}

	return child;
}

/**
 * @brief Returns the next available process id
 * @return a process id
 */
unsigned int next_pid(void) {

	mutex_lock(&pid_lock);
	unsigned int new_pid = pid++;
	mutex_unlock(&pid_lock);

	return new_pid;
}

/**
 * @brief Exit the given process
 *
 * Note that we can only vanish a thread who has no more threads. Thus all
 * threads called "vanish_thread()" prior to this call.
 *
 * If we still have process children, we update their family relations 
 * such that init becomes their parent and can collect their exit status 
 * via wait().
 *
 * @param process the process to vanish
 * @return 0 on sucess, a negative error code otherwise.
 */
int vanish_process(process_t *process) {

	if (process == NULL) return ERR_ARG_NULL;

	// Make sure we have no more active threads
	if (process->threads > 0) return ERR_ACTIVE_THREADS; 	

	// Make children not destroyed yet available to "init"
	if (process->children > 0) {

		process_t *init_task = init()->process;
		if (init_task == NULL)
			kernel_panic("Init is nowhere to be found.");

		// the parent now was more children
		init_task->children += process->children;

		// Update the "parent" link of all processes
		process_t *last = NULL;
		process_t *current = process->youngest_child;
		while (current != NULL) {
			current->parent = init_task;
			last = current;
			current = current->older_sibling;
		}

		// Now update so that the last is the younger sibling of the
		// youngest from init
		last->older_sibling = init_task->youngest_child;
		if (init_task->youngest_child)
			init_task->youngest_child->younger_sibling = last;

		init_task->youngest_child = process->youngest_child;

	}

	// Mark this process as having exited, for waiting threads to 
	// collect.
	process->state = EXITED;
	return 0;
}


/**
 * @brief Destroys a process and all resources for it
 *
 * This function is called by anoher process which called wait and is now 
 * cleaning up the remains (like the kernel stacks and control blocks).
 *
 * @param process the process to destroy
 * @return 0 on sucess, a negative error code otherwise
 */
int destroy_process(process_t *process) {

	if (process == NULL) return ERR_ARG_NULL;

	// Make sure the given thread has called vanish_process()
	if (process->state != EXITED) return ERR_PROCESS_NOT_EXITED;
	process->state = BURIED;

	// Destroy all the threads
	while (process->youngest_thread != NULL) {
		destroy_thread(process->youngest_thread);
	}

	// Update family of parent process
	process_t *older = process->older_sibling;
	process_t *younger = process->younger_sibling;
	if (older) older->younger_sibling = younger;
	if (younger) younger->older_sibling = older;
	else if (process->parent)
		process->parent->youngest_child = older;

	if (process->parent) process->parent->children -= 1;
	
	// Destroy all the pages
	int derr = destroy_paging(process);
	if (derr < 0) return derr;

	sfree(process->memregions, PAGE_SIZE);
	thrlist_destroy(process->waiting);
	free(process);
	return 0;
}

