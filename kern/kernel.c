/** @file kernel.c
 *  @brief An initial kernel.c
 *
 *  You should initialize things in kernel_main(),
 *  and then run stuff.
 *
 *  @author Harry Q. Bovik (hqbovik)
 *  @author Fred Hacker (fhacker)
 *  @bug No known bugs.
 */

#include <common_kern.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */

#include <page.h>
#include <inc/syscall.h>

#include <thread.h>
#include <process.h>
#include <malloc.h>
#include <x86/cr.h>
#include <drivers.h>
#include <errors.h>
#include <interrupts.h>

/** @brief Kernel entrypoint.
 *  
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp) {

	int err;

	// Initialize paging
	err = install_paging(mbinfo->mem_upper);
	if (err) kernel_panic("Unable to setup paging. Error %d", err);

	// Init threads
	thread_init();

	// Install syscalls
	err = install_syscalls();
	if (err) kernel_panic("Unable to setup syscalls. Error %d", err);

	// Install drivers
	install_handlers();

	/**
	 * Create the "god" process, which will then use the system calls 
	 * york and exec to launch 'idle' and 'init'. This function 
	 * differs from the normal create_process because we create a user 
	 * stack for god, which we do not when calling fork();
	 */
	process_t *process = create_god_process();
	if (process == NULL)
		kernel_panic("Unable to create god process. Error %d", err);

	thread_t *thread = create_thread(process);
	if (thread == NULL)
		kernel_panic("Unable to create god thread. Error %d", err);

	// Activate paging with the page directory of the god process
	set_running(thread);

	// Now that we have a thread we can start using mutexes
	install_mutex();
	enable_interrupts();

	void **args = calloc(2, sizeof(void*));
	args[0] = (void*) "god";
	args[1] = NULL;

	err = _exec(args);
	kernel_panic("THERE IS NO GOD ! (error %d)", err);	

	while (1) continue;
	return 0;
}
