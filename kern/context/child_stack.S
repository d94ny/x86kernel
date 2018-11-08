/**
 * @file child_stack.S
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Defines the function responsible for building a thread stack
 *
 * This being an assembly file, you will find more informations and 
 * details about the stack construction process in the header file.
 *
 * @see context.h
 */


.global child_stack

# int child_stack(uint32_t new->esp0,
#	thread_t *new,
#	unsigned int * new->esp,
#	unsigned int current->esp0);

child_stack:
	
	pushl %ebp
	movl %esp, %ebp

	pushl %ebx
	pushl %edx
	pushl %ecx

	# Step 1 : Copy the fork_int stack frame to the kernel stack of 
	#  the child. We stop when we reach the base address of _fork.
	#  That address is currently saved as old_ebp.

	movl 20(%ebp), %eax		# current->esp0
	movl 8(%ebp), %ebx		# new->esp0
	movl (%ebp), %ecx		# the _fork base pointer
	addl $4, %ecx			# One word further up (on ret_adr) 
							# we don't want to copy the return 
							# address
	
	# Copy the fork_int stack frame
	copy_parent_stack:
	
		movl (%eax), %edx 
		movl %edx, (%ebx)

		subl $4, %eax
		subl $4, %ebx

		cmp %ecx, %eax			# check if we reached the bottom of the
								# bottom of the fork_int stack frame
		jne copy_parent_stack	# continue if not
	

	# We copied everything up to but excluding the return address of
	# fork_int. We ignore it since we use a custom resturn address.
	# Note that we also don't care about the "old_ebp" since we pop it 
	# again in fork_int.

	# Step 2 : Handcraft the stack for the second part of the context
	#  switch so that a yield to the child works correctly. The return
	#  address we put on the stack will point to the child_ret_addr in
	#  which we just return 0.

	# %ebx is the "stack pointer" into the child kernel stack
	movl 12(%ebp), %edx
	movl %edx, (%ebx)		# Save self for context_switch
	
	movl $child_ret_addr, -4(%ebx)	# Save the return address
	movl $0, -8(%ebx)		# save junk for the ebp


	# Now we simulate the "pushal" instruction. Note that most of them 
	# don't matter since fork_int pops them all again. Only %ebp 
	# requires some thoughts.	
		
	movl %ebx, %edx
	subl $8, %edx			# edx now is the ebp we save in 
							# the pusha block, which points
							# to the old_ebp (which here is 0)

	# The order is eax, ecx, edx, ebx, esp, ebp, esi, edi
	movl $0, -12(%ebx)
	movl $0, -16(%ebx)
	movl $0, -20(%ebx)
	movl $0, -24(%ebx)
	movl %edx, -28(%ebx)	# Both ebp and esp point to the 
	movl %edx, -32(%ebx)	# base of the "pushal" block
	movl $0, -36(%ebx)
	movl $0, -40(%ebx)


	# Step 3 : Now save the esp of the tcb structure of the child to
	#  the top of the pusha block. That is 8 bellow %edx
	subl $32, %edx
	movl 16(%ebp), %ecx	
	movl %edx, (%ecx)		# into &(new->esp) 	


	# Now the child is ready for adventures !
	
	popl %ebx
	popl %edx
	popl %ecx
	popl %ebp
	ret

