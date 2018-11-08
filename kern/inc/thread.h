/**
 * @file thread.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Function prototype and structures for thread management
 */

#ifndef __P2_THREAD_H_
#define __P2_THREAD_H_

typedef struct thread_t thread_t;

#include <types.h>
#include <page_types.h>
#include <thrlist.h>
#include <process.h>
#include <lock.h>

#define THREAD_INITIAL_TID 32
#define THREAD_KERNEL_SIZE 2

/**
 * Lock used to make the malloc functions thread safe.
 * @see malloc_wrappers.c
 */
extern mutex_t mem_lock;

/**
 * States a thread can be in during his life. For each of these states, 
 * there is a corresponding 'setter' function.
 */
typedef enum {THR_RUNNING, THR_BLOCKED, THR_SLEEPING,
	THR_WAITING, THR_ZOMBIE} thrstate_t;

/**
 * The thread control block
 */
struct thread_t {

	// the thread ID and state of the thread
	unsigned int 	tid;
	thrstate_t	state;

	/* The esp where the thread's context was saved during a 
	 * context_switch */
	unsigned int	esp;

	/* The stack pointer of the kernel and user stacks */
	uint32_t	esp0;
	uint32_t	esp3;

	/* The process/task the thread belongs to */
	process_t	*process;

   	/* A lock used to assure atomicity during a deschedule/make_runnable 
	 * sequence */
	mutex_t		*thread_lock;

	/* List of acquired locks we should release when being vanished */
	mutex_t		*acquired_lock;

	/**
	 * The following are used to provide an embeded traversal method.
	 * This restricts each thread to be part of at most one list, which
	 * is a resonable assumptions since a thread can't be running,
	 * sleeping or waiting at the same time.
	 */
	thrlist_t	*list;
	thread_t	*next;
	thread_t	*prev;

	/* Embeded traversal for the hash table of all threads in existence */
	thread_t	*hash_next;
	thread_t	*hash_prev;

	/**
	 * The following are used to identify threads belonging to the 
	 * same process.
	 */
	thread_t	*older_sibling;
	thread_t	*younger_sibling;	

	/**
	 * The following field is used for the sleep() system call and 
	 * specifies the time, as the total number of clock interrupts 
	 * since the timer has initialized, when the thread should be made
	 * runnable again.
	 */
	unsigned int	wake;

	/* The thread's registered exception handler, as per the swexn system
	 * call */
	vaddr_t swexn_eip;
	vaddr_t swexn_esp;
	void *swexn_arg;

	/* The following is used for the kernel locking system to provide
	 * embeded traversal for waiting lists. */
	thread_t 	*mutex_nextwait;
	thread_t	*cond_nextwait;
};


/* Initialisation */
void thread_init(void);

/* Setting */
int unset_state(thread_t *thread);
int set_running(thread_t *thread);
int set_runnable(thread_t *thread);
int set_blocked(thread_t *thread);
int set_sleeping(thread_t *thread, unsigned int sleep);
int set_waiting(thread_t *thread);

/* Getting */
thread_t *get_thread(unsigned int tid);
thread_t *get_self(void);
thread_t *get_running(void);
thread_t *get_sleeping(void);
thread_t *get_waiting(process_t *parent);
thread_t *idle(void);
thread_t *init(void);

/* Misc */
unsigned int num_runnable(void);
int is_idle(thread_t *thread);
int set_idle(thread_t *idle);
int set_init(thread_t *init);

/* Management */
unsigned int next_tid(void);
thread_t *create_thread(process_t *parent);
thread_t *copy_thread(process_t *, thread_t *, boolean_t handler);
int vanish_thread(void);
int destroy_thread(thread_t *thread);

#endif /* ! __P2_THREAD_H_ */

