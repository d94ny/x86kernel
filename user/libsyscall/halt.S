#include <syscall_int.h>

.global halt

halt:
	int $HALT_INT
	ret
