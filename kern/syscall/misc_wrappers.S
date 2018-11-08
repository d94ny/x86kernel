/**
 * @file misc_wrappers.S
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * 
 * This file contains assembly wrappers around the system calls in misc.c,
 * allowing to save and restore program state while the thread is in kernel
 * mode.
 */

 #include <seg.h>

.globl _halt
.global halt_int

.globl _readfile
.global readfile_int

halt_int:
	push %ds
	push %es
	push %fs
	push %gs
	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl %ebp
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	call _halt
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %ebp

	pop %gs
	pop %fs
	pop %es
	pop %ds
	iret

readfile_int:
	push %ds
	push %es
	push %fs
	push %gs
	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl %ebp
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi
	call _readfile
	popl %esi
	popl %edi
	popl %edx
	popl %ecx
	popl %ebx
	popl %ebp

	pop %gs
	pop %fs
	pop %es
	pop %ds
	iret
