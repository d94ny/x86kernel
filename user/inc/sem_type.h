/** @file sem_type.h
 *  @brief This file defines the type for semaphores.
 */

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <inc/types.h>

#include <inc/mutex.h>
#include <inc/cond.h>

typedef struct sem {
	boolean_t initialized;	// Is the semaphore initialized
	int n;					// The "free slots" count
	mutex_t *mutex;			// Mutex protecting the data structure
	cond_t *free_slots;		// Condition signaling that some slots are free
} sem_t;

#endif /* _SEM_TYPE_H */
