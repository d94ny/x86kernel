/**
 * @file autostack.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Implements automatic stack growth for legacy single threaded programs
 * 
 * Registers an exception handler to handle page faults in a single threaded
 * program. If a page fault is catched, the handler sees if it is within a
 * reasonable range under the current stack and if it is the case tries to
 * expand the stack. If, for any reason, this process fails, the handler
 * passes the instruction back to the execution to trigger the actual kernel 
 * exception.
 * 
 * @bugs No bugs known
 */

#include <stdlib.h>
#include <syscall.h>
#include <assert.h>

#define STACK_SIZE_WORDS (PAGE_SIZE/sizeof(void *)) // # of words in a page
#define MAX_ENLARGEMENT_POWER 6 // Stack growth will be at most 2^6 = 64 pages

/**
 * The structure represents the limits of a stack space
 */
typedef struct stack_limits {
	unsigned int stack_high;	// Highest address in the stack
	unsigned int stack_low;		// Lowest address in the stack
} stack_limits_t;

// Pointer to a free allocated memory zone for the handler to have a stack
static void **exception_stack = NULL;

/**
 * @brief Handles the exceptions received
 * 
 * If this is a page fault, does stuff according the the file description
 * 
 * @param arg the limits of the current stack region
 * @param ureg the processor state at the moment of the fault
 */
void swexn_handler(void *arg, ureg_t *ureg) {
	stack_limits_t *stack_limits = (stack_limits_t *) arg;
	if (ureg->cause == SWEXN_CAUSE_PAGEFAULT) {
		// We have catched a page fault

		/* We have to check that the address is in a valid expansion region
		 * (twice the actual stack)
		 */
		unsigned int illegal_value = (unsigned int) ureg->cr2;
		unsigned int stack_size = stack_limits->stack_high
			- stack_limits->stack_low + 4;
		assert(stack_size % PAGE_SIZE == 0);

		int i, k;
		for (i = 1, k = 1; i <= MAX_ENLARGEMENT_POWER; ++i, k *= 2) {
			unsigned int new_stack_pages = k - 1;
			unsigned int new_stack_low = stack_limits->stack_low
				- new_stack_pages * PAGE_SIZE;

			if (illegal_value < stack_limits->stack_low
				&& illegal_value >= new_stack_low) {
				// We increase the stack
				if (new_pages((void *) new_stack_low,
						new_stack_pages * PAGE_SIZE) == 0) {
					/* We reexecute the instruction as usual, and reexecute
					 * the incriminated instruction
					 */
					stack_limits->stack_low = new_stack_low;
					swexn(exception_stack[STACK_SIZE_WORDS - 1], swexn_handler,
						stack_limits, ureg);
					return;
				}
			}
		}
		
	}
	/* This is a classic exception, we just have to handle the fault and exit
	 * the program
	 */
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
			swexn(exception_stack, swexn_handler,
				stack_limits, ureg);
			break;
		default:
			// If this happens, then this is way worse than any of the above
			panic("Unknown exception triggered: %d\n"
				"Killing task...", ureg->cause);
	}
}

/**
 * @brief Registers the exception handler at the beginning of the program
 * 
 * Allocates a stack and registers the handler
 * 
 * @param stack_high the highest address of the original stack
 * @param stack_low the lowest address of the original stack
 */
void install_autostack(void *stack_high, void *stack_low) {
	// Allocate exception stack (on the heap, yes...)
	exception_stack = calloc(PAGE_SIZE, sizeof(char));
	
	// Create the stack_limits structure
	stack_limits_t *stack_limits = malloc(sizeof(stack_limits_t));
	stack_limits->stack_high = (unsigned int) stack_high;
	stack_limits->stack_low = (unsigned int) stack_low;

	// Register exception handler
	swexn(&(exception_stack[STACK_SIZE_WORDS - 1]), swexn_handler,
		stack_limits, NULL);
}

