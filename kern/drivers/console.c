/**
 * @file console.c
 * @brief Console output management
 * 
 * This file contains all functions needed to handle output of characters (and
 * color) to the machine's console display. It also contains the global
 * variables needed to keep the state of the console across the functions.
 * 
 * When a function expecting a location on the screen (row/column) gets an
 * illegal value, the driver's behaviour can be one of two options:
 * 1. The values are the stored position of the cursor. In that case,
 * something went wrong at some point in the execution, so we crash the kernel.
 * 2. The values are not the stored position of the cursor (new values): In
 * that case the function just doesn't do anything. It is interpreted as an
 * action taking place outside of the screen and therefore is not displayed in
 * any way.
 * The driver doesn't check the validity of the values (in particular the
 * pointers) passed to its functions. It is the responsibility of the system
 * calls to check those values before passing them down to the console.
 * 
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * @bugs No known bugs
 */

#include <ctype.h>
#include <stdlib.h>

#include <asm.h>
#include <types.h>
#include <video_defines.h>

#include <drivers.h>
#include <console.h>
#include <errors.h>

/* State variables */
static int cursor_row = 0; // Current row of the cursor (even hidden)
static int cursor_col = 0; // Current column of the cursor (even hidden)
static boolean_t cursor_hidden = FALSE; // Is the cursor hidden or not?
static int term_color = FGND_WHITE | BGND_RED; // Current output colors

/**
 * @brief Initializes the console state and resets the display
 * 
 * @return Void.
 */
void init_console() {
	clear_console();
	set_cursor(0,0);
	set_term_color(FGND_WHITE | BGND_RED);
}

/* Character display */

/* See p1kern.h for documentation */
int putbyte(char ch) {
	if (!check_cursor_position(cursor_row, cursor_col)) {
		kernel_panic("Invalid cursor position");
	}

	// Get global values
	int row = cursor_row;
	int col = cursor_col;
	int color;
	get_term_color(&color);

	switch(ch) {
		case '\n': // Line return, we get to the next line
			if (row < CONSOLE_HEIGHT - 1) {
				++row;
			} else { // Last line on screen, we've got to make some room
				scroll();
			}
			// No break, that's normal (\n is just an enhanced \r)
		case '\r': // Carriage return, we just come back to the line start
			set_cursor(row, 0);
			break;
		case '\b': // Delete (row and col will be the char to delete)
			if (col == 0) { // We are trying to delete a line break
				if (row == 0) { // We can't go above the first line
					break;
				}
				--row; //We'll end up on the line above anyway
				col = CONSOLE_WIDTH - 1;
			} else { // We just delete the previous character.
				--col;
			}

			// Character deletion
			draw_char(row, col, ' ', -1); // -1 means "keep color"
			set_cursor(row, col);
			break;
		default: // For any "regular" character
			if (isprint(ch)) {
				// We avoid unprintable chars to be consistent to the user
				draw_char(row, col, ch, color);
				next_cursor();
			}
	}

	return (int) ch;
}

/* see p1kern.h for documentation */
void putbytes(const char *s, int len) {
	int i;
	for (i = 0; i < len; ++i) {
		char c = s[i];
		if (c == '\0') {
			break;
		}

		// We write the string until it ends or we printed enough
		putbyte(c);
	}
}

/* see p1kern.h for documentation */
void draw_char(int row, int col, int ch, int color) {
	// We draw only if we're on the screen
	if (!check_cursor_position(row, col)) return;
	
	char* mem_pos = get_mem_pos(row, col);

	*mem_pos = ch;
	// A color of -1 means we want to keep the same color
	if (color != -1 && check_term_color(color)) {
		*(mem_pos + 1) = color;
	}
}

/* see p1kern.h for documentation */
char get_char(int row, int col) {
	// We can get information only about the screen
	if (!check_cursor_position(row, col)) return '\0'; // Default value

	char* mem_pos = get_mem_pos(row, col);
	return *mem_pos;
}

/**
 * @brief Copies one character from one location to another
 * 
 * @return 1 if the copy was succesful, 0 otherwise
 */
void copy_char(int from_row, int from_col, int to_row, int to_col) {
	if (!check_cursor_position(from_row, from_col)
		|| !check_cursor_position(to_row, to_col)) return;
	
	char* mem_pos_from = get_mem_pos(from_row, from_col);
	char* mem_pos_to = get_mem_pos(to_row, to_col);
	
	// We want to keep char and color together
	*mem_pos_to = *mem_pos_from;
	*(mem_pos_to + 1) = *(mem_pos_from + 1);
}

/**
 * @brief gets the address of the screen location in memory
 * 
 * The method doesn't check its arguments, it assumes that the caller will
 * have made sure that the values are within the console dimensions.
 */
char* get_mem_pos(int row, int col) {
	return (char*) (CONSOLE_MEM_BASE + 2*(row*CONSOLE_WIDTH + col));	
}

/**
 * @brief checks if the given position is on screen
 * 
 * @return TRUE if the position is valid, FALSE otherwise
 */
boolean_t check_cursor_position(int row, int col) {
	return row >= 0 && row < CONSOLE_HEIGHT && col >= 0 && col < CONSOLE_WIDTH;
}

/* Color management */

/* see p1kern.h for documentation */
int set_term_color(int color) {
	if (!check_term_color(color)) return -1;

	term_color = color;
	return 0;
}

/* see p1kern.h for documentation */
void get_term_color(int *color) {
	if (!check_term_color(term_color)) {
		kernel_panic("Invalid terminal color scheme");
	}

	*color = term_color;
}

/**
 * @brief checks if the given color code is valid
 * 
 * @return 1 if the color code is valid, 0 otherwise
 */
boolean_t check_term_color(int color) {
	return term_color <= 0xFF && term_color >= 0x00;
}

/* Cursor management */

/* see p1kern.h for documentation */
int set_cursor(int row, int col) {
	if (!check_cursor_position(row, col)) return -1;
	
	cursor_row = row;
	cursor_col = col;

	if (!cursor_hidden) { // We update the display if needed
		send_curpos(cursor_row, cursor_col);
	}

	return 0;
}

/* see p1kern.h for documentation */
void get_cursor(int *row, int *col) {
	if (!check_cursor_position(cursor_col, cursor_row)) {
		kernel_panic("Invalid cursor position");
	}

	*row = cursor_row;
	*col = cursor_col;
}

/* see p1kern.h for documentation */
void hide_cursor() {
	// Update the state and the display
	cursor_hidden = TRUE;
	send_curpos(CONSOLE_HEIGHT, CONSOLE_WIDTH);
}

/* see p1kern.h for documentation */
void show_cursor() {
	// Update the status and the display if possible
	cursor_hidden = FALSE;
	if (!check_cursor_position(cursor_row, cursor_col)) { // Should not happen
		kernel_panic("Invalid cursor position");
	}
	send_curpos(cursor_row, cursor_col);
}

/**
 * @brief Send the cursor information to the hardware for display
 */
void send_curpos(int row, int col) {
	if (!check_cursor_position(row, col)) return;

	// Index in memory table
	int offset = row * CONSOLE_WIDTH + col;

	// Transmission of the offset word
	outb(CRTC_IDX_REG, CRTC_CURSOR_LSB_IDX);
	outb(CRTC_DATA_REG, (uint8_t) offset);
	outb(CRTC_IDX_REG, CRTC_CURSOR_MSB_IDX);
	outb(CRTC_DATA_REG, (uint8_t)(offset >> 8));
}

/**
 * @brief Moves the cursor to its next legal position
 */
void next_cursor() {
	// We can do this only if we're inside the screen
	if (!check_cursor_position(cursor_row, cursor_col)) {
		panic("Invalid cursor position");
	}

	int row = cursor_row;
	int col = cursor_col;

	if (col < CONSOLE_WIDTH - 1) { // Just the next spot
		set_cursor(row, ++col);
	} else { // We have a line break to operate
		if (row < CONSOLE_HEIGHT - 1) { // Nothing's wrong
			set_cursor(++row, 0);
		} else { // We're at the last line, we must make some room
			scroll();
			set_cursor(row, 0);
		}
	}
}

/* General purpose utility functions (no argument check) */

/**
 * @brief Shifts the display up by one line
 */
void scroll() {
	int i,j;
	for (i = 0; i < CONSOLE_HEIGHT - 1; ++i) {
		for (j = 0; j < CONSOLE_WIDTH; ++j) {
			copy_char(i+1, j, i, j); // We copy each char one line above
		}
	}
	clear_row(CONSOLE_HEIGHT - 1); // We clear the last row
}

/* see p1kern.h for documentation */
void clear_console()
{
	int i;
	for (i = 0; i < CONSOLE_HEIGHT; ++i) {
		clear_row(i);
	}
	set_cursor(0,0); // Reset the cursor
}

/**
 * @brief Sets the given row to a blank state
 */
void clear_row(int row) {
	int j;
	for (j = 0; j < CONSOLE_WIDTH; ++j) {
			draw_char(row, j, ' ', FGND_WHITE | BGND_RED);
	}
}
