/**
 * @file context.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Implementation of the context switch between two threads.
 *
 * Since this is an assembly file, the doxygen along with a general 
 * explanation of the context_switch mechanisms (with beautiful stack 
 * pictures) are included in the header file.
 *
 * @see context.h 
 */

#include <simics.h>

.globl context_switch
.globl after_switch
.global stack_switch
.global set_running
.global you_can_switch_me_out_now

# void context_switch(thread_t *self, thread_t *other)
context_switch:

	cli
	push %ebp
	movl %esp, %ebp

	pushal	

	# Check if self and other are identical in which we just return
	movl 12(%ebp), %ecx
	movl 8(%ebp), %edx
	cmp %ecx, %edx
	je no_context_switch
	

	pushl %esp			# push the esp right after `pusha`
	pushl 12(%ebp)		# push other
	pushl 8(%ebp)		# push self
	
	call stack_switch	# switch the stack pointers
	movl %eax, %esp		# eax contains the new esp
						# we are now on other's stack /!\

	after_switch:
	
	# On the "other" stack esp points to the top of the "pusha"
	# section since we pushed "esp" right after the "pusha"

	popal				# restore all the registers of 'other'
						# note that 'other' is now actually 'self'
						# also restores the ebp, so we can acces
						# the arguments correctly
	
	# Now esp+ebp point to the old_ebp, which is just 0
	
	pushl 8(%ebp)		# call set_running with self
						# Note that %ebp should be pointing
						# inside the kernel space, which is 
						# direct mapped

	call set_running	# this sets eps0 and cr3

	pop %eax			# pop the what we just pushed so that now
						# esp points to the return address to
						# the syscall call which "others" called
						# to context_switch
						# Use %eax since context_switch has no
						# return value anyway

	call you_can_switch_me_out_now
	
	popl %ebp			# restore the ebp of the systtem call
						# this is 0 for child thread created by 
						# fork

	sti					# enable interrupts	
	ret					# return to the calling system call

	no_context_switch:	# If we had self == other just exit

	popal

	call you_can_switch_me_out_now

	popl %ebp
	sti
	ret	

