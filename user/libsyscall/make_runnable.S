#include <syscall_int.h>

.global make_runnable

make_runnable:
	pushl %esi
	movl 8(%esp), %esi
	int $MAKE_RUNNABLE_INT
	popl %esi
	ret