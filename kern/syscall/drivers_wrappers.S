/**
 * @file drivers_wrappers.S
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * 
 * This file contains assembly wrappers around the system calls in drivers.c,
 * allowing to save and restore program state while the thread is in kernel
 * mode.
 */

#include <seg.h>

.globl _getchar
.globl _readline
.globl _print
.globl _set_term_color
.globl _get_cursor_pos
.globl _set_cursor_pos

.global getchar_int
.global readline_int
.global print_int
.global set_term_color_int
.global get_cursor_pos_int
.global set_cursor_pos_int

getchar_int:
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
	call _getchar
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

readline_int:
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
	call _readline
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

print_int:
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
	call _print
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

set_term_color_int:
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
	call _set_term_color
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

get_cursor_pos_int:
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
	call _get_cursor_pos
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

set_cursor_pos_int:
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
	call _set_cursor_pos
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