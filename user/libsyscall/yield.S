#include <syscall_int.h>

.global yield

yield:
	pushl %esi
	movl 8(%esp), %esi
	int $YIELD_INT
	popl %esi

	ret
