/**
 * @file page.h
 * @brief Prototypes and constants for page.c
 * 
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 */
#ifndef __KERN_PAGE_H_
#define __KERN_PAGE_H_

#include <stdint.h>
#include <page_types.h>
#include <inc/process.h>		/* process_t */
#include <types.h>
#include <common_kern.h>

#define ADDR_MASK 0xFFFFF000
#define FLAGS_MASK 0xFFF
#define DIR_MASK 0xFFC00000
#define PAGE_MASK 0x003FF000
#define LOWER_MEM_SIZE (1 << 20)

#define PAGE_TABLE_ENTRIES (PAGE_SIZE/sizeof(pte_t))

#define PE_SETFLAG(pe, flag) ((pe) | (flag))
#define PE_UNSETFLAG(pe, flag) ((pe) & ~(flag))
#define PE_GETFLAG(pe, flag) (((pe) & FLAGS_MASK & (flag)) != 0)
#define PE_SETADDR(pe, addr) ((((vaddr_t) addr) & ADDR_MASK) | ((pe) & FLAGS_MASK))
#define PE_GETADDR(pe) ((pe) & ADDR_MASK)
#define PDE_OFFSET(va) (((va) & DIR_MASK) >> 22)
#define PAGE_ADDR(va) ((va) & ADDR_MASK)
#define PTE_OFFSET(va) (((va) & PAGE_MASK) >> 12)

#define FRAME_ID(pa) ((uint32_t)(pa) < USER_MEM_START ? -1 : ((uint32_t)(pa) - USER_MEM_START) >> 12)
#define FRAME_ADDR(id) (((id) << 12) + USER_MEM_START)

/* Kernel paging */
int install_paging(vm_size_t upper_mem);
int activate_paging(pde_t *cr3);
pde_t *init_paging(void);
void reset_paging(void);
int create_page(vaddr_t va, mem_type_t type, paddr_t ref_frame);
int destroy_page(vaddr_t va);
int destroy_paging(process_t *process);

/* Page faults */
void page_fault_handler(void);
void _page_fault_handler(void);

/* Frame management */
int init_frame_allocator(vm_size_t upper_mem);
paddr_t allocate_frame(void);
paddr_t _allocate_frame(void);
int get_frame(paddr_t frame);
int _get_frame(paddr_t frame);
int free_frame(paddr_t frame);
int copy_on_write(vaddr_t page);

/* Utils */
pte_t *get_pte(vaddr_t va, pde_t *cr3);
pde_t *get_pde(vaddr_t va, pde_t *cr3);
int copy_paging(process_t *parent, process_t *child);

#endif /* __KERN_PAGE_H_ */
