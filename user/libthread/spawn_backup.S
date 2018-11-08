#include <p2thread.h>
#include <syscall_int.h>

.globl thr_launch
.global thr_spawn

/**
 * @brief Spawns a new child thread
 *
 * thr_spawn is called by thr_create to fork a new thread and then launch 
 * it by calling thr_launch(). Since we don't know whether the parent 
 * thread or the child thread will obtain the CPU first after the system call 
 * to thread_fork we need to make sure that the stack pointer points to the 
 * base of the previously created child stack so that the parent thread is out 
 * of "reach".
 *
 * After the thread_fork we check if eax is 0, which means we are in the child 
 * thread and need to execute func. Otherwise we are in the parent thread and 
 * need to restore our original stack pointer before returning. 
 *
 * @param base the base of the child stack
 * @param func the function the child should execute
 * @param arg the argument of the function
 * @return the kernel ID of the child thread 
 *
 */

thr_spawn:

	# Standard caller-conventions
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx

	# Save the current stack pointer, pointing inside the parent stack.
	# We subtract 8 to make it point to the return address, the state it 
	# should be in right before the ret instruction
	movl %esp, %ebx
	subl $8, %ebx

	# Set the stack pointer inside the new child stack
	# The base is the first argument
	movl 8(%ebp), %esp

	# Call the dreaded thread_fork which makes a new register set
	int $THREAD_FORK_INT

	# Now we check in which thread we are. eax contains 0 if we are in the 
	# child thread, otherwise the kernel ID of the child if we are in the 
	# parent thread
	movl %eax, %eax
	jz child

		# We are in the parent thread
		# Restore the original stack pointer and return
		# No need to push ebx since our esp is already pointing to the 
		# return address
		movl %ebx, %esp
		ret

	child:

		# We are in the child thread
		# prepare the arguments for launch_thread
		pushl 16(%ebp)		# arg2 = arg
		pushl 12(%ebp)		# arg1 = func
		
		# Call launch_thread which executes func(arg) and then exits 
		# the thread using thr_exit()
		call thr_launch

		# we should never return from this
	
