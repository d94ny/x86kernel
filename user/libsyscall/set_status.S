#include <syscall_int.h>

.global set_status

set_status:
	pushl %esi
	movl 8(%esp), %esi
	int $SET_STATUS_INT
	popl %esi
	ret