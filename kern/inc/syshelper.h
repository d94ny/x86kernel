/**
 * @file thread.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Function prototype and structures for thread management
 */

#ifndef __P2_SYSHELPER_H_
#define __P2_SYSHELPER_H_
 
#include <exec2obj.h>
#include <thread.h>
#define EOF ((char)(-1))
#define MAX_ARGS 1024
#define AUTHORIZED_FLAGS 0x000108d5
#define STRARR_MAX_SIZE 1024
#define STR_MAX_LEN 4096

typedef struct {
	unsigned long off;
	unsigned long len;
	unsigned long start;
} simple_elf_seg_t;

const exec2obj_userapp_TOC_entry *exec2obj_entry(const char *filename);
int getbytes(const char *filename, int offset, int size, char *buf);
int string_array_length(char **array);

void init_syscall_mutexes(void);

// Check functions
boolean_t check_page(vaddr_t addr, boolean_t write);
boolean_t check_buffer(char* buf, size_t len, boolean_t write);
boolean_t check_string(char* str);
boolean_t check_string_array(char **arr);
boolean_t check_array(void* array, size_t len);

#endif /* !__P2_SYSHELPER_H_ */
