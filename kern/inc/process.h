/**
 * @file process.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Function prototype and structures for process management
 */

#ifndef __P2_PROCESS_H_
#define __P2_PROCESS_H_

typedef struct process_t process_t;

#include <page_types.h>
#include <thread.h>
#include <thrlist.h>

#define PROCESS_INITIAL_PID 1

/* The states a process belongs to during his lifetime */
typedef enum {RUNNING, EXITED, BURIED} process_state_t;

/* The process control block */
struct process_t {

	/* Process ID, exit_status and state */
	unsigned int 	pid;
	int 			exit_status;
	process_state_t state;	

	/* The page directory base pointer cr3 */
	pde_t			*cr3;
	
	/* New pages chunks @see syscall/paging.c */
	uint32_t		*memregions;
	unsigned int	next_memregion_idx;

	/**
	 * The following are used to provide a family hierachy between 
	 * processes.
	 */
	process_t		*parent;
	process_t		*youngest_child;
	process_t		*older_sibling;
	process_t		*younger_sibling;
	unsigned int	children;

	/**
	 * The following is to provide access to all threads of a given 
	 * process. The thread ID of the original thread of this task is 
	 * also saved for `wait`.
	 */
	thread_t		*youngest_thread;
	int				original_tid;
	
	/**
	 * This is the count of "active" threads, which have not yet 
	 * called `vanish_thread()`.
	 */
	unsigned int	threads;

	/* List of waiting threads */
	thrlist_t	*waiting;

};

process_t *create_process(void);
int destroy_process(process_t *process);
process_t *create_god_process(void);
process_t *copy_process(process_t *parent);
process_t *exited_child(process_t *parent);
unsigned int next_pid(void);
int vanish_process(process_t *process);

#endif /* ! __P2_PROCESS_H_ */
