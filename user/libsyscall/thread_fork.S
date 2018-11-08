#include <syscall_int.h>

.global thread_fork

thread_fork:
	int $THREAD_FORK_INT
