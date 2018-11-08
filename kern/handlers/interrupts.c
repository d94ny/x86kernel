/**
 * @file interrupts.c
 * @brief General function for interrupt handlers
 * 
 * This file contains the functions needed by all interrupt handlers, to set
 * up their IDT entry and then acknowledge they have handled their interrupt
 * when they are called.
 * 
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * @bugs No check for IDT size when adding a gate
 */

#include <inc/stdint.h>
#include <x86/asm.h>
#include <x86/interrupt_defines.h>

#include <interrupts.h>

/**
 * @brief Inserts the given gate to the IDT
 * 
 * @param gate the 64 bits of the gate
 * @param index the index of the entry in the IDT
 * @return Void.
 */
void insert_to_idt(uint64_t gate, unsigned int index) {
	// Store the bytes
	uint64_t* idt = idt_base();
	idt[index] = gate;
}

/**
 * @brief creates a trap IDT entry (64 bits) from a trap gate
 */
uint64_t create_trap_idt_entry(const trap_gate_t* gate) {
	// Construct the gate parts
	uint16_t offset_msb = (uint16_t) (gate->offset >> 16);
	uint16_t offset_lsb = (uint16_t) (gate->offset & 0xFFFF);
	uint8_t privilege_level = gate->privilege_level & 0x3;
	uint16_t segment_selector = gate->segment;

	// Construct the gate
	uint64_t entry = 0x0;
	entry |= ((uint64_t) offset_msb) << 48;			// Bits 48-63
	entry |= ((uint64_t) 0x1) << 47;				// Bit  47
	entry |= ((uint64_t) privilege_level) << 45;	// Bits 45-46
	entry |= ((uint64_t) 0xF00) << 32;				// Bits 32-43
	entry |= ((uint64_t) segment_selector) << 16;	// Bits 16-31
	entry |= ((uint64_t) offset_lsb);				// Bits  0-15

	return entry;
}

/**
 * @brief creates an interrupt IDT entry (64 bits) from a trap gate
 */
uint64_t create_interrupt_idt_entry(const trap_gate_t* gate) {
	// Construct the gate parts
	uint16_t offset_msb = (uint16_t) (gate->offset >> 16);
	uint16_t offset_lsb = (uint16_t) (gate->offset & 0xFFFF);
	uint8_t privilege_level = gate->privilege_level & 0x3;
	uint16_t segment_selector = gate->segment;

	// Construct the gate
	uint64_t entry = 0x0;
	entry |= ((uint64_t) offset_msb) << 48;			// Bits 48-63
	entry |= ((uint64_t) 0x1) << 47;				// Bit  47
	entry |= ((uint64_t) privilege_level) << 45;	// Bits 45-46
	entry |= ((uint64_t) 0xE00) << 32;				// Bits 32-43
	entry |= ((uint64_t) segment_selector) << 16;	// Bits 16-31
	entry |= ((uint64_t) offset_lsb);				// Bits  0-15

	return entry;
}
