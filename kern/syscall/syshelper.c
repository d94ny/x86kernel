/**
 * @file syshelper.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief System call helper functions 
 */

#include <string.h>
#include <exec2obj.h>
#include <elf_410.h>
#include <stdlib.h>
#include <errors.h>
#include <page.h>
#include <x86/page.h>
#include <cr.h>
#include <simics.h>

#include <syshelper.h>

/**
 * @brief Returns the userapp entry for a given filename
 *
 * @param filename the file whose entry we want
 * @return the exec2obj userapp entry, NULL if not found
 */
const exec2obj_userapp_TOC_entry *exec2obj_entry(const char *filename) {

	// Verify that filename is not NULL
	if (filename == NULL) return NULL;

	// Find the corresponding entry in the exec2obj	
	const exec2obj_userapp_TOC_entry *cursor = exec2obj_userapp_TOC;
	const exec2obj_userapp_TOC_entry *target = NULL;

	int i;
	for (i = 0; i < exec2obj_userapp_count; i++, cursor++) {
		
		// Compare the execname to filename
		if (strcmp(filename,cursor->execname) == 0) {
			
			// We found the correct entry
			target = cursor;
			break;
		}
	}
	
	// If target is NULL we didn't find the entry
	return target;
}


/**
 * @brief Copies data from a file into a buffer.
 *
 * @param filename   the name of the file to copy data from
 * @param offset     the location in the file to begin copying from
 * @param size       the number of bytes to be copied
 * @param buf        the buffer to copy the data into
 *
 * @return returns the number of bytes copied on succes; -1 on failure
 */
int getbytes(const char *filename, int offset, int size, char *buf) {

	// Verify filename and buf
	if (filename == NULL || buf == NULL) {
		return ERR_ARG_NULL;
	}

	// Verify size and offset
	if (size < 0 || offset < 0) return ERR_NEGATIVE_ARG;

	// Find the corresponding entry in the exec2obj	
	const exec2obj_userapp_TOC_entry *target = exec2obj_entry(filename);;
	
	// If target is NULL we didn't find the entry
	if (target == NULL) return ERR_NO_OBJ_ENTRY;

	// Make sur our offset is still valid
	if (target->execlen <= offset) return ERR_INVALID_OFFSET;

	// Copy 'size' bytes into buf unless we reach EOF before
	const char *bytes = &target->execbytes[offset];

	int i;
	for (i = 0; i < size && bytes != NULL && bytes[i] != EOF
			&& (i+offset) < target->execlen; i++) {
 
		// Simply copy into te buffer
		buf[i] = bytes[i];
	}

	// The number of copied bytes is i
	return i;
}

/**
 * @brief Gives the length of the given null-terminated string array
 */
int string_array_length(char **array) {
	if (array == NULL) return 0;
	int i;
	for (i = 0; i <= MAX_ARGS; ++i) {
		if (array[i] == NULL) {
			return i;
		}
	}
	return ERR_ARRAY_LENGTH;
}

/**
 * @brief Checks whether the given address won't cause a fault
 * 
 * @param write checks if we can write to the address as well
 */
boolean_t check_page(vaddr_t addr, boolean_t write) {
	pte_t *pte = get_pte(PAGE_ADDR(addr), (void*) get_cr3());

	if (pte != NULL) {
		boolean_t check = TRUE;
		check &= PE_GETFLAG(*pte, PTE_PRESENT);
		if (write) {
			check &= PE_GETFLAG(*pte, PTE_USER);
			check &= PE_GETFLAG(*pte, PTE_READWRITE);
		}
		return check;
	}

	return FALSE;
}

/**
 * @brief Checks whether the given buffer can be safely read/written to
 */
boolean_t check_buffer(char* buf, size_t len, boolean_t write) {
	uint32_t cursor;
	int checked;
	for (cursor = (uint32_t) buf, checked = 0;
			checked < len;
			checked += PAGE_SIZE - (cursor - PAGE_ADDR(cursor)),
				cursor = PAGE_ADDR(cursor) + PAGE_SIZE) {
		if (!check_page(cursor, write)) {
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * @brief Checks if the given string terminates and can be read safely
 */
boolean_t check_string(char* str) {
	uint32_t cursor;
	int checked;
	for (cursor = (uint32_t) str, checked = 0;
			checked < STR_MAX_LEN;
			cursor = PAGE_ADDR(cursor) + PAGE_SIZE) {
		if (!check_page(cursor, FALSE)) {
			return FALSE;
		}

		int i;
		for (i = cursor;
				(i < PAGE_ADDR(cursor) + PAGE_SIZE) & (checked < STR_MAX_LEN);
				++i, ++checked) {
			if (*((char*)i) == 0) return TRUE;
		}
	}
	return FALSE;
}

/**
 * @brief Checks if the null-terminated array of strings is valid and safe
 */
boolean_t check_string_array(char **arr) {
	uint32_t cursor;
	int checked;
	for (cursor = (uint32_t) arr, checked = 0;
			checked < STRARR_MAX_SIZE;
			cursor = PAGE_ADDR(cursor) + PAGE_SIZE) {
		if (!check_page(cursor, FALSE)) {
			return FALSE;
		}

		uint32_t i;
		for (i = cursor;
				i < PAGE_ADDR(cursor) + PAGE_SIZE;
				i += sizeof(char *), ++checked) {
			if (*((char*)i) == 0) return TRUE;
			if (!check_string((char*)i)) {
				return FALSE;
			}
		}
	}
	return FALSE;
}

/**
 * @brief Checks if the memory region starting at array of size len words is
 * safe.
 */
boolean_t check_array(void* array, size_t len) {
	uint32_t cursor;
	int checked;
	for (cursor = (uint32_t) array, checked = 0;
			checked < len;
			checked += PAGE_SIZE - (cursor - PAGE_ADDR(cursor)),
				cursor = PAGE_ADDR(cursor) + PAGE_SIZE) {
		if (!check_page(cursor, FALSE)) {
			return FALSE;
		}
	}
	return TRUE;
}
