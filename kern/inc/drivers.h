/**
 * @file drivers.h
 * @brief Constants and function prototypes for driver code
 * 
 * @author Loic Ottet (lottet)
 * @bugs No known bugs
 */

#ifndef _KERN_DRIVERS_H_
#define _KERN_DRIVERS_H_

#include <x86/timer_defines.h>
#include <types.h>

/* Timer */

#define TIMER_INTERRUPT_RATE 100 // Number of interrupts per second
#define TIMER_CYCLES_PER_INTERRUPT TIMER_RATE / TIMER_INTERRUPT_RATE

void install_handlers(void);
void ack_interrupt(void);

void init_timer(void);
void timer_interrupt_handler(void);
unsigned int get_time(void);

void dont_switch_me_out(void);
void you_can_switch_me_out_now(void);

/* Keyboard */

// Buffer constant
#define KEY_BUFFER_SIZE 256
#define MAX_LINE_LENGTH 4096

void init_keyboard(void);
void keyboard_handler(void);
int readchar(void);
void keyboard_interrupt_handler(void);

/* Console */
void init_console(void);

char* get_mem_pos(int row, int col);
void copy_char(int from_row, int from_col, int to_row, int to_col);

boolean_t check_term_color(int color);

boolean_t check_cursor_position(int row, int col);
void send_curpos(int row, int col);
void next_cursor(void);

void scroll(void);
void clear_row(int row);

#endif /* _KERN_TIMER_H_ */
