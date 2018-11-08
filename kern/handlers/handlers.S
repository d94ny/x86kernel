/**
 * @file handlers.S
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * 
 * This file contains wrappers for the different exceptions that can occur
 * during the execution of a program. All fall back to the general
 * exception_handler, which in turn handles them as per exception.c
 */

#include <seg.h>
#include <simics.h>
#include <ureg.h>

.globl _page_fault_handler
.global page_fault_handler

.globl _exception_handler
.global divide_handler
.global debug_handler
.global breakpoint_handler
.global overflow_handler
.global boundcheck_handler
.global opcode_handler
.global nofpu_handler
.global segfault_handler
.global stackfault_handler
.global protfault_handler
.global fpufault_handler
.global alignfault_handler
.global smidfault_handler

page_fault_handler:
	# Save program state
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	# Set kernel data segments
	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# Handler does stuff
	call _page_fault_handler

	# If we're lucky enough, we come back and restore state
	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

# Generic handler, comments and code are similar for all of them
divide_handler:
	# Save program state
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	# Set kernel data segments
	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# Call the generic exception handler with the good params
	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_DIVIDE
	call _exception_handler

	# Discard the arguments on the stack
	addl $8, %esp

	# Restore program state
	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

debug_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_DEBUG
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

breakpoint_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_BREAKPOINT
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

overflow_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_OVERFLOW
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

boundcheck_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_BOUNDCHECK
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

opcode_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_OPCODE
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

nofpu_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_NOFPU
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

segfault_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_SEGFAULT
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

stackfault_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_STACKFAULT
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

protfault_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_PROTFAULT
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

fpufault_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_FPUFAULT
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

alignfault_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_ALIGNFAULT
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret

smidfault_handler:
	push %ds
	push %es
	push %fs
	push %gs
	pushal

	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl 0x30(%esp)			# Error code
	pushl $SWEXN_CAUSE_SIMDFAULT
	call _exception_handler

	addl $8, %esp

	popal
	pop %gs
	pop %fs
	pop %es
	pop %ds

	addl $4, %esp	# Discard the error code pushed onto the stack 

	iret