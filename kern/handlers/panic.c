/**
 * @file panic.c
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * 
 * The file defines two panic functions, used respecitvely when a thread has
 * been corrupted in such a way that its execution cannot be resumed
 * correctly, and when the kernel internal data structures are corrupted,
 * which means that no thread is safe to run anymore.
 */

#include <thread.h>
#include <context.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <simics.h>
#include <asm.h>

#include <inc/syscall.h>

/**
 * @brief Panic function
 */
void panic(const char *fmt, ...) {
	// The killed thread might have disabled interrupts
	enable_interrupts();

	/* Print the message */	
	va_list vl;
	char buf[80];

	va_start(vl, fmt);
	vsnprintf(buf, sizeof (buf), fmt, vl);
	va_end(vl);
	lprintf(buf);

	va_start(vl, fmt);
	vprintf(fmt, vl);
	va_end(vl);

	printf("\n");

	/* Exit the thread */
	_set_status(-2);
	_vanish();
	/* NEVER RETURNS */
}

/**
 * @brief Kernel panic function
 * 
 * Stops the kernel when an important inconsistency is found within the
 * kernel, making its proper execution impossible
 */
void kernel_panic(const char *fmt, ...) {
	/* Print the message */	
	va_list vl;
	char buf[80];

	va_start(vl, fmt);
	vsnprintf(buf, sizeof (buf), fmt, vl);
	va_end(vl);
	lprintf(buf);

	va_start(vl, fmt);
	vprintf(fmt, vl);
	va_end(vl);

	printf("\n");

	// Stop the kernel, but allow access to the debugging information
	MAGIC_BREAK;
	_halt();
}
