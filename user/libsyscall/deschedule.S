#include <syscall_int.h>

.global deschedule

deschedule:
	pushl %esi
	movl 8(%esp), %esi
	int $DESCHEDULE_INT
	popl %esi
	ret