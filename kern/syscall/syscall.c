/*
 * @file syscall.c
 * @brief Syscall functions
 * 
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 */

#include <seg.h>
#include <idt.h>
#include <syscall_int.h>
#include <interrupts.h>
#include <syshelper.h>
#include <inc/syscall.h>

int install_syscalls() {
	init_syscall_mutexes();

	trap_gate_t trap_gate;

	trap_gate.segment = SEGSEL_KERNEL_CS;
	trap_gate.privilege_level = 0x3;

	trap_gate.offset = (uint32_t) gettid_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), GETTID_INT);

	trap_gate.offset = (uint32_t) exec_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), EXEC_INT);

	trap_gate.offset = (uint32_t) fork_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), FORK_INT);

	trap_gate.offset = (uint32_t) yield_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), YIELD_INT);
	
	trap_gate.offset = (uint32_t) deschedule_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), DESCHEDULE_INT);
	
	trap_gate.offset = (uint32_t) make_runnable_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), MAKE_RUNNABLE_INT);
	
	trap_gate.offset = (uint32_t) sleep_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), SLEEP_INT);

	trap_gate.offset = (uint32_t) get_ticks_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), GET_TICKS_INT);
	
	trap_gate.offset = (uint32_t) set_status_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), SET_STATUS_INT);

	trap_gate.offset = (uint32_t) wait_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), WAIT_INT);

	trap_gate.offset = (uint32_t) vanish_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), VANISH_INT);

	trap_gate.offset = (uint32_t) new_pages_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), NEW_PAGES_INT);

	trap_gate.offset = (uint32_t) remove_pages_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), REMOVE_PAGES_INT);

	trap_gate.offset = (uint32_t) getchar_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), GETCHAR_INT);
	
	trap_gate.offset = (uint32_t) readline_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), READLINE_INT);
	
	trap_gate.offset = (uint32_t) print_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), PRINT_INT);
	
	trap_gate.offset = (uint32_t) set_term_color_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), SET_TERM_COLOR_INT);
	
	trap_gate.offset = (uint32_t) get_cursor_pos_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), GET_CURSOR_POS_INT);
	
	trap_gate.offset = (uint32_t) set_cursor_pos_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), SET_CURSOR_POS_INT);
	
	trap_gate.offset = (uint32_t) halt_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), HALT_INT);

	trap_gate.offset = (uint32_t) swexn_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), SWEXN_INT);

	trap_gate.offset = (uint32_t) thread_fork_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), THREAD_FORK_INT);

	trap_gate.offset = (uint32_t) readfile_int;
	insert_to_idt(create_trap_idt_entry(&trap_gate), READFILE_INT);

	return 0;
}
