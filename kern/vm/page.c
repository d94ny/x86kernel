/**
 * @file page.c
 * @brief Virtual Memory management
 * 
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * @bugs No known bugs
 */

#include <page.h>
#include <interrupts.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <common_kern.h>
#include <x86/page.h>
#include <assert.h>
#include <asm.h>
#include <idt.h>
#include <seg.h>
#include <cr.h>
#include <simics.h>
#include <context.h>		/* context_switch */
#include <process.h>		/* process_t */
#include <errors.h>		/* ERR_* */
#include <launch.h>
#include <syshelper.h>
#include <ureg.h>
#include <lock.h>

/**
 * A blank frame, read-only, to implement ZFOD
 */
static paddr_t zero_frame = NULL;
/**
 * A buffer to copy frames in case copy on write can't be setup
 */
static uint32_t *frame_buffer = NULL;
/**
 * Mutex protecting the frame buffer
 */
static mutex_t frame_buffer_lock;

/**
 * @brief Installs paging for the system
 * 
 * This function sets up the paging infrastructure (the kernel paging 
 * system and the page fault handler). It has to be called only once, before
 * any call to init_paging and any switch to user space.
 * @return 0 if all went well, negative number if paging could not be 
 * enabled.
 */
int install_paging(vm_size_t upper_mem) {
	// Install exception handler
	trap_gate_t trap_gate;

	trap_gate.segment = SEGSEL_KERNEL_CS;
	trap_gate.offset = (uint32_t) page_fault_handler;
	trap_gate.privilege_level = 0x0;

	insert_to_idt(create_trap_idt_entry(&trap_gate), IDT_PF);

	// Create the zero frame
	zero_frame = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (zero_frame == NULL) return ERR_MALLOC_FAIL;
	memset(zero_frame, 0, PAGE_SIZE);

	// Create the frame buffer
	frame_buffer = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (frame_buffer == NULL) return ERR_MALLOC_FAIL;
	mutex_init(&frame_buffer_lock);

	// Initialize the frame allocator
	int err = init_frame_allocator(upper_mem);
	if (err) return err;

	return 0;
}

/**
 * @brief Initializes the page directory for the current process
 * 
 * This function has to be called after install_paging. It sets up a page
 * directory and page tables for the calling process. The kernel space is
 * direct-mapped, and the process begins with no user space pages allocated.
 * In fact, this function is called only once, to setup the god process paging.
 * Once this is done, new processes are created with the fork system call,
 * which creates the page tables with copy_paging()
 * @return the address of the created page directory
 */
pde_t *init_paging() {
	/* Page directory */

	// Allocate
	pde_t* cr3 = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (cr3 == NULL) return NULL;
	
	// Initialize
	int i;
	for (i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
		boolean_t has_user;
		pde_t pde = 0;
		if (i*PAGE_TABLE_ENTRIES*PAGE_SIZE < USER_MEM_START) {
			// If the page directory has pages in kernel space
			pde = PE_SETFLAG(pde, PDE_PRESENT);
			pde = PE_SETFLAG(pde, PDE_KERNEL);

			// Allocate page table
			pte_t *pt = smemalign(PAGE_SIZE, PAGE_SIZE);
			if (pt == NULL) return NULL;

			// Create all page entries
			int j;
			for (j = 0; j < PAGE_TABLE_ENTRIES; ++j) {
				if ((i*PAGE_TABLE_ENTRIES + j)*PAGE_SIZE >= USER_MEM_START) {
					// We've reached the end of kernel 
					// space
					has_user = TRUE;
					pde = PE_SETFLAG(pde, PDE_USER);
					// Fill all remaining PTEs with 0
					memset(pt + j, 0, PAGE_TABLE_ENTRIES - j);
					break;
				}

				// Create page table entry
				pte_t pte = 0;
				pte = PE_SETFLAG(pte, PTE_PRESENT);
				pte = PE_SETFLAG(pte, PTE_READWRITE);
				pte = PE_SETFLAG(pte, PTE_GLOBAL);
				pte = PE_SETADDR(pte, (i*PAGE_TABLE_ENTRIES + j)*PAGE_SIZE);
				pt[j] = pte;
			}

			// Link the page table to the page directory entry
			pde = PE_SETADDR(pde, pt);
		}

		// Write the entry to the page directory
		cr3[i] = pde;
	}

	// Map the zero page
	pte_t *zero_pte = get_pte((vaddr_t) zero_frame, cr3);
	if (zero_pte == NULL) {
		/*
		 * Either smemalign returned an address out of the kernel, or we
		 * fucked up the page table, or something even crazier happened, which
		 * can't be good...
		 */
		 kernel_panic("Incoherent zero frame");
	}
	
	/*
	 * Make it unwritable
	 */
	*zero_pte = PE_UNSETFLAG(*zero_pte, PTE_READWRITE);

	// Return the page directory
	return cr3;
}

/**
 * @brief Resets the page directory to its initial state
 * 
 * Deletes all user-related page table entries in the paging system of the
 * current process. All kernel pages remain mapped in the page tables.
 */
void reset_paging() {
	// Get the page directory
	pde_t *cr3 = (pde_t *) get_cr3();
	if (cr3 == NULL) {
		panic("No page directory registered for thread %d", get_self()->tid);
	}

	// Read across all page tables
	int i, j;
	for (i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
		// If there is no page table attached, our work is done
		if (!PE_GETFLAG(cr3[i], PDE_PRESENT)) continue;

		// Get the page table
		pte_t *pt = (pte_t *) PE_GETADDR(cr3[i]);
		
		if (PE_GETFLAG(cr3[i], PDE_USER)) {


			for (j = 0; j < PAGE_TABLE_ENTRIES; ++j) {
				pte_t pte = pt[j];
				if (!PE_GETFLAG(pte, PTE_PRESENT)) continue;
				if (PE_GETFLAG(pte, PTE_GLOBAL)) continue;
				if (!PE_GETFLAG(pte, PTE_USER)) continue;
				if (PE_GETFLAG(pte, PTE_ZEROPAGE)) continue;


				int err = free_frame((paddr_t) PE_GETADDR(pte));
				if (err) {
					lprintf("free frame %lx failed", PE_GETADDR(pte));
					MAGIC_BREAK;
				}
				pt[j] = 0;
			}
			if (!PE_GETFLAG(cr3[i], PDE_KERNEL)) {
				cr3[i] = 0;
				//sfree(pt, PAGE_SIZE);
			}

		}
	}

	// We flush the TLB by setting cr3 to itself
	set_cr3((uint32_t)cr3);
}

/**
 * @brief Creates a page mapping
 * 
 * This creates a mapping between a given virtual address and a physical 
 * frame.
 * The mapping can have multiple forms:
 * - If type is read-only, and a reference frame is specified, the virtual
 * address is directly mapped to the reference frame.
 * - If type is read-write, and a reference frame is specified, the 
 *   virtual
 * address is mapped to the reference frame via copy-on-write
 * - If type is BSS, the reference frame is ignored and the virtual 
 *   address is
 * mapped to the zero frame with copy on write (implied by the zero frame 
 * bit)
 * - If no reference frame is specified, the virtual address is mapped to 
 *   a
 * newly allocated frame, initialized to zero.
 * 
 * @param va the virtual address to map
 * @param type the type of memory one wants to map
 * @param ref_frame the reference frame for implementing copy-on-write
 * @return 0 if success, and a negative error code othwewise
 */
int create_page(vaddr_t va, mem_type_t type, paddr_t ref_frame) {
	// Argument check
	if (va % PAGE_SIZE != 0) return ERR_INVALID_ARG;
	if (va < USER_MEM_START) return ERR_INVALID_ARG;
	if (ref_frame != NULL) {
		if ((uint32_t) ref_frame % PAGE_SIZE != 0) return ERR_INVALID_ARG;
		if ((uint32_t) ref_frame < USER_MEM_START) return ERR_INVALID_ARG;
	}

	// Try to allocate new frame if necessary, to revert errors easily
	paddr_t new_frame = NULL;
	if (type != MEM_TYPE_BSS && !ref_frame) {
		new_frame = allocate_frame();
		if (new_frame == NULL) {
			return ERR_NO_FRAMES;
		}
	}

	pde_t *cr3 = (pde_t *) get_cr3();
	pte_t* pte = get_pte(va, cr3);
	if (pte == NULL) {
		// The page table has not been created

		pde_t *pde = get_pde(va, cr3);
		// Create page table
		pte_t *pt = smemalign(PAGE_SIZE, PAGE_SIZE);
		if (pt == NULL) {
			int err = free_frame(new_frame);
			if (err) {
				panic("Couldn't free previously allocated copy frame");
			}
			return ERR_MALLOC_FAIL;
		}
		memset(pt, 0, PAGE_SIZE);

		// Set flags
		*pde = PE_SETFLAG(*pde, PDE_PRESENT);
		*pde = PE_SETFLAG(*pde, PDE_READWRITE);
		*pde = PE_SETFLAG(*pde, PDE_USER);
		*pde = PE_SETADDR(*pde, pt);

		// Set all entries to 0
		memset(pt, 0, PAGE_TABLE_ENTRIES);
		pte = &pt[PTE_OFFSET(va)];
	} else {
		// We only need this check if the page table was already 
		// there
		if (PE_GETFLAG(*pte, PTE_PRESENT)) {
			int err = free_frame(new_frame);
			if (err) {
				panic("Couldn't free previously allocated copy frame");
			}
			return ERR_PAGE_ALREADY_PRESENT;
		}
	}

	// Create the page table entry
	*pte = PE_SETFLAG(*pte, PTE_PRESENT);
	*pte = PE_SETFLAG(*pte, PTE_USER);
	if (type == MEM_TYPE_BSS) {
		// Setup zero-fill on demand
		*pte = PE_SETFLAG(*pte, PTE_ZEROPAGE);
		*pte = PE_SETADDR(*pte, zero_frame);
	} else if (ref_frame) {
		// Setup copy on write
		*pte = PE_SETFLAG(*pte, PTE_COPYONWRITE);
		*pte = PE_SETADDR(*pte, ref_frame);
	} else {
		// Setup new frame
		*pte = PE_SETADDR(*pte, new_frame);
	}

	// Type-specific adjustments
	switch (type) {
		case MEM_TYPE_TEXT:
		case MEM_TYPE_RODATA:
			// Read-only types
			break;
		case MEM_TYPE_DATA:
		case MEM_TYPE_HEAP:
		case MEM_TYPE_STACK:
		case MEM_TYPE_BSS:
		case MEM_TYPE_USER:
			// Read-write types
			*pte = PE_SETFLAG(*pte, PTE_READWRITE);
			break;
		default:
			assert(FALSE);
	}

	// Page successfuly allocated, we're good
	return 0;
}

/**
 * @brief Removes the given page from the process' page table
 * 
 * This functions updates the corresponding page table entry and flushes the
 * TLB to avoid inconsistencies.
 * @return 0 on success, negative number on error
 */
int destroy_page(vaddr_t va) {
	if (va % PAGE_SIZE != 0) return ERR_INVALID_ARG;

	pde_t *cr3 = (pde_t *)get_cr3();
	if (cr3 == NULL) {
		panic("No page directory registered for thread %d", get_self()->tid);
	}
	pte_t *pte = get_pte(va, cr3);
	if (pte == NULL) return ERR_DIRECTORY_NOT_PRESENT;

	if (!PE_GETFLAG(*pte, PTE_PRESENT)) return ERR_PAGE_NOT_PRESENT;
	if (PE_GETFLAG(*pte, PTE_GLOBAL)) return ERR_KERNEL_PAGE;
	if (!PE_GETFLAG(*pte, PTE_USER)) return ERR_KERNEL_PAGE;

	paddr_t frame = (paddr_t) PE_GETADDR(*pte);
	*pte = 0;
	set_cr3(get_cr3());

	int err = free_frame(frame);
	if (err) panic("Frame allocator coherence error");

	return 0;
}

/**
 * @brief Handles page faults in the program
 * 
 * When the program encounters a page fault, multiple outcomes are possible:
 * 1. We tried to write to the zero page -> Zero fill
 * 2. We tried to write to a copy-on-write page -> Copy
 * 3. It is a normal page fault, call the user swexn handler or, if there is
 * none, panic.
 */
void _page_fault_handler() {
	// The faulting address
	vaddr_t addr = get_cr2();

	pde_t *cr3 = (pde_t *)get_cr3();
	if (cr3 == NULL) {
		panic("No page directory registered for thread %d", get_self()->tid);
	}
	pte_t *pte = get_pte(addr, cr3);
	
	if (pte != NULL) {
		// The page table exists, we might be able to do something
		if (PE_GETFLAG(*pte, PTE_ZEROPAGE)) { // It's the zero page
			paddr_t frame = allocate_frame();
			if (frame != NULL) {
				*pte = PE_UNSETFLAG(*pte, PTE_ZEROPAGE);
				*pte = PE_SETFLAG(*pte, PTE_READWRITE);
				*pte = PE_SETADDR(*pte, frame);
				set_cr3(get_cr3());
				memset((void*) PAGE_ADDR(addr), 0, PAGE_SIZE);
				return;
			}
		} else if (PE_GETFLAG(*pte, PTE_COPYONWRITE)) {
			*pte = PE_UNSETFLAG(*pte, PTE_COPYONWRITE);
			*pte = PE_SETFLAG(*pte, PTE_READWRITE);
			copy_on_write(PAGE_ADDR(addr));
			return;
		}
	}

	thread_t *thread = get_self();
	if (thread->swexn_eip != 0x0 && thread->swexn_esp != 0x0) {
		vaddr_t eip = thread->swexn_eip;
		vaddr_t esp3 = thread->swexn_esp;
		void *arg = thread->swexn_arg;

		// Setup the stack for the handler
		if (check_array((void*) esp3,
				(sizeof(ureg_t) + 3 * sizeof(uint32_t)) / sizeof(void*))) {
			// Unregister the exception handler
			thread->swexn_eip = 0x0;
			thread->swexn_esp = 0x0;
			thread->swexn_arg = NULL;

			ureg_t *ureg = (ureg_t *) (esp3 - sizeof(ureg_t));

			uint32_t *esp0 = (uint32_t *) thread->esp0;

			// Cause and cr2
			ureg->cause = SWEXN_CAUSE_PAGEFAULT;
			ureg->cr2 = addr;

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
			void **argbase = (void *) 
				(esp3 - sizeof(ureg_t) - 2 * sizeof(uint32_t));
			*(argbase - 1) = 0x0;
			*(argbase) = arg;
			*(argbase + 1) = ureg;

			// Transfer execution to the handler, coming back to user space
			launch(eip, (uint32_t) (argbase - 1));
		}
	}

	// Nothing could be done, we lost the thread, :'(
	panic("Page fault at address 0x%lx", addr);
}

/**
 * @brief Returns the address of the page table entry for the given address
 * 
 * @param cr3 the page directory of the applicable process
 * @return the entry address if it exists, NULL otherwise
 */
pte_t *get_pte(vaddr_t va, pde_t *cr3) {
	pde_t *pde = get_pde(va, cr3);
	if (PE_GETFLAG((uint32_t) *pde, PDE_PRESENT)) {
		return &((pte_t *)PE_GETADDR(*pde))[PTE_OFFSET(va)];
	}
	return NULL;
}

/**
 * @brief Returns the page directory entry for the page table of the given
 * address
 */
pde_t *get_pde(vaddr_t va, pde_t *cr3) {
	return &cr3[PDE_OFFSET(va)];
}

/**
 * @brief Copies the memory regions of a process to another
 *
 * This method is called by the copy_process() function which is used by
 * the fork system call. The idea is pretty simple : we simply fill in
 * the page tables with the same entries as the old page tables, but with
 * the COPYONWRITE flag on both the parent and the child.
 * 
 * Note on errors: when one occurs after some of the child's page tables have
 * been allocated, the function's caller has to destroy the paging system for
 * the child, but not to touch the one of the parent, even though some copy on
 * write flags have been set. This is to avoid a costly manipulation, and this
 * doesn't induce corectness flaws in the kernel. As the frame count will
 * still be consistent, the next access on the parent's page, if any, will
 * simply remove the copy on write flag if the process was the sole holder of
 * the frame.
 *
 * @param parent the task whose memory regions we copy
 * @param child the task we copy the memory to
 * @return 0 on sucess, a negative error code otherwise
 */
int copy_paging(process_t *parent, process_t *child) {
	
	// Verify data
	if (parent == NULL) return ERR_ARG_NULL;
	if (child == NULL) return ERR_ARG_NULL;

	// Get the two cr3's
	pde_t *pcr3 = parent->cr3;
	pde_t *ccr3 = child->cr3; 

	// Iterate over the page directory
	int pd_index = 0;
	int pt_index = 0;
	for (pd_index = 0; pd_index < PAGE_TABLE_ENTRIES; pd_index++) {

		// Check if we have a page table for that entry
		if (!PE_GETFLAG(pcr3[pd_index], PDE_PRESENT)) continue;

		// Check if the page is for the user space
		if (!PE_GETFLAG(pcr3[pd_index], PDE_USER)) continue;

		// If there is a page table we get it
		pte_t *pt = (pte_t *) PE_GETADDR(pcr3[pd_index]);
		
		// Then we create a page table for the child
		pte_t *ptc = smemalign(PAGE_SIZE, PAGE_SIZE);
		if (ptc == NULL) {
			// The parent will destroy the paging
			return ERR_MALLOC_FAIL;
		}
		memset(ptc, 0, PAGE_SIZE);
		
		// Set our page directory entry to present
		ccr3[pd_index] = PE_SETFLAG(ccr3[pd_index], PDE_PRESENT);
		ccr3[pd_index] = PE_SETFLAG(ccr3[pd_index], PDE_READWRITE);
		ccr3[pd_index] = PE_SETFLAG(ccr3[pd_index], PDE_USER);

		// And point the pde to the new page table
		ccr3[pd_index] = PE_SETADDR(ccr3[pd_index], ptc);
	
		// Iterate over all pages in that page table
		for (pt_index = 0; pt_index < PAGE_TABLE_ENTRIES;
				pt_index++, pt++, ptc++) {

			if (!PE_GETFLAG(*pt, PTE_PRESENT)) continue;
			if (!PE_GETFLAG(*pt, PTE_USER)) continue;

			*ptc = *pt;
	
			/**
			 * Increment the number of pages linked to that 
			 * given frame. This will then be used by the copy
			 * on write mecanism to determine if a personal 
			 * copy of the frame is really necessary or if 
			 * every other thread has already copied it.
			 */
			paddr_t frame = (paddr_t) PE_GETADDR(*ptc);
			int err = get_frame(frame);

			if (err == ERR_KERNEL_FRAME) continue; 
			if (err == 0) {
				/**
				 * We set the flag of both the parent and 
				 * child to copy on write so that the 
				 * first thread to write to that page gets 
				 * a personal copy of the frame.
				 */
				if (PE_GETFLAG(*pt, PTE_READWRITE)) {
					// Only if the page is not read-only
					*ptc = PE_SETFLAG(*ptc, PTE_COPYONWRITE);
					*ptc = PE_UNSETFLAG(*ptc, PTE_READWRITE);
					*pt = PE_SETFLAG(*pt, PTE_COPYONWRITE);
					*pt = PE_UNSETFLAG(*pt, PTE_READWRITE);
				}
				continue;
			} else if (err == ERR_TOO_MANY_FRAME_OWNERS) {	
				/**
				 * If we couldn't get that frame because it has 
				 * too many owners already we need to create a 
				 * seperate frame.
				 */
				paddr_t pa = allocate_frame();
				if (pa) {
					*ptc = PE_SETADDR(*ptc, pa);
					*ptc = PE_SETFLAG(*ptc, PTE_PRESENT);
					*ptc = PE_SETFLAG(*ptc, PTE_READWRITE);
					*ptc = PE_SETFLAG(*ptc, PTE_USER);
					*ptc = PE_UNSETFLAG(*ptc, PTE_COPYONWRITE);
					/**
					 * Now we need to copy the old frame into 
					 * the onw we just allocated.
					 *
					 * Note that we have the cr3 of the 
					 * parent.
					 */
					vaddr_t va = pd_index << 22;  	
					va += (pt_index & 0x3FF) << 12;

					mutex_lock(&frame_buffer_lock);
					memcpy(frame_buffer, (void*)va, PAGE_SIZE);
					*pt = PE_SETADDR(*pt, pa);
					memcpy((void*)va, frame_buffer, PAGE_SIZE);
					mutex_unlock(&frame_buffer_lock);

					// Restore the parents frame
					*pt = PE_SETADDR(*pt, frame);	
				}
			} else {
				// We are out of luck, everything failed !
				destroy_paging(child);
				return err;
			}
		}
	}

	// Flush the TLB
	set_cr3(get_cr3());

	return 0;
}

/**
 * @brief Destroys all page tables and the page directory of a process
 *
 * @param process the process whose pages to destroy
 * @return 0 on sucess, a negative error code otherwise
 */
int destroy_paging(process_t *process) {
	// Iterate over the page directory
	// Note : don't use destroy_page which is not generic
	int pd_index = 0;
	int pt_index = 0;
	pde_t *cr3 = process->cr3;
	for (pd_index = 0; pd_index < PAGE_TABLE_ENTRIES; pd_index++) {
	
		// Check if we have a page table for that entry
		if (!PE_GETFLAG(cr3[pd_index], PDE_PRESENT)) continue;
	
		// If there is a page table we get it
		pte_t *pt = (pte_t *) PE_GETADDR(cr3[pd_index]);

		// Iterate over all pages of the page table
		for (pt_index = 0; pt_index < PAGE_TABLE_ENTRIES; 
			pt_index++, pt++) {
	
			if (PE_GETFLAG(*pt, PTE_ZEROPAGE)) continue;	
			if (!PE_GETFLAG(*pt, PTE_PRESENT)) continue;
			if (PE_GETFLAG(*pt, PTE_GLOBAL)) continue;
			if (!PE_GETFLAG(*pt, PTE_USER)) continue;

			// Free the frame
			paddr_t frame = (paddr_t) PE_GETADDR(*pt);
			*pt = 0;
			int err = free_frame(frame);
			if (err < 0 && err != ERR_KERNEL_FRAME) {
				panic("Frame allocator coherence error %d", err);			
			}
		}

		// Now free the page table
		sfree((void *)PE_GETADDR(cr3[pd_index]), PAGE_SIZE);
	}

	// Finally free the page directory
	sfree((void *)cr3, PAGE_SIZE);
	return 0;
}
