#include <syscall_int.h>

.global readline

readline:
	pushl %esi
	movl %esp, %esi
	addl $8, %esi
	int $READLINE_INT
	popl %esi
	ret