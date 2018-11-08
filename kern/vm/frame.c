/**
 * @file frame.c
 * @brief Contains the frame management functions
 * 
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 */

#include <stdlib.h>
#include <page.h>
#include <x86/page.h>
#include <cr.h>
#include <malloc.h>
#include <string.h>
#include <simics.h>
#include <errors.h>
#include <lock.h>

/*
 * A table holding counters for the frames in user space.
 * 
 * Each counter represents the number of processes currently having a page
 * table entry pointing to the frame. This is used to implement copy on write.
 * For data consistency, it is expected that whenever a frame has a counter
 * value of more than one, no process can have writing rights to it and has to
 * copy the frame before being able to write to it.
 */
static uint8_t *frames = NULL;
/*
 * Total number of frames available for the users
 */
static size_t nb_frames = 0;
/*
 * Identifier of a frame pointed to by nobody. -1 if all frames are taken.
 */
static int next_available_frame = 0;
/*
 * A frame-sized buffer to transfer data between frames, via the kernel
 */
static uint32_t *cow_buffer = NULL;
/*
 * Protects the data structures, only one thread at a time can act on the
 * frame data structures.
 */
static mutex_t fa_mutex;

/**
 * @brief Initializes the frame allocator
 * 
 * This function creates the frames table to be able to allocate frames in the
 * program.
 * 
 * @param upper_mem the size of the allocatable memory, in kilobytes
 * @return 0 on success, negative number on error
 */
int init_frame_allocator(vm_size_t upper_mem) {
	nb_frames = (LOWER_MEM_SIZE + upper_mem * 1024 - USER_MEM_START)
		/ PAGE_SIZE;

	frames = calloc(nb_frames, sizeof(uint8_t));
	if (frames == NULL) return ERR_MALLOC_FAIL;

	cow_buffer = smemalign(PAGE_SIZE, PAGE_SIZE);
	if (cow_buffer == NULL) return ERR_MALLOC_FAIL;

	mutex_init(&fa_mutex);

	return 0;
}

/**
 * @brief Returns the address of an available frame
 * 
 * The function updates the data structure as well. If no frame is available,
 * the function returns a NULL pointer. The calling function must handle this
 * case according to its policy.
 * This function comes in two versions:
 * _allocate_frame, which does the job without caring about locking
 * allocate_frame, which locks the mutex and calls the _ version
 */
paddr_t _allocate_frame() {
	if (next_available_frame == -1) {
		// No available frames to give
		return NULL;
	}

	paddr_t frame = (paddr_t) FRAME_ADDR(next_available_frame);
	
	int err = _get_frame(frame);
	if (err) kernel_panic("Allocate frame couldn't get frame");

	return frame;
}
paddr_t allocate_frame() {
	mutex_lock(&fa_mutex);
	paddr_t frame = _allocate_frame();
	mutex_unlock(&fa_mutex);
	return frame;
}

/**
 * @brief Releases the frame from a process
 * 
 * If the process was the last one holding the frame, the frame is put back in
 * the free frame pool.
 */
int free_frame(paddr_t frame) {
	if (((uint32_t) frame) % PAGE_SIZE != 0) return -1;

	mutex_lock(&fa_mutex);

	if (FRAME_ID(frame) == -1) {
		// Cannot free a kernel frame (no user can own it)
		mutex_unlock(&fa_mutex);
		return ERR_KERNEL_FRAME;
	}

	if (frames[FRAME_ID(frame)] == 0) {
		// Trying to free a frame held by nobody
		mutex_unlock(&fa_mutex);
		return ERR_FREE_OWNERLESS_FRAME;
	}

	frames[FRAME_ID(frame)]--;

	if (frames[FRAME_ID(frame)] == 0 && next_available_frame == -1) {
		// The frame we freed is the only one available
		next_available_frame = FRAME_ID(frame);
	}

	mutex_unlock(&fa_mutex);

	return 0;
}

/**
 * @brief Increases the number of processes having "possession" of the frame
 * 
 * Possession means to have a page table entry pointing to the frame, via copy
 * on write.
 * To prevent the counter from overflowing, every frame can have a maximum of
 * 255 threads pointing to it. When this occurs the calling thread has to
 * allocate a new frame for the page by itself.
 * As for allocate frame, this function has a locking and a non-locking
 * flavor, which are called from the outside or from frame functions,
 * respectively.
 * 
 * @return 0 on success, negative error code otherwise
 */ 
int _get_frame(paddr_t frame) {
	if (((uint32_t) frame) % PAGE_SIZE != 0) return -1;

	if (FRAME_ID(frame) == -1) {
		// No user can own a kernel frame
		return ERR_KERNEL_FRAME;
	}

	if (frames[FRAME_ID(frame)] == (uint8_t) -1) {
		// To prevent the counter from overflowing
		return ERR_TOO_MANY_FRAME_OWNERS; 
	}

	frames[FRAME_ID(frame)]++;

	if (FRAME_ID(frame) == next_available_frame) {
		// We have to find the new next available frame
		int i, f;
		boolean_t found = FALSE;
		for (i = 0, f = next_available_frame; i < nb_frames;
				++i, f = (f+1) % nb_frames) {
			if (frames[f] == 0) {
				next_available_frame = f;
				found = TRUE;
				break;
			}
		}
		if (!found) next_available_frame = -1;		
	}

	return 0;	
}
int get_frame(paddr_t frame) {
	mutex_lock(&fa_mutex);
	int err = _get_frame(frame);
	mutex_unlock(&fa_mutex);
	return err;
}

/**
 * @brief Copies the contents of a page to a new frame exclusive to the
 * calling thread
 * 
 * This function allocates a new frame for the calling thread, and copies the
 * contents of the frame pointed to by the virtual address addr. The caller
 * then has an exclusive, writable copy of the frame, on which he can do
 * whatever he wants.
 * @return 0 on success, a negative number on error
 */
int copy_on_write(vaddr_t page_addr) {
	if (page_addr % PAGE_SIZE != 0) return -1;

	pde_t *cr3 = (pde_t *) get_cr3();
	pte_t *pte = get_pte(page_addr, cr3);
	if (pte == NULL) {
		panic("Trying to copy on write on a non-existing page");
	}
	paddr_t old_frame = (paddr_t) PE_GETADDR(*pte);
	int old_frame_id = FRAME_ID(old_frame);

	mutex_lock(&fa_mutex);

	if (frames[old_frame_id] == 1) {
		// We are the last process holding to the frame, we're good to go
		mutex_unlock(&fa_mutex);
		return 0;
	} else if (frames[old_frame_id] == 0) {
		mutex_unlock(&fa_mutex);
		return ERR_FREE_OWNERLESS_FRAME;
	}

	// Create a new frame to copy to
	paddr_t new_frame = _allocate_frame();
	if (new_frame == NULL) {
		// Impossible to copy on write
		mutex_unlock(&fa_mutex);
		return ERR_NO_FRAMES;
	}

	// Save old frame contents on kernel buffer
	memcpy(cow_buffer, (void*) page_addr, PAGE_SIZE);

	// Set the page table entry to the new frame
	*pte = PE_SETADDR(*pte, new_frame);
	set_cr3(get_cr3());

	// Copy the page pack to the new frame
	memcpy((void*) page_addr, cow_buffer, PAGE_SIZE);

	frames[old_frame_id]--;

	mutex_unlock(&fa_mutex);

	return 0;
}
