.global testandset

testandset:
	movl $1, %eax
	movl 4(%esp), %ecx
	xchgl %eax, (%ecx)
	ret