/**
 * @file keyboard.c
 * @brief The keyboard handler
 * 
 * The keyboard handler receives interrupts from the hardware keyboard, stores
 * them in a buffer so they can be read afterwards by the readchar() function.
 * 
 * A note about the buffer. It has two index variables pointing to the last 
 * entered (by the handler) and last processed (by readchar()) indexes in the
 * array. When those numbers are different, everything is fine. But when both
 * are equal, we can be in two very different situations. The first (and
 * simple) case is when the buffer is empty. In this case we just have to
 * prevent readchar from processing the next character, as it is currently
 * junk. The second case is a bit trickier. It occurs when the buffer is full,
 * and cannot hold the additional char we try to fit in. In that case, the
 * design decision is to drop the incoming character without ever processing
 * it, giving priority to the previously entered values. The rationale behind
 * this is that the "user" wants to see what he entered first occur first in
 * any case. Printing the last half of a phrase or playing the end of a
 * sequence of actions in a game, for example, are not an expected behavior
 * for the program.
 * 
 * We chose the approach to disable interrupts for keyboard-related operations
 * instead of a locking solution to keep the interrupts as short as possible.
 * 
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * @bugs No known bugs
 */

#include <stdio.h>

#include <stdint.h>
#include <asm.h>
#include <keyhelp.h>
#include <seg.h>

#include <drivers.h>
#include <interrupts.h>
#include <lock.h>

/* Global buffer variables */
static uint8_t keyboard_buffer[KEY_BUFFER_SIZE];
// Index of the last key sent by the keyboard
static unsigned int last_entered = KEY_BUFFER_SIZE;
// Index of the last key sent to the user
static unsigned int last_processed = KEY_BUFFER_SIZE;
// Is the buffer full? (In the case where last_entered = last_processed)
static boolean_t is_buffer_full = FALSE;
// Mutex protecting the buffer
static mutex_t keyboard_mutex;
// Condition variable on new input to the buffer
static cond_t new_key;

/**
 * @brief initializes the keyboard handler
 * 
 * init_handlers() ensures that no interruption is called during the setup of
 * the drivers.
 */
void init_keyboard() {
	// Create the gate
	trap_gate_t keyboard_gate;
	keyboard_gate.segment = SEGSEL_KERNEL_CS;
	keyboard_gate.offset = (uint32_t) keyboard_interrupt_handler;
	keyboard_gate.privilege_level = 0x0;

	// Store it in the IDT
	insert_to_idt(create_trap_idt_entry(&keyboard_gate), KEY_IDT_ENTRY);
	
	mutex_init(&keyboard_mutex);
	cond_init(&new_key);
}

/**
 * @brief Called when an interrupt is sent by the keyboard
 * 
 * We signal the condition variable only after acknowledging the interrupt 
 * to ensure that the interrupts keep flowing even if the thread somehow 
 * gets stuck in the cond_signal call.
 */
void keyboard_handler() {

	mutex_lock(&keyboard_mutex);

	// We store the character in the next available buffer spot
	unsigned int next_char_idx = (last_entered + 1) % KEY_BUFFER_SIZE;
	if (!is_buffer_full) {
		keyboard_buffer[next_char_idx] = inb(KEYBOARD_PORT);
		last_entered = next_char_idx;
		
		if (last_entered == last_processed) {
			is_buffer_full = TRUE;
		}	
	}

	mutex_unlock(&keyboard_mutex);
	ack_interrupt();

	cond_signal(&new_key);
}

/* see p1kern.h for documentation */
/* Accessing the buffer and updating the last_processed variable is an
 * operation that has to be made atomical to avoid that a timer-driven
 * interrupt begins a new readchar while we are doing this one.
 */
int readchar() {
	kh_type k = 0;

	mutex_lock(&keyboard_mutex);
	do {
		if ((last_processed != last_entered) || is_buffer_full) {
			// We have characters to process
			unsigned int next_char_idx = (last_processed + 1) %
				KEY_BUFFER_SIZE;
			k = process_scancode(keyboard_buffer[next_char_idx]);
			last_processed = next_char_idx;

			if (is_buffer_full) {
				is_buffer_full = FALSE;
			}
		} else cond_wait(&new_key, &keyboard_mutex);

	} while (!KH_ISMAKE(k) || !KH_HASDATA(k));
	mutex_unlock(&keyboard_mutex);

	return KH_GETCHAR(k);
}
