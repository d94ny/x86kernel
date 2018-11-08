/**
 * @file management.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Implementation of thread manangement realted functions.
 */

#include <thread.h>
#include <errors.h>
#include <context.h>	
#include <simics.h>
#include <stdlib.h>
#include <drivers.h>
#include <inc/syscall.h>
#include <ureg.h>
#include <common_kern.h>
#include <seg.h>
#include <syshelper.h>

#ifndef _SYSCALL_H
typedef void (*swexn_handler_t)(void *arg, ureg_t *ureg);
#endif

/**
 * @brief Deschedules the calling thread
 *
 * After having checked the given flag atomically with respect to 
 * make_runnable() on the same thread, we enter the "do not switch me out" 
 * mode, which corresponds to an unrealistic kernel state. Here we block 
 * ourselves, which will remove us from the running list, yet we still 
 * have the CPU. A context switch during that period could have damaging 
 * consequences.
 *
 * Once we are removed from the running list, in the blocked state, we 
 * look for another thread to run. Here we simply take the next thread on 
 * the running list, or idle if there is no such thread.
 *
 * @param flag determines the behavior of the method as descibed above
 * @return 0 on success, a negative error code otherwise.
 */
int _deschedule(int *flag) {
	if (!check_page((uint32_t)flag, FALSE)) return ERR_INVALID_ARG;

	thread_t *self = get_self();

	// Atomically with respect to make_runnable
	mutex_lock(self->thread_lock);		
	if (*flag != 0) {
		mutex_unlock(self->thread_lock);	
		return 0;
	}

	dont_switch_me_out();

	mutex_unlock(self->thread_lock);

	// Block ourselves
	int err = set_blocked(self);
	if (err < 0) {
		mutex_unlock(self->thread_lock);		
		return err;	
	}

	// Find a thread to execute
	thread_t *other = get_running();
	if (other == NULL) other = idle();

	context_switch(self, other);
	return 0;

}

/**
 * @brief Transfer execution of the invoking thread to another thread
 *
 * When a positive thread id is provided we verify that the thread is 
 * runnable (that is he has the  "THR_RUNNING" state). If that is not 
 * the case we return a negative error code.
 *
 * When -1 is given instead of a thread id we defer to the next thread on 
 * the "running" list. Since there might only be the invoking thread on 
 * the list yield() might have no effect.
 *
 * Since the invoking thread never leaves the "runnable" list we never 
 * defer execution to the idle thread.
 *
 * When we call set_runnable we are briefly completely abscent from the 
 * running list before being appended to the end. During that period the 
 * 'running' list property of having the running thread at its head is 
 * violated and we thus need to enter the "do not switch" mode.
 *
 * @param tid the thread id to transfer execution to or -1
 * @return 0 on success, a negative error code otherwise
 */
int _yield(int tid) {

	thread_t *self = get_self();

	// try to find the thread to which we should transfer execution
	thread_t *other = NULL;
	if (tid >= 0) {
		other = get_thread(tid);
		if (other == NULL || other->state != THR_RUNNING)
			return ERR_YIELD_NOT_RUNNABLE;
	}

	dont_switch_me_out();

	// In a yield we are still runnable
	set_runnable(self);

	// If yield was called with -1, we just run the next in line
	if (other == NULL) other = get_running();

	// Note that we could be the only one on "runnable" in which case 
	// self = other, and context_switch will return immediately
	context_switch(self, other);
	return 0;
}

/**
 * @brief Makes a deschedules thread runnable again
 *
 * We first look up the thread with given id in the hash table and verify 
 * it is indeed blocked. If so we atomically (with respect to 
 * deschedule on the same thread) append him to the runnable list and 
 * context switch to him if no error occured.
 *
 * @param tid the thread id of the thread to make runnable
 * @reurn 0 on success, a negative error code otherwise
 */
int _make_runnable(int tid) {

	// Make sure tid is valid
	if (tid < 0) return ERR_INVALID_TID;

	// Check if the thread is actually blocked
	thread_t *target = get_thread(tid);
	if (target == NULL || target->state != THR_BLOCKED)
		return ERR_NOT_BLOCKED;
	
	// Atomically with respect to deschedule
	thread_t *self = get_self();
	mutex_lock(target->thread_lock);

	dont_switch_me_out();

	// Otherwise make him runnabel again, but don't transfer yet
	int err = set_runnable(target);

	mutex_unlock(target->thread_lock);
	if (err == 0) context_switch(self, target);
	else you_can_switch_me_out_now();

	return err;	
}


/**
 * @brief Deschedules the calling thread until at least ticks timer 
 * interrupts have occured.
 *
 * We simply add  ourselves to the list of sleeping threads and transfer 
 * execution to either the next thread on the running list or idle if no 
 * such thread exists.
 *
 * @param ticks the number of ticks to sleep
 * @return 0 on sucess, a negative error code otherwise
 */
int _sleep(int ticks) {

	if (ticks == 0) return 0;
	if (ticks < 0) return ERR_NEGATIVE_SLEEP;

	thread_t *self = get_self();

	if (self == NULL) return ERR_SELF_NULL;

	dont_switch_me_out();

	int err = set_sleeping(self, ticks);	
	if (err < 0) {
		you_can_switch_me_out_now();
		return err;
	}

	// Transfer execution to next thread in line or idle
	thread_t *other = get_running();
	if (other == NULL) other = idle();

	context_switch(self, other);
	return 0;
}

/**
 * @brief Returns the number of timer ticks occured since the system boot
 * @return the number of timer ticks
 */
unsigned int _get_ticks(void) {
	return get_time();
}

/**
 * @return the thread ID of the invoking thread
 */ 
int _gettid() {
	return get_self()->tid;
}

/**
 * @brief Registers an exception handler for the current process
 * 
 * If both esp3 and eip are not NULL, this function registers an exception
 * handler for the current process. This handler will be called when an
 * exception is triggered by an instruction of this process, except exceptions
 * caused by kernel mechanichs such as copy on write or zero fill on demand.
 * 
 * If newureg is not null, the user gets the specified values in its registers
 * when it gets back to user space. The system call of course checks that the
 * sensible registers (most notably the segment registers and the EFLAGS
 * register) have suitable values. The user can slaughter itself, but it must
 * not, in any case, cause trouble in the kernel.
 * 
 * If one of the two operations can't be carried out when it should have, the
 * other one will not be served either.
 * 
 * @param esp3 the initial stack pointer for the exception stack
 * @param eip the address of the handler
 * @param arg the argument to the exception handler
 * @param newureg the new values for the thread's registers when it returns to
 * user space
 */
int _swexn(void **args) {
	if (!check_array(args, 4)) return ERR_INVALID_ARG;

	void *esp3 = args[0];
	if (esp3 != NULL && !check_page((uint32_t)esp3, TRUE))
		return ERR_INVALID_ARG;
	
	swexn_handler_t eip = args[1];
	if (eip != NULL && !check_page((uint32_t)eip, FALSE))
		return ERR_INVALID_ARG;
	if (eip != NULL && (uint32_t) eip < USER_MEM_START)
		return ERR_INVALID_ARG;

	void *arg = args[2];

	ureg_t *newureg = args[3];
	if (newureg != NULL &&
			!check_array(newureg, sizeof(ureg_t) / sizeof(void*)))
		return ERR_INVALID_ARG;
	thread_t *thread = get_running();
	uint32_t *esp0 = (void *) thread->esp0;
	if (newureg != NULL) {
		if ((newureg->ds != SEGSEL_USER_DS 
					&& newureg->ds != SEGSEL_USER_CS)
				|| (newureg->es != SEGSEL_USER_DS 
					&& newureg->es != SEGSEL_USER_CS)
				|| (newureg->fs != SEGSEL_USER_DS 
					&& newureg->fs != SEGSEL_USER_CS)
				|| (newureg->gs != SEGSEL_USER_DS 
					&& newureg->gs != SEGSEL_USER_CS)
				|| (((newureg->eflags ^ *(esp0 - 3)) 
					& ~AUTHORIZED_FLAGS) != 0)) {
			return ERR_INVALID_ARG;
		}
	}

	if (esp3 == NULL || eip == NULL) {
		// Deregister the currently registered exception handler
		thread->swexn_eip = 0x0;
		thread->swexn_esp = 0x0;
		thread->swexn_arg = NULL;
	} else {
		// Register a new exception handler
		thread->swexn_eip = (uint32_t) eip;
		thread->swexn_esp = (uint32_t) esp3;
		thread->swexn_arg = arg;
	}

	if (newureg != NULL) {
		// Adopt new ureg values

		// Segment registers (only user values possible)
		/*
		 * We don't forbid USER_CS as the threads can perfectly destroy
		 * themselves
		 */
		*(esp0 - 6) = newureg->ds;
		*(esp0 - 7) = newureg->es;
		*(esp0 - 8) = newureg->fs;
		*(esp0 - 9) = newureg->gs;

		// General purpose registers (can be anything)
		*(esp0 - 10) = newureg->ebp;
		*(esp0 - 11) = newureg->ebx;
		*(esp0 - 12) = newureg->ecx;
		*(esp0 - 13) = newureg->edx;
		*(esp0 - 14) = newureg->edi;
		*(esp0 - 15) = newureg->esi;

		// IRET stack frame (cs and ss MUST NOT be changed)
		//*(esp0 - 1) = newureg->ss;
		*(esp0 - 2) = newureg->esp; // Wrong values will be handled by paging
		// Bulletproof the new eflags with only authorized modifications
		uint32_t eflags = newureg->eflags & AUTHORIZED_FLAGS;
		*(esp0 - 3) |= eflags;
		//*(esp0 - 4) = newureg->cs;
		*(esp0 - 5) = newureg->eip; // Wrong values will be handled by paging

	}
	return 0;
}
