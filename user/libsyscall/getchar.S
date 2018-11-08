#include <syscall_int.h>

.global getchar

getchar:
	int $GETCHAR_INT
	ret