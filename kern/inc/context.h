/**
 * @file context.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Prototypes for context switching and handeling.
 *
 * This headefile includes all function which play a role in either the 
 * construction of a thread context, or the switch from one to another.
 *
 * Since these functions are quite complex you will find that both 
 * child_stack and context_switch have been implemented in assembly. Thus 
 * the doxygen has been provided here, but you will still find significant 
 * details in the .S files themselves.
 *
 * It might seem strange that the switch_stack is the only function 
 * without doxygen, but you will find it in the .c file as usual.
 */

#ifndef __P2_CONTEXT_H_
#define __P2_CONTEXT_H_

#include <thread.h>

/**
 * @brief Context Switch between two threads
 *
 * This function is responsible for transfering execution to a given
 * thread and should only be called from the kernel mode.
 *
 * The unit of context switching are threads, thus context switch will
 * receive two arguments, the calling thread and the thread which should
 * be executed next.
 *
 * Any thread which is "awaken" will always be inside the context_switch.
 * Thus we don't have to handle the program pointer at all.
 *
 * Given the sensitivity of the data we are handleling, like the esp0
 * (denoting the kernel stack), we are disabling interrupts for the 
 * integretity of the function.
 *
 * Likewise, during the events leading up to the context switch as well as
 * as during the context_switch function itself, the state of the kernel,
 * that is which threads are running, is not truthful and coherant 
 * anymore. Thus we exceptionally cannot authorize a timer interrupt to 
 * make another thread runnable.
 *
 * To ensure that during these sensitive moments when the kernel state 
 * does not correspond to reality (say we removed ourselves from the 
 * running list, but are still the thread holding the CPU), we use a 
 * simple "do not context switch" lock. So during the instructions leading 
 * up to the context switch that lock is set, and we are responsible for 
 * unsetting it at the end.
 *
 * To later restore the context of a thread we push all the relevant data 
 * onto the stack, and save a pointer to where the context was saved in 
 * the thread control block.
 * @see thread.h
 *
 * The steps we take during the switch are as folllows:
 *
 * 1. We push all general-purpose registers
 * 2. We call the stack_switch method which switches the value of %esp
 * 3. Then we pop all general-purpose registers
 * 4. Finally we call set_running which handles the cr3 and esp0
 *
 * Here is the stack created by context_switch :
 * 	+ ------------- +
 * 	|     other     | <-- the thread to switch to
 * 	+ ------------- +
 *	|      self	    | <-- ourselves
 *	+ ------------- +
 *	| ret to syscll |
 * 	+ ------------- +
 * 	|    old ebp	| <-- ebp
 * 	+ ------------- +
 * 	|     PUSHA	    |
 *	| 		        |
 *	+ ------------- + <-- esp (saved in the tcb)
 *
 *
 * So when we switch the stack, we know that our context is saved right 
 * where our esp is now pointing.
 *
 * Please @see context.S for more details.
 *
 * @param self the calling thread
 * @param other the thread to be executed
 */
void context_switch(thread_t *self, thread_t *other);

/**
 * @see stack.c
 */
unsigned int stack_switch(thread_t *self, thread_t *other, unsigned int);


/**
 * @brief Constructs the kernel stack of a new thread
 * 
 * This function will handcraft the kernel stack of a new thread so that 
 * we can later on transfer execution to him, for example through a call 
 * to yield(), and leave the kernel space as if we had previously made a 
 * system call.
 *
 * For a simple yield to work seemlessly as described above we need to 
 * create an artificel stack environment which simulates a system call as 
 * well as the stack structure required by the context_switch function 
 * described above.
 *
 * Luckily the thread which wants to create a new thread is himself in 
 * kernel mode with such a stack.
 *
 * Here is the stack of the "parent" thread, which desired to call into 
 * existence another one, from here on refered to as the "child" thread :
 * Let's say the parent called the fork() system call.
 *
 * Before entering _fork() we save information on the stack during
 * the fork_int function and INT call.
 *
 * 	+ ------------- +
 * 	|  general info	|
 *	+ ------------- +
 *
 * When we call child_stack from within _fork() we create :
 *	
 *	+ ------------- +
 *	|      args		|
 *	+ ------------- +	
 *	| ret to _fork 	|
 * 	+ ------------- +
 *	|    old_ebp	| <-- the base pointer of the _fork stack frame
 *	+ ------------- + 
 *	: pushed stuff	:
 *	+ - - - - - - - +
 *
 * Now as described above, the following stack layout is required to make
 * the after_switch (second part) from context_switch send us to child_ret.
 *
 *	+ ------------- + 
 *	|      self		|
 *	+ ------------- +
 *	|   child_ret	|
 *	+ ------------- +
 *	|    old_ebp    | <-- ebp
 *	+ ------------- +
 *	|				|
 *	|     PUSHA		|
 *	| 				|
 *	+ ------------- + <-- esp
 *
 * Given these infos we built the child stack based on these rules :
 * *
 * 1. We don't bother sending the child back into the _fork stack 
 * frame, and send him directly to the fork_int frame. Thus 
 * child_ret will be "child_ret_add" from fork_int
 *
 * @see lifecycle_wrappers.S
 *
 * 2. The ebp which we saved in the pusha block should point to
 * the old ebp, one block above the "pusha" block, so that the 'ret' 
 * instruction returns to 'child_ret'
 *
 * 3. The old_ebp in the second block is not important. it can be
 * funk, since we pop ebp in child_ret_addr.
 *
 * So this will be the stack the child thread will have :
 * 
 * 	+ ------------- +
 * 	|  	fork_int	|
 *	|  stack frame	|
 *	+ ------------- + 
 *	|      self		|
 *	+ ------------- +
 *	|   child_ret	|
 *	+ ------------- +
 *	|     junk      |
 *	+ ------------- +
 *	|				|
 *	|     PUSHA		|
 *	| 				|
 *	+ ------------- + <-- esp
 *
 * Please @see child_stack.S for more information.
 *
 * Finally it is important to note that the child will never return from 
 * the child_stack function, since we skipped that stack frame. He will 
 * "awake" directly at "child_ret_add" in the fork_int function after the 
 * context_switch restores the atificial context.
 *
 * @param new_esp0 the kernel stack of the child thread 
 * @param new_thread the child thread tcb needed by set_running later
 * @param new_esp the placeholder for the esp of the child thread
 * @param current_esp0 the kernel stack of the parent thread
 */
void child_stack(uint32_t, thread_t *, unsigned int *, unsigned int);

#endif /* !__P2_CONTEXT_H_ */

