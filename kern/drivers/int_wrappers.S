/**
 * @file int_wrappers.S
 * @brief Contains the assembly wrappers for the interrupt handlers.
 * 
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 */

#include <seg.h>

.global timer_interrupt_handler
.globl timer_handler

.global keyboard_interrupt_handler
.globl keyboard_handler

timer_interrupt_handler:
	# Save program state
	push %gs
	push %fs
	push %es
	push %ds
	pushal

	# Update kernel data segments
	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# Handler does stuff
	call timer_handler

	# Restore program state
	popal
	pop %ds
	pop %es
	pop %fs
	pop %gs

	# Return to normal execution
	iret

keyboard_interrupt_handler:
	# Save program state
	push %gs
	push %fs
	push %es
	push %ds
	pushal

	# Update kernel data segments
	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# Handler does stuff
	call keyboard_handler

	# Restore program state
	popal
	pop %ds
	pop %es
	pop %fs
	pop %gs

	# Return to normal execution
	iret