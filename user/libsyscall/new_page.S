#include <syscall_int.h>

.global new_page

print:
	movl %esp, %esi
	addl $4, %esi
	int $PRINT_INT
	ret