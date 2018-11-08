#include <syscall_int.h>

.global print

print:
	pushl %esi
	movl %esp, %esi
	addl $8, %esi
	int $PRINT_INT
	popl %esi
	ret