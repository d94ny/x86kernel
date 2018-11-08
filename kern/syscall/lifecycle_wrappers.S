#include <seg.h>

.globl _exec
.globl _fork
.globl _set_status
.globl _wait
.globl _vanish
.globl _thread_fork

.global exec_int
.global fork_int
.global set_status_int
.global wait_int
.global vanish_int
.global thread_fork_int

.global child_ret_addr
.global before_iret
.global exec_ret

exec_int:
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
	call _exec
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

fork_int:
	push %ds
	push %es
	push %fs
	push %gs
	mov $SEGSEL_KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	pushl %ebp		# push everything except %eax
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushl %edi
	pushl %esi

	call _fork		# _fork returns either 0 or child id
	exec_ret:
	jmp parent_ret_addr

	child_ret_addr:

	# This point is reached only by the child, when after_switch 
	# returns, since we have given this address as the return address 
	# on the handcrafted stack.
	#
	# Now we have a pointer to self (just above the child return 
	# address). Thus we pop something to get the same state as the 
	# parent was before the call to _fork

	popl %eax
	movl $0, %eax

	# Here the stack should be as if we were the parent

	parent_ret_addr:
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


set_status_int:
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
	call _set_status
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


wait_int:
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
	call _wait
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

vanish_int:
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
	call _vanish
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

thread_fork_int:
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
	call _thread_fork
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
	before_iret:
	iret

