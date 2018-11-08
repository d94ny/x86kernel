/** @file lock.h
 *  @brief This file defines the type for all types of locks.
 */

#ifndef _P3_LOCK_H
#define _P3_LOCK_H

typedef struct mutex mutex_t;
typedef struct cond cond_t;
typedef struct rwlock rwlock_t;

#include <types.h>
#include <thread.h>

struct mutex {
	boolean_t mutex_lock;		// Is someone performing changes on the waiting list
	thread_t *owner;			// Who is in the mutex (=locked). NULL if nobody's in
	thread_t *list_owner;		// Who is performing changes on the waiting list
	thread_t *first_waiting;	// First thread in the waiting list
	thread_t *last_waiting;		// Last thread in the waiting list

	mutex_t	 *previous_lock;	// Most recent mutex hold when this one
								// was aquired
};

struct cond {
	mutex_t mutex;			// Mutex protecting the data structure
	thread_t *first_waiting;	// List of threads waiting to be signaled
	thread_t *last_waiting;
};

struct rwlock {
	mutex_t mutex;			// General protection mechanism

	boolean_t writer_in;	// Is a writer currently holding the lock
	int readers_in;			// Number of reader threads in the lock
	int writers_waiting;	// Number of writer threads waiting on the lock
	int readers_waiting;	// Number of reader threads waiting on the lock
	
	cond_t no_threads_in;	// Condition for writers when everyone is out
	cond_t no_writers_in;	// Condition for readers when all writers are out
};

/* Mutexes */
void install_mutex(void);
void mutex_init(mutex_t *mp);
void mutex_destroy(mutex_t *mp);
void mutex_lock(mutex_t *mp);
void mutex_unlock(mutex_t *mp);

void waitlist_addLast(mutex_t *mp, thread_t *thr);
thread_t *waitlist_removeHead(mutex_t *mp);

unsigned int testandset(void *var);

/* Condition variables */
void cond_init(cond_t *cv);
void cond_destroy(cond_t *cv);
void cond_wait(cond_t *cv, mutex_t *mp);
void cond_signal(cond_t *cv);
void cond_broadcast(cond_t *cv);

int awaken_first_thread(cond_t *cv);
void cond_waitlist_addLast(cond_t *cv, thread_t *thr);
thread_t *cond_waitlist_removeHead(cond_t *cv);

/* Readers/Writers locks */
#define RWLOCK_READ  0
#define RWLOCK_WRITE 1

void rwlock_init(rwlock_t *rwlock);
void rwlock_lock(rwlock_t *rwlock, int type);
void rwlock_unlock(rwlock_t *rwlock);
void rwlock_destroy(rwlock_t *rwlock);
void rwlock_downgrade(rwlock_t *rwlock);

#endif /* _P3_LOCK_H */
