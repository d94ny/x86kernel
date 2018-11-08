/**
 * @file exception.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Exception handler for multithreaded applications
 * 
 * The handler reacts to faults by killing the task, and to traps by doing
 * nothing and resetting the exception handler, as the execution will continue
 * on the next line.
 * 
 * @bugs No known bugs
 */

#include <syscall.h>
#include <ureg.h>
#include <stdio.h>

extern void panic(const char *, ...);

/**
 * @brief Exception handler for multithreaded applications
 * 
 * cf. File description
 * 
 * @param arg the address to the exception stack (to register the handler
 * again if necessary)
 * @param ureg the state of the program at the moment of the exception
 */
void multi_swexn_handler(void *arg, ureg_t *ureg) {
	void *exception_stack_top = arg;
	switch(ureg->cause) {
		case SWEXN_CAUSE_DIVIDE:
			panic("Exception triggered: Division by 0.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_BOUNDCHECK:
			panic("Exception triggered: Array index out of bounds.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_OPCODE:
			panic("Exception triggered: Invalid opcode.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_NOFPU:
			panic("Exception triggered: Why the heck did you use the FPU??\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_SEGFAULT:
			panic("Exception triggered: Segmentation fault.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_STACKFAULT:
			panic("Exception triggered: Stack fault.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_PROTFAULT:
			panic("Exception triggered: Protection fault.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_PAGEFAULT:
			panic("Exception triggered: Page fault.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_FPUFAULT:
			panic("Exception triggered: Floating-point fault.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_ALIGNFAULT:
			panic("Exception triggered: Alignment fault.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_SIMDFAULT:
			panic("Exception triggered: SIMD floating-point fault.\n"
				"Killing task...");
			break;
		case SWEXN_CAUSE_DEBUG:
		case SWEXN_CAUSE_BREAKPOINT:
		case SWEXN_CAUSE_OVERFLOW:
			// Ignore those, they will continue at the next instruction
			swexn(exception_stack_top, multi_swexn_handler, exception_stack_top, ureg);
			break;
		default:
			// If this happens, then this is way worse than any of the above
			panic("Unknown exception triggered: %d\n"
				"Killing task...", ureg->cause);
	}
}
