/**
 * @file interrupts.h
 * @brief Function declarations and types for interrupts.c
 * 
 * @author Loic Ottet (lottet)
 * @bugs No known bugs
 */

#ifndef _KERN_INTERRUPTS_H_
#define _KERN_INTERRUPTS_H_

#include <inc/stdint.h>

typedef struct {
	uint16_t segment;			// The segment selector
	uint32_t offset;			// The offset (address) of the handler function
	uint8_t privilege_level;	// The needed privilege level (2 bits)
} trap_gate_t;

void insert_to_idt(uint64_t gate, unsigned int index);
uint64_t create_trap_idt_entry(const trap_gate_t* gate);
uint64_t create_interrupt_idt_entry(const trap_gate_t* gate);

// Exception handlers
void install_exceptions(void);

void _exception_handler(uint32_t cause, uint32_t error_code);
void divide_handler(void);
void debug_handler(void);
void breakpoint_handler(void);
void overflow_handler(void);
void boundcheck_handler(void);
void opcode_handler(void);
void nofpu_handler(void);
void segfault_handler(void);
void stackfault_handler(void);
void protfault_handler(void);
void fpufault_handler(void);
void alignfault_handler(void);
void smidfault_handler(void);

#endif /* _KERN_INTERRUPTS_H_ */