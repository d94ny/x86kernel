#include <syscall_int.h>

.global task_vanish

task_vanish:
	movl 4(%esp), %esi
	int $TASK_VANISH_INT
