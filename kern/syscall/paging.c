/**
 * @file paging.c
 * @brief User space memory management
 * 
 * This file contains the new_pages and remove_pages system calls, essential
 * for memory allocation in user space. These system calls operate using the
 * memregions table in the process descriptor table. This table is a map from
 * virtual addresses to the number of frames that were allocated by new_pages
 * for that address, and allow remove_pages to effectively remove the actual
 * number of allocated pages without having to get a size argument when given
 * an address.
 */

#include <stdlib.h>
#include <string.h>
#include <simics.h>
#include <inc/syscall.h>
#include <inc/page.h>
#include <x86/page.h>
#include <syshelper.h>
#include <errors.h>

/**
 * @brief Allocates len bytes (page-aligned) from base (page-aligned)
 * 
 * The function allocates the needed pages and registers the tuple (base, len)
 * in the memregions table for later deallocation. It strives to ensure that
 * whenever the system call fails, everything that was done up to that point
 * is reverted to its state at the time of the call.
 */
int _new_pages(int* args) {
	if (!check_array(args, 2)) return ERR_INVALID_ARG;
	vaddr_t base = args[0];
	int len = args[1];

	if (base % PAGE_SIZE != 0) return ERR_INVALID_ARG;
	if (len < 0 || len % PAGE_SIZE != 0 || len > 0xfff * PAGE_SIZE)
		return ERR_INVALID_ARG;

	process_t *process = get_self()->process;
	if (process == NULL) kernel_panic("Unregistered thread");
	if (process->next_memregion_idx == -1) return ERR_WORN_OUT_NEW_PAGES;

	// Number of pages to allocate
	int num_pages = len / PAGE_SIZE;

	int i;
	for (i = 0; i < num_pages; ++i) {
		int err = create_page(base + i * PAGE_SIZE, MEM_TYPE_USER, NULL);
		if (err) {
			// Roll back
			int j;
			for (j = 0; j < i; ++j) {
				int err = destroy_page(base + j * PAGE_SIZE);
				if (err) kernel_panic(
					"Unable to destroy previously allocated page!");
			}
			return err;
		}
	}

	// Zero out the page
	memset((void *)base, 0, len);

	// Memory region tracking
	uint32_t entry = base | num_pages;
	process->memregions[process->next_memregion_idx] = entry;

	// Find and set the next available spot
	int nextid = (process->next_memregion_idx + 1) % PAGE_TABLE_ENTRIES;
	for(;nextid != process->next_memregion_idx;
			nextid = (nextid + 1) % PAGE_TABLE_ENTRIES) {
		if (process->memregions[nextid] == 0) {
			break;
		}
	}
	if (nextid == process->next_memregion_idx) {
		// All fields in the table are full
		process->next_memregion_idx = -1;
	} else {
		process->next_memregion_idx = nextid;
	}

	return 0;
}

/**
 * @brief Removes previously allocated pages from the process' memory space
 * 
 * The function uses the memregions table to map from the address to the
 * number of pages to free. It also frees the table entry when done and
 * updates the process data structure accordingly.
 */
int _remove_pages(void* argbase) {
	vaddr_t base = (vaddr_t) argbase;

	if (base % PAGE_SIZE != 0) return -1;

	process_t *process = get_self()->process;
	if (process == NULL) kernel_panic("Unregistered thread");

	// Get number of pages in region
	int i, reg_idx;
	int num_pages = 0;
	for (i = 0; i < PAGE_TABLE_ENTRIES; ++i) {
		if ((process->memregions[i] & 0xfffff000) == base) {
			reg_idx = i;
			num_pages = process->memregions[i] & 0xfff;
			break;
		}
	}

	// There was no memory region at this address
	if (num_pages == 0) return -2;

	// Remove the pages
	for (i = 0; i < num_pages; ++i) {
		int err = destroy_page(base + i * PAGE_SIZE);
		if (err) kernel_panic("Memory regions unsafely unallocated");		
	}

	// Clear the region entry
	process->memregions[reg_idx] = 0;
	if (process->next_memregion_idx == -1) {
		process->next_memregion_idx = reg_idx;
	}

	return 0;
}
