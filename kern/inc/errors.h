/**
 * @file errors.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brieif This header file defines all sorts of errors
 */


#ifndef __P2_ERRORS_H_
#define __P2_ERRORS_H_

/* GENERAL ERRORS */
#define GENERAL_SUCCESS 0
#define ERR_ARG_NULL -2
#define ERR_NEGATIVE_ARG -18
#define ERR_INVALID_TID -3
#define ERR_INVALID_ARG -34
#define ERR_MALLOC_FAIL -12

/* LIST MANAGEMENT */
#define ERR_REMOVE_FAIL -4
#define ERR_ADD_FAIL -5
#define ERR_THR_IN_LIST -10

/* THREAD MANAGEMENT */
#define ERR_SELF_NULL -6
#define ERR_YIELD_NOT_RUNNABLE -7
#define ERR_NEGATIVE_SLEEP -8
#define ERR_NOT_BLOCKED -9

/* EXEC */
#define ERR_ELF_INVALID -11
#define ERR_CALLOC_FAIL -12
#define ERR_ELF_LOAD_FAIL -13
#define ERR_SAVE_ARGS_FAIL -14
#define ERR_CREATE_USERSTACK_FAIL -15
#define ERR_SEGMENT_PAGE_FAIL -16

/* STRING LENGTH */
#define ERR_ARRAY_LENGTH -17

/* EXEC 2 OBJ */
#define ERR_NO_OBJ_ENTRY -19
#define ERR_INVALID_OFFSET -20

/* FORK */
#define ERR_COPY_PRO_FAIL -21
#define ERR_COPY_THR_FAIL -22
#define ERR_MULTIPLE_THREADS -29

/* WAIT */
#define ERR_NO_CHILDREN -23
#define ERR_MISBEHAVING_CHILD -24
#define ERR_NO_ORIGINAL_THREAD -25
#define ERR_CHILDREN_GONE -26
#define ERR_NO_PROCESS -27
#define ERR_WAIT_FULL -28

/* PAGES */
#define ERR_NO_FRAMES -30
#define ERR_KERNEL_FRAME -33
#define ERR_FREE_OWNERLESS_FRAME -35
#define ERR_TOO_MANY_FRAME_OWNERS -36
#define ERR_PAGE_ALREADY_PRESENT -37
#define ERR_DIRECTORY_NOT_PRESENT -38
#define ERR_KERNEL_PAGE -39
#define ERR_PAGE_NOT_PRESENT -40
#define ERR_WORN_OUT_NEW_PAGES -41

/* VANISH */
#define ERR_ACTIVE_THREADS -31
#define ERR_PROCESS_NOT_EXITED -32

/* PANIC */
void panic(const char *, ...);
void kernel_panic(const char *, ...);

#endif /* !__P2_ERRORS_H_ */
