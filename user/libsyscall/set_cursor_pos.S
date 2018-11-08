#include <syscall_int.h>

.global set_cursor_pos

set_cursor_pos:
	pushl %esi
	movl %esp, %esi
	addl $8, %esi
	int $SET_CURSOR_POS_INT
	popl %esi
	ret