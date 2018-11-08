/** @file rwlock_type.h
 *  @brief This file defines the type for reader/writer locks.
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <inc/cond.h>
#include <inc/mutex.h>
#include <inc/types.h>

typedef struct rwlock {
	boolean_t initialized;
	mutex_t *mutex;			// General protection mechanism
	boolean_t writer_in;	// Is a writer currently holding the lock
	int readers_in;			// Number of reader threads in the lock
	int writers_waiting;	// Number of writer threads waiting on the lock
	int readers_waiting;	// Number of reader threads waiting on the lock
	cond_t *no_threads_in;	// Condition for writers when everyone is out
	cond_t *no_writers_in;	// Condition for readers when all writers are out
} rwlock_t;

#endif /* _RWLOCK_TYPE_H */
