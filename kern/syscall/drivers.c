/**
 * @file drivers.c
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * 
 * This file contains the syscalls related to input and output to and from the
 * console. Both are protected by a mutex, preventing multiple threads from
 * writing to the console at the same time, and from reading characters from
 * the input stream at the same time.
 * 
 * @bugs getchar() not implemented
 */

#include <simics.h>
#include <console.h>
#include <string.h>
#include <drivers.h>
#include <types.h>
#include <malloc.h>
#include <lock.h>
#include <errors.h>
#include <syshelper.h>

// Ensures that the prompt belongs to only one thread
static mutex_t input_mutex;
// Ensures that prints are not interleaved
static mutex_t output_mutex;

/**
 * @brief Initializes the driver mutexes
 */
void init_syscall_mutexes() {
	mutex_init(&input_mutex);
	mutex_init(&output_mutex);
}

/**
 * @brief Not implemented
 */
char _getchar() {
	lprintf("Getchar: feature not implemented");
	return -1;
}

/**
 * @brief Tries to read a line from the keyboard
 * 
 * If no '\n' terminated line of text is available at the moment, the thread
 * blocks (in readchar) and waits for a new character to be entered by the
 * user.
 */
int _readline(void** args) {
	if (!check_array(args, 2)) return ERR_INVALID_ARG;
	int size = (int) args[0];
	char *buf = (char*) args[1];

	if (!check_buffer(buf, size, TRUE)) return ERR_INVALID_ARG;
	if (size < 0 || size > MAX_LINE_LENGTH) return ERR_INVALID_ARG;

	char *line_buf = smalloc(size * sizeof(char));
	if (line_buf == NULL) return ERR_MALLOC_FAIL;

	// We get in queue
	mutex_lock(&input_mutex);

	int i;
	boolean_t done = FALSE;
	int cursor = 0;
	for(i = 0; i < size && !done; ++i) {
		char c = readchar();
		
		if (c != '\b' || cursor != 0) {
			putbyte(c);
		}

		switch(c) {
			case '\n':
				// Copy and return the buffer
				line_buf[cursor++] = c;
				strncpy(buf, line_buf, i + 1);
				done = TRUE;
			case '\b':
				if (cursor > 0) {
					line_buf[cursor--] = 0;
				}
			case -1:
				break;
			default:
				line_buf[cursor++] = c;
		}
	}

	mutex_unlock(&input_mutex);

	sfree(line_buf, size * sizeof(char));

	return i;
}

/**
 * @brief Prints the given string into the console
 */
int _print(void **args) {
	if (!check_array(args, 2)) return ERR_INVALID_ARG;
	int size = (int) args[0];
	char *buf = (char*) args[1];

	if (!check_buffer(buf, size, FALSE)) return ERR_INVALID_ARG;

	mutex_lock(&output_mutex);
	putbytes(buf, size);
	mutex_unlock(&output_mutex);

	return 0;
}

/**
 * @brief Sets the terminal font and background color
 */
int _set_term_color(int color) {
	mutex_lock(&output_mutex);
	int err = set_term_color(color);
	mutex_unlock(&output_mutex);
	return err;
}

/**
 * @brief Puts the current cursor position in the given pointers
 */
int _get_cursor_pos(int **args) {
	if (!check_array(args, 2)) return ERR_INVALID_ARG;	
	int *row = args[0];
	int *col = args[1];

	if (!check_page((uint32_t)row, TRUE)) return ERR_INVALID_ARG;
	if (!check_page((uint32_t)col, TRUE)) return ERR_INVALID_ARG;
	
	mutex_lock(&output_mutex);
	get_cursor(row, col);
	mutex_unlock(&output_mutex);
	return 0;
}

/**
 * @brief Sets the console cursor position to the given values
 */
int _set_cursor_pos(int *args) {
	if (!check_array(args, 2)) return ERR_INVALID_ARG;
	int row = args[0];
	int col = args[1];

	mutex_lock(&output_mutex);
	int err = set_cursor(row, col);
	mutex_unlock(&output_mutex);

	return err;
}
