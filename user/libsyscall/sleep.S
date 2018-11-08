#include <syscall_int.h>

.global sleep

sleep:
	pushl %esi
	movl 8(%esp), %esi
	int $SLEEP_INT
	popl %esi
	ret