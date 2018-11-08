#include <syscall_int.h>

.global swexn

swexn:
	pushl %esi
	movl %esp, %esi
	addl $8, %esi
	int $SWEXN_INT
	popl %esi
	ret
