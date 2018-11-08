#include <syscall_int.h>

.global readfile

readfile:
	pushl %esi
	movl %esp, %esi
	addl $8, %esi
	int $READFILE_INT
	popl %esi
	ret
