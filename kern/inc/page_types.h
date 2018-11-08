/**
 * @file page_types.h
 * @brief Defines types used for paging
 * 
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 */
#ifndef __KERN_PAGE_TYPES_H_
#define __KERN_PAGE_TYPES_H_

#include <types.h>
#include <stdint.h>

typedef void* paddr_t;
typedef uint32_t vaddr_t;
typedef uint32_t pte_t;
typedef uint32_t pde_t;

typedef enum {
	MEM_TYPE_TEXT,
	MEM_TYPE_RODATA,
	MEM_TYPE_DATA,
	MEM_TYPE_BSS,
	MEM_TYPE_HEAP,
	MEM_TYPE_STACK,
	MEM_TYPE_USER,
} mem_type_t;

typedef enum {
	PDE_PRESENT			= (1 << 0),
	PDE_READWRITE		= (1 << 1),
	PDE_USER			= (1 << 2),
	PDE_WRITETHROUGH	= (1 << 3),
	PDE_NOCACHE			= (1 << 4),
	PDE_ACCESSED		= (1 << 5),
	PDE_PAGESIZE		= (1 << 7),
	PDE_KERNEL			= (1 << 9) 		// The page table contains kernel space
} pde_flag_t;

typedef enum {
	PTE_PRESENT			= (1 << 0),
	PTE_READWRITE		= (1 << 1),
	PTE_USER			= (1 << 2),
	PTE_WRITETHROUGH	= (1 << 3),
	PTE_NOCACHE			= (1 << 4),
	PTE_ACCESSED		= (1 << 5),
	PTE_DIRTY			= (1 << 6),
	PTE_PAGESIZE		= (1 << 7),
	PTE_GLOBAL			= (1 << 8),
	PTE_ZEROPAGE		= (1 << 9), 	// The page is only zeros
	PTE_COPYONWRITE		= (1 << 10),	// The page is the child of a copy on write
} pte_flag_t;

#endif /* !__KERN_PAGE_TYPES_H_ */
