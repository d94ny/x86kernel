/**
 * @file exception.c
 * @brief Exception handler for all exceptions except page faults
 * 
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * @bugs No known bugs
 */

#include <stdlib.h>

#include <idt.h>
#include <seg.h>
#include <types.h>
#include <ureg.h>

#include <errors.h>
#include <interrupts.h>
#include <launch.h>
#include <thread.h>

/**
 * @brief Installs the exception handlers in the idt
 */
void install_exceptions() {
	trap_gate_t trap_gate;

	trap_gate.segment = SEGSEL_KERNEL_CS;
	trap_gate.privilege_level = 0x0;

	trap_gate.offset = (uint32_t) divide_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_DE);

	trap_gate.offset = (uint32_t) debug_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_DB);

	trap_gate.offset = (uint32_t) breakpoint_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_BP);

	trap_gate.offset = (uint32_t) overflow_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_OF);

	trap_gate.offset = (uint32_t) boundcheck_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_BR);

	trap_gate.offset = (uint32_t) opcode_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_UD);

	trap_gate.offset = (uint32_t) nofpu_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_NM);

	trap_gate.offset = (uint32_t) segfault_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_NP);

	trap_gate.offset = (uint32_t) stackfault_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_SS);

	trap_gate.offset = (uint32_t) protfault_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_GP);

	trap_gate.offset = (uint32_t) fpufault_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_MF);

	trap_gate.offset = (uint32_t) alignfault_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_AC);

	trap_gate.offset = (uint32_t) smidfault_handler;
	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_XF);
}

/**
 * @brief Generic handler for non-page fault exceptions
 * 
 * This handler checks whether the fault/exception was caused by user code, in
 * which case it calls the swexn handler of the user, if it has one
 * registered, or kill the faulting thread.
 * In the other cases the kernel has caused the fault, which means something
 * is very wrong with the internal state of the operating system. In the case
 * such a thing happens we shut down the kernel immediately.
 * 
 * @param cause the type of exception triggered
 * @param error_code information about the faulting segment
 */
void _exception_handler(uint32_t cause, uint32_t error_code) {
	// Check if we entered the kernel because of the fault
	uint32_t seg_id = (error_code & 0xFFF8) >> 3;

	thread_t *thread = get_self();

	if (seg_id == SEGSEL_USER_CS_IDX || seg_id == SEGSEL_USER_DS_IDX) {
		// The fault was caused by the user
		// We give him a chance to solve it with his swexn handler
		if (thread->swexn_eip != 0x0 && thread->swexn_esp != 0x0) {
			vaddr_t eip = thread->swexn_eip;
			vaddr_t esp3 = thread->swexn_esp;
			void *arg = thread->swexn_arg;

			// Unregister the exception handler
			thread->swexn_eip = 0x0;
			thread->swexn_esp = 0x0;
			thread->swexn_arg = NULL;

			// Setup the stack for the handler
			//if (!check_region(esp3, sizeof(ureg_t + 3 * sizeof(uint32_t)))) break;
			ureg_t *ureg = (ureg_t *) (esp3 - sizeof(ureg_t));

			uint32_t *esp0 = (uint32_t *) thread->esp0;

			// Cause
			ureg->cause = cause;

			// Code segments
			ureg->ds = *(esp0 - 7);
			ureg->es = *(esp0 - 8);
			ureg->fs = *(esp0 - 9);
			ureg->gs = *(esp0 - 10);

			// General purpose registers
			ureg->eax = *(esp0 - 11);
			ureg->ecx = *(esp0 - 12);
			ureg->edx = *(esp0 - 13);
			ureg->ebx = *(esp0 - 14);
			ureg->zero = 0;
			ureg->ebp = *(esp0 - 16);
			ureg->esi = *(esp0 - 17);
			ureg->edi = *(esp0 - 18);

			// Special registers
			ureg->error_code = *(esp0 - 6);
			ureg->eip = *(esp0 - 5);
			ureg->cs = *(esp0 - 4);
			ureg->eflags = *(esp0 - 3);
			ureg->esp = *(esp0 - 2);
			ureg->ss = *(esp0 - 1);

			// Setup arguments
			void **argbase = (void *) (esp3 - sizeof(ureg_t) - 2 * sizeof(uint32_t));
			*(argbase - 1) = 0x0;
			*(argbase) = arg;
			*(argbase + 1) = ureg;

			// Transfer execution to the handler, coming back to user space
			launch(eip, (uint32_t) (argbase - 1));
		}

		// The user caused the fault but has no swexn handler registered
		int tid = thread->tid;
		switch(cause) {
			case SWEXN_CAUSE_DIVIDE:
				panic("Exception in thread %d: Divide by zero", tid);
				break;
			case SWEXN_CAUSE_OVERFLOW:
				panic("Exception in thread %d: Overflow exception", tid);
				break;
			case SWEXN_CAUSE_BOUNDCHECK:
				panic("Exception in thread %d: Bound check exception", tid);
				break;
			case SWEXN_CAUSE_OPCODE:
				panic("Exception in thread %d: Bad opcode exception", tid);
				break;
			case SWEXN_CAUSE_NOFPU:
				panic("Exception in thread %d: No FPU present", tid);
				break;
			case SWEXN_CAUSE_SEGFAULT:
				panic("Exception in thread %d: Segmentation fault", tid);
				break;
			case SWEXN_CAUSE_STACKFAULT:
				panic("Exception in thread %d: Stack fault", tid);
				break;
			case SWEXN_CAUSE_PROTFAULT:
				panic("Exception in thread %d: Protection fault", tid);
				break;
			case SWEXN_CAUSE_PAGEFAULT:
				panic("Exception in thread %d: Page fault", tid);
				break;
			case SWEXN_CAUSE_FPUFAULT:
				panic("Exception in thread %d: FPU Fault", tid);
				break;
			case SWEXN_CAUSE_ALIGNFAULT:
				panic("Exception in thread %d: Alignment fault", tid);
				break;
			case SWEXN_CAUSE_SIMDFAULT:
				panic("Exception in thread %d: SIMD Fault", tid);
				break;
			default:
				panic("Unknown exception in thread %d.", tid);
		}
	}

	// The fault occured in the kernel, or the user has not registered a handler
	int tid = thread->tid;
	switch(cause) {
		case SWEXN_CAUSE_DIVIDE:
			kernel_panic("Exception in thread %d: Divide by zero", tid);
			break;
		case SWEXN_CAUSE_OVERFLOW:
			kernel_panic("Exception in thread %d: Overflow exception", tid);
			break;
		case SWEXN_CAUSE_BOUNDCHECK:
			kernel_panic("Exception in thread %d: Bound check exception", tid);
			break;
		case SWEXN_CAUSE_OPCODE:
			kernel_panic("Exception in thread %d: Bad opcode exception", tid);
			break;
		case SWEXN_CAUSE_NOFPU:
			kernel_panic("Exception in thread %d: No FPU present", tid);
			break;
		case SWEXN_CAUSE_SEGFAULT:
			kernel_panic("Exception in thread %d: Segmentation fault", tid);
			break;
		case SWEXN_CAUSE_STACKFAULT:
			kernel_panic("Exception in thread %d: Stack fault", tid);
			break;
		case SWEXN_CAUSE_PROTFAULT:
			kernel_panic("Exception in thread %d: Protection fault", tid);
			break;
		case SWEXN_CAUSE_PAGEFAULT:
			kernel_panic("Exception in thread %d: Page fault", tid);
			break;
		case SWEXN_CAUSE_FPUFAULT:
			kernel_panic("Exception in thread %d: FPU Fault", tid);
			break;
		case SWEXN_CAUSE_ALIGNFAULT:
			kernel_panic("Exception in thread %d: Alignment fault", tid);
			break;
		case SWEXN_CAUSE_SIMDFAULT:
			kernel_panic("Exception in thread %d: SIMD Fault", tid);
			break;
		default:
			kernel_panic("Unknown exception in thread %d.", tid);
	}
}