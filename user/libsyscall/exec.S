#include <syscall_int.h>
.globl sim_breakpoint
.global exec

exec:
	pushl %esi
	movl %esp, %esi
	addl $8, %esi
	int $EXEC_INT
	popl %esi
	ret
