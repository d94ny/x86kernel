/**
 * @file drivers.c
 * @brief Contains the driver initialization function
 * 
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * @bugs No bugs known
 */

#include <asm.h>
#include <interrupt_defines.h>

#include <drivers.h>

/**
 * @brief Sets up the drivers for a smoothly working user interface
 */
void install_handlers() {
	init_console();
	init_keyboard();
	init_timer();
}

/**
 * @brief Sends an acknowledgement message to the interrupt control
 */
void ack_interrupt() {
	outb(INT_CTL_PORT, INT_ACK_CURRENT);
}
