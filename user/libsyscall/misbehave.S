#include <syscall_int.h>

.global misbehave

misbehave:
	pushl %esi
	movl 8(%esp), %esi
	int $MISBEHAVE_INT
	popl %esi
	ret
