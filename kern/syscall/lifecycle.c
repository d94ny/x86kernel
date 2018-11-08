/**
 * @file lifecycle.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Implementation of thread life cycle realted system calls.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <inc/syscall.h>
#include <exec2obj.h>
#include <elf_410.h>

#include <simics.h>
#include <x86/page.h>
#include <x86/cr.h>

#include <launch.h>
#include <page.h>
#include <drivers.h>
#include <thread.h>
#include <process.h>
#include <types.h>
#include <errors.h>
#include <syshelper.h>
#include <context.h>

/**
 * @brief Loads a user program
 *
 * We read the program data from the "RAM disk" image included in the 
 * kernel executable binary and load the data into the tasks address 
 * space.
 *
 * We use the provided execname to determine if the idle or init thread is 
 * being loaded and update the corresponding pointers required by our 
 * kernel in the future at the end of the function.
 *
 * To read the program data we use the exec2obj utility which will also be 
 * in charge of validating the header and providing a simple ELF header 
 * with information about each section.
 *
 * We first initialize the stack area of the new program with the provided 
 * arguments and reset the previous paging which we inherited during the 
 * fork call.
 *
 * Then we iteratate over the simple ELF header and create pages for each 
 * section and copy the data from the exec2obj file into that page.
 *
 * @param args the program arguments and the filename of the program
 * @return does not return on sucess, a negative error code otherwise
 */
int _exec(void **args) {
	if (!check_array(args, 2)) return ERR_INVALID_ARG;

	// Argument 1
	char *execname = (char*) args[0];
	if (!check_string(execname)) return ERR_INVALID_ARG;
	boolean_t is_idle = (strcmp("idle", execname) == 0);
	boolean_t is_init = (strcmp("init", execname) == 0);

	// Argument 2
	char **argvec = (char**) args[1];
	if (!check_string_array(argvec)) return ERR_INVALID_ARG;

	// Verify that the ELF header is valid
	if (elf_check_header(execname) != ELF_SUCCESS)
		return ERR_ELF_INVALID;	

	// Create the Simple ELF header
	simple_elf_t *hdr = calloc(1, sizeof(simple_elf_t));
	if (hdr == NULL) return ERR_CALLOC_FAIL;

	// Load the ELF header
	if (elf_load_helper(hdr, execname) != ELF_SUCCESS) {
		free(hdr);
		return ERR_ELF_LOAD_FAIL;
	}

	// Get the entry of the file in the exec2obj TOC
	const exec2obj_userapp_TOC_entry *target = exec2obj_entry(execname);;

	// fsource is the source of the file content
	unsigned int fs = (unsigned int)target->execbytes;

	// We store the arguments to survive the paging reset
	int num_args = string_array_length(argvec);

	// Save the execname and the arguments in the kernel
	char **karg = calloc(num_args, sizeof(char*));
	int total_arg_length = 0;
	int i;
	for (i = 0; i < num_args; ++i) {
		char *arg = argvec[num_args - i - 1];
		int arglen = strlen(arg) + 1;

		karg[i] = calloc(arglen, sizeof(char));
		if (karg[i] == NULL) {

			// Abort everyhting
			for(; i > 0; --i) free(karg[i]);
			free(karg);
			free(hdr);
			return ERR_CREATE_USERSTACK_FAIL;
		}

		strncpy(karg[i], arg, arglen);
		total_arg_length += arglen + 1;
	}

	// Reset the paging, only kernel pages are mapped now
	reset_paging();

	// Put the argument strings above the stack
	int num_arg_pages = total_arg_length / PAGE_SIZE 
		+ (total_arg_length % PAGE_SIZE == 0) ? 0:1;
	vaddr_t va = PAGE_ADDR(-1);
	for (i = 0; i < num_arg_pages; ++i) {
		if (i > 0) va -= PAGE_SIZE;
		int err = create_page(va, MEM_TYPE_RODATA, NULL);
		if (err) return ERR_SAVE_ARGS_FAIL;
	}
	vaddr_t bottom_argzone = va;

	// Create user stack
	uint32_t *esp3 = ((uint32_t *) va) - 1;
	int err = create_page(PAGE_ADDR((uint32_t) esp3), MEM_TYPE_STACK, NULL);
	if (err) {	
		for (i = 0; i < num_args; ++i) free(karg[i]);
		free(karg);
		free(hdr);
		return ERR_CREATE_USERSTACK_FAIL;
	}

	// Push the argument string pointers
	int len;
	va = bottom_argzone;
	for (i = 0; i < num_args; va += len, ++i) {
		len = strlen(karg[i]) + 1;
		strncpy((char*)va, karg[i], len);
		*(esp3 - i) = va;
		free(karg[i]);
	}
	free(karg);

	// Push the arguments
	uint32_t *argbase = esp3 - num_args - 4;
	*(argbase + 4) = PAGE_ADDR((uint32_t) esp3);
	*(argbase + 3) = (uint32_t) esp3;
	*(argbase + 2) = (uint32_t) (argbase + 5);
	*(argbase + 1) = num_args;
	get_running()->esp3 = (uint32_t) (argbase);

	/**
	 * To avoid repeated code we iterate over a smaller structure which 
	 * contains the length, offset and start, in the same order as the 
	 * original structure of the simple elf header.
	 *
	 * Now we iterate over txt, data, rodata and bss segments
	 */
	simple_elf_seg_t *segs = (simple_elf_seg_t *)&(hdr->e_txtoff);	
	mem_type_t type[4] = {MEM_TYPE_TEXT,
			      MEM_TYPE_DATA,
			      MEM_TYPE_RODATA,
			      MEM_TYPE_BSS};



	for (i = 0; i < 4; i++) {
		// Create pages for the segment and load it
		unsigned int copied = 0;
		unsigned int length = 0;
		if (type[i] == MEM_TYPE_BSS) length = hdr->e_bsslen;
		else length = segs[i].len;
		
		while (copied < length) {

			// Compute the next virtual address
			vaddr_t va;
			if (type[i] == MEM_TYPE_BSS)
				va = hdr->e_bssstart + copied;
			else va = segs[i].start + copied;
	
			/**
			 * Find how much space we will have in the page,
			 * since va might point inside a page.
			 */
			unsigned int space = PAGE_ADDR(va) + PAGE_SIZE-va;

			boolean_t bss_page_created = FALSE;
			
			// Create the page for that virtual address
			if (type[i] == MEM_TYPE_TEXT || type[i] == MEM_TYPE_DATA
					|| (type[i] == MEM_TYPE_RODATA
						&& (PAGE_ADDR(va) < PAGE_ADDR(hdr->e_txtstart)
						|| PAGE_ADDR(va) > PAGE_ADDR(hdr->e_txtstart + hdr->e_txtlen)))
					|| (type[i] == MEM_TYPE_BSS
						&& (PAGE_ADDR(va) < PAGE_ADDR(hdr->e_datstart)
						|| PAGE_ADDR(va) > PAGE_ADDR(hdr->e_datstart + hdr->e_datlen)))) {
				// Create the page if it has not been created yet
				int err = create_page(PAGE_ADDR(va), type[i], NULL);
				if (err) return ERR_SEGMENT_PAGE_FAIL;
				
				if (type[i] == MEM_TYPE_BSS) {
					bss_page_created = TRUE;
				}
			}

			// Special case if are handeling bss
			if (type[i] == MEM_TYPE_BSS) {
				if (!bss_page_created) {
					memset((void *) va, 0, (space > hdr->e_bsslen) ? hdr->e_bsslen : space);
				}
				copied += space;
				continue;
			}

			// We are handeling data, txt, or rodata
			// Determine from where in the file we need to read
			unsigned int rs = fs + segs[i].off + copied;
	
			// Now copy the bytes from the file into that page
			(void)memcpy((void *)va, (void *)rs, space);

			// We copied "space" additional bytes
			copied += space;

		}
	}

	/**
	 * Finally we check if the program we are now running is idle. In 
	 * that case we remove him from the "runnable" list, to which we 
	 * were added during _fork
	 */
	if(is_idle && set_idle(get_self()) < 0) kernel_panic("No idle thread");	
	if(is_init && set_init(get_self()) < 0) kernel_panic("No init thread");
	launch(hdr->e_entry, get_self()->esp3);

	return 0;
}

/**
 * @brief Creates a new task with the user context from the running parent
 *
 * We copy the running process and thread, set that new thread to be 
 * runnable and handcraft its kernel stack. You will much more details 
 * about that construction in @see context.h
 *
 * @return 0 to child and the child id to the parent on success, a 
 * 		negative error code on failure
 */
int _fork() {

	// Get the invoking thread
	thread_t *current = get_self();

	// Make sure the calling thread is the only thread of the process
	if (current->process->threads > 1)
		return ERR_MULTIPLE_THREADS;

	// Copy the process
	process_t *child = copy_process(current->process);
	if (child == NULL) return ERR_COPY_PRO_FAIL;

	// Now that the process is copied we need to create a thread
	thread_t *new = copy_thread(child, current, TRUE);
	if (new == NULL) {
	 	destroy_process(child);  
		return ERR_COPY_THR_FAIL;
	}

	// Make the thread runnable
	int err = set_runnable(new);
	if (err < 0) {
		destroy_process(child);
		destroy_thread(new);	
		return err;
	}

	// Handcraft the stack for the child at esp0
	child_stack(new->esp0, new, &(new->esp), current->esp0);
	
	// Note that the child never returns to that point. We make him
	// go directly to the fork_int function.

	// The id is 0 for the child, and the childs id for the parent
	return new->tid;
}


/**
 * @brief Sets the exit status of the current task
 * @param status the exit status for the calling thread
 */
void _set_status(int status) {	
	process_t *process = get_self()->process;

	// can't call panic, since it calls exit(x);
	if (process == NULL) kernel_panic("Can't set status without process");

	process->exit_status = status;
}

/**
 * @brief Collects the exit status of a task
 *
 * We look for a child which has exited and collect that exit status. If 
 * we have no child which exited yet we deschedule ourselves. When a child
 * does exit it checks whether its parent has waiting threads.
 *
 * @param status_ptr the placeholder for the exit status
 * @return the original thread of the exiting task
 */
int _wait(int *status_ptr) {
	if (status_ptr != NULL && !check_page((uint32_t)status_ptr, TRUE)) return ERR_INVALID_ARG;

	thread_t *self = get_self();
	process_t *task = self->process;

	// CHECK1 : Do we even have (alive) children ?
	if (task->children == 0) return ERR_NO_CHILDREN;

	// CHECK2 : Do we have more waiting threads than children ?
	if (task->children <= task->waiting->size) return ERR_WAIT_FULL;

	// Find an exited child	
	process_t *child;

	// If no child has exited yet
	while ((child = exited_child(task)) == NULL && task->children > 0) {
		dont_switch_me_out();

		int err = set_waiting(self);
		if (err < 0) return err;
	
		// Context Switch somewhere else
		thread_t *other = get_running();
		if (other == NULL) other = idle();
		context_switch(self, other);
	}

	// Are all children gone?
	if (task->children == 0) return ERR_CHILDREN_GONE;
	
	// Set the exit status of the child
	if (status_ptr != NULL)
		*status_ptr = child->exit_status;	

	// Return thread ID of original thread
	int original_tid = child->original_tid;
	if (original_tid == -1) return ERR_NO_ORIGINAL_THREAD;

	// Here we need to remove all resources of the process	
	int derr = destroy_process(child);
	if (derr) return derr;

	return original_tid;
}


/**
 * @brief Terminates the execution of the calling thread
 * 
 * We vanish the current thread. If we are the last thread of the process 
 * we also vanish the porcess and make our exit status available to a 
 * thread from the parent process via wait().
 *
 * When we vanish we cannot destroy and free all our resources since we 
 * are still running. Thus when a thread does call wait() it will take care
 * of destroying the process and all its threads entirely.
 *
 * Vanish thus simply changes the state of the thread so it cannot regain
 * execution in the future.
 */
void _vanish(void) {

	thread_t *self = get_self();

	dont_switch_me_out();

	// Now vanish !
	vanish_thread();

	thread_t *other = get_running();
	if (other == NULL) other = idle();

	// Are we the last thread of the task ?
	process_t *process = self->process;
	if (process->threads == 0) {

		vanish_process(process);

		// Is a thread from the parent waiting for us ?
		if (process->parent) {
			thread_t *waiting = NULL;
			waiting = get_waiting(process->parent);
			if (waiting != NULL) {
				set_runnable(waiting);
				other = waiting;
			}
		}
	}

	// Goodbye cruel world
	context_switch(self, other);
}


/**
 * @brief Creates a new thread associated with the calling process
 *
 * Very similar to the _fork() function but does not create a new process, 
 * but only spawns a new thread for the calling process.
 *
 * Once again, I highly encourage you to read context.h
 *
 * @return 0 to child and the child id to the parent on success, a 
 * 		negative error code on failure
 */
int _thread_fork(void) {
	
	// Get the invoking thread
	thread_t *current = get_self();
	if (current == NULL) return ERR_SELF_NULL;

	// Copy the process
	process_t *process = current->process;
	if (process == NULL) return ERR_COPY_PRO_FAIL;

	// Now that the process is copied we need to create a thread
	thread_t *new = copy_thread(process, current, FALSE);
	if (new == NULL) return ERR_COPY_THR_FAIL;
	
	// Make the thread runnable
	int err = set_runnable(new);
	if (err < 0) return err;

	// Handcraft the stack for the child at esp0
	child_stack(new->esp0, new, &(new->esp), current->esp0);

	// Note that the child never returns to that point. We make him
	// go directly to the fork_int function.

	// The id is 0 for the child, and the childs id for the parent
	return new->tid;
}

