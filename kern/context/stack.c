/**
 * @file stack.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Implementation of the stack switch required by context_switch.
 */

#include <types.h>
#include <thread.h>
#include <context.h>
#include <simics.h>

/**
 * @brief Switches the stack pointer to a new thread
 *
 * This function is called by context_switch in order to jump from one 
 * kernel stack to another. 
 *
 * When doing so we save the given esp in our control block so we know 
 * where to find our context later.
 *
 * The actual "jump" will be done in assembly as 
 * soon as this function returns though.
 *
 * This was implemented in C rather than assembly like the rest of the 
 * "context" family for more flexibility of the thread control blocks.
 *
 * @param self the calling thread
 * @param other the thread to be executed shortly
 * @param esp the esp where our context was saved
 *
 * @return the esp where the other thread context is 
 */
unsigned int stack_switch(thread_t *self, thread_t *other,
		unsigned int esp) {

	self->esp = esp;
	return other->esp;
}

