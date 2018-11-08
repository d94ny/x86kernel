#include <syscall_int.h>

.global gettid

gettid:
	int $GETTID_INT
	ret