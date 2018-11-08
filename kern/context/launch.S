/**
 * @file launch.S
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Responsible for transitioning a new thread to user mode. 
 * 
 * Please @see launch.h the header file for for information.
 */

#include <seg.h>

.globl launch


# void launch(unsigned long entry, uint32_t esp3);
launch:
	push	%ebp
	movl 	%esp, %ebp
	
	movl	8(%ebp), %edx		# the entry point
	movl	12(%ebp), %ecx		# the user stack

	mov	$SEGSEL_USER_DS, %ax
	mov		%ax, %ds			# change data segment register	
	mov 	%ax, %es 
	mov 	%ax, %fs
	mov 	%ax, %gs

	push	$SEGSEL_USER_DS		# push SS
	pushl	%ecx				# push esp3

	pushfl

	push 	$SEGSEL_USER_CS		# push CS
	pushl 	%edx				# push the entry point

	iret						# use iret to switch to user mode


