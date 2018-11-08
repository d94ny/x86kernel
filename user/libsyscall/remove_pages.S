#include <syscall_int.h>

.global remove_pages

remove_pages:
	pushl %esi
	movl 8(%esp), %esi
	int $REMOVE_PAGES_INT
	popl %esi
	ret