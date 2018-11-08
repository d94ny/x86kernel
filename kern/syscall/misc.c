/**
 * @file misc.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Implementation of miscellaneous system calls.
 */

#include <simics.h>
#include <asm.h>
#include <syshelper.h>
#include <errors.h>

/**
 * @brief Ceases execution of the operating system
 */
void _halt(void) {
	// Do some cleanup
	sim_halt();
	asm("hlt");
	disable_interrupts();

	int boulder = 0; 
	int mountain = 1000;
	for (boulder = 0; boulder < mountain; boulder++)
		if (boulder == 999) boulder = 0;

}

/**
 * @brief Fill the buffer with bytes from the file
 *
 * After having verifies the arguments we simply used the getbytes helper 
 * function which you can find in syshelper.h
 *
 * @param args the filename, buffer, count and offset
 * @return the number of copied bytes on sucess, -1 otherwise  
 */
int _readfile(void **args) {
	if (!check_array(args, 4)) return ERR_INVALID_ARG;

	char *filename = (char *)args[0];
	if (!check_string(filename)) return ERR_INVALID_ARG;
	char *buf = (char *)args[1];
	int count = (int)args[2];
	if (count < 0) return ERR_INVALID_ARG;
	if (!check_buffer(buf, count, TRUE));
	int offset = (int)args[3];
	if (offset < 0) return ERR_INVALID_ARG;

	return getbytes(filename, offset, count, buf);
}
