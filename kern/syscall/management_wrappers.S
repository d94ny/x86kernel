#include <seg.h>

.globl _deschedule
.globl _gettid
.globl _yield
.globl _sleep
.globl _make_runnable
.globl _get_ticks
.globl _swexn

.global deschedule_int
.global gettid_int
.global yield_int
.global sleep_int
.global make_runnable_int
.global get_ticks_int
.global swexn_int

yield_int:
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
	call _yield
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

deschedule_int:
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
	call _deschedule
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


gettid_int:
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
	pushl %esi
	pushl %edi
	call _gettid
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %ebp
	pop %gs
	pop %fs
	pop %es
	pop %ds
	iret

get_ticks_int:
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
	pushl %esi
	pushl %edi
	call _get_ticks
	popl %edi
	popl %esi
	popl %edx
	popl %ecx
	popl %ebx
	popl %ebp
	pop %gs
	pop %fs
	pop %es
	pop %ds
	iret
make_runnable_int:
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
	call _make_runnable
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

sleep_int:
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
	call _sleep
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

swexn_int:
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
	call _swexn
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
