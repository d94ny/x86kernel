/**
 * @file mutex_asm.S
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * 
 * Contains the atomical testandset function to implement locking
 */

.global testandset

testandset:
	movl $1, %eax
	movl 4(%esp), %ecx
	xchgl %eax, (%ecx)
	ret