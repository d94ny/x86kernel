/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#include <inc/types.h>
#include "list.h"

typedef struct cond {
	boolean_t initialized;	// Is the condvar initialized
	mutex_t *mutex;			// Mutex protecting the data structure
	list_t *waiting_list;	// List of threads waiting to be signaled
} cond_t;

#endif /* _COND_TYPE_H */
