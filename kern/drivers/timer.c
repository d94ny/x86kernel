/**
 * @file timer.c
 * @brief The timer interrupt handler
 * 
 * The timer interrupt handler receives interrupts from the clock at set
 * intervals and handled them by incrementing an internal counter and calling
 * a "user"-defined callback function.
 * 
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * @bugs (minor) What if num_ticks overflows? C's default behavior is to reset
 * the counter to 0, which seems to be the most rational thing to do.
 */

#include <stdlib.h>
#include <inc/stdint.h>
#include <x86/asm.h>
#include <x86/seg.h>
#include <x86/timer_defines.h>
#include <thread.h>
#include <context.h>
#include <simics.h>
#include <assert.h>

#include <drivers.h>
#include <interrupts.h>


// Global variables
static unsigned int num_ticks = 0; // Clock counter (somewhat)
static boolean_t no_switch = FALSE; // Can we initiate context switch ?

/**
 * @brief Initializes the timer handler so it does its job
 * 
 * Adds the handler to the IDT and launches the hardware clock.
 * 
 * init_handler() ensures that no interruption occurs during the setup
 * 
 * @param tickback The function called at each interrupt
 */
void init_timer() {
	// Gate creation
	trap_gate_t timer_gate;
	timer_gate.segment = SEGSEL_KERNEL_CS;
	timer_gate.offset = (uint32_t) timer_interrupt_handler;
	timer_gate.privilege_level = 0x00;

	// Insertion
	insert_to_idt(create_trap_idt_entry(&timer_gate), TIMER_IDT_ENTRY);

	// Period computation
	uint16_t period = TIMER_CYCLES_PER_INTERRUPT;
	uint8_t period_lsb = period & 0xFF;
	uint8_t period_msb = period >> 8;

	// Send period to hardware
	outb(TIMER_MODE_IO_PORT, TIMER_SQUARE_WAVE);
	outb(TIMER_PERIOD_IO_PORT, period_lsb);
	outb(TIMER_PERIOD_IO_PORT, period_msb);
}

/**
 * @brief Calls the tickback function and increments the counter
 * 
 * The kernel convention is that it is the responsibility of the tickback
 * function to disable interrupts if its actions need to access shared data
 * structures in an atomical way. The rationale is that there is no sense in
 * disabling and then enabling the interrupts over an empty tickback function.
 */
void timer_handler() {
	++num_ticks;

	if (no_switch) {
		ack_interrupt();	
		return;
	}

	// Awake sleeping threads
	thread_t *head;
	boolean_t awoken = FALSE;
	while ((head = get_sleeping()) != NULL
		&& head->wake <= num_ticks) {
	
		// Make the thread runnable again but don't transfer
		set_runnable(head);
		awoken = TRUE;
	}

	// If we are the idle thread and woke someone up
	if (awoken && is_idle(get_running())) {
		thread_t *self = get_self();
		unset_state(self);
		thread_t *other = get_running();
		dont_switch_me_out();
		ack_interrupt();
		context_switch(self, other);
		return;
	}

	// Time slicing
	/**
	 * Since we context_switch to idle, idle will be on the 
	 * "running" list. Thus we actually never have an empty 
	 * list. So to see if we have another thread other than 
	 * idle to run the size of "running" must be greater than 
	 * 1.
	 */
	thread_t *other = NULL;
	thread_t *self = get_self();

	/**
	 * If we aren't the idle thread we just transfer to the 
	 * next thread in line.
	 */
	if (!is_idle(self)) {

		// Add ourselves to the end of the list
		set_runnable(self);
		other = get_running(); // Never NULL, but check

	} else {

		// We need to check if we have other threads ready
		unsigned int ready = num_runnable();
		if (ready > 1) {
			// Remove idle from the list
			unset_state(self);
			other = get_running();
		}
	}

	dont_switch_me_out();
	ack_interrupt();
	if (other != NULL) context_switch(self, other);
	else you_can_switch_me_out_now();

	return;	

}


/**
 * @brief Retrieve the number of timer ticks since system boot
 * @return the number of timer ticks since system boot
 */
unsigned int get_time(void) {
	return num_ticks;
}

/**
 * @brief Asks not to be switched out by a clock interrupt
 * 
 * A thread will call this function when it is acting on critical data
 * structures (most notably the running list) and a context switch at that
 * moment would most certainly result in inconsistencies within the kernel.
 */
void dont_switch_me_out() {
	no_switch = TRUE;
}

/**
 * @brief Exits the dont_switch_me_out area.
 */
void you_can_switch_me_out_now() {
	no_switch = FALSE;
}
