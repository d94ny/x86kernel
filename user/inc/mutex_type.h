/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

#include <inc/types.h>
#include "list.h"

typedef struct mutex {
	boolean_t initialized;	// Is the mutex initialized
	boolean_t mutex_lock;	// Is someone performing changes on the STRUCTURE
	int owner;				// Who is in the mutex (=locked). -1 if nobody's in
	int id;					// Identifier for the mutex, used for debugging
	list_t *waiting_list;	// List of threads waiting to enter the mutex
} mutex_t;


#endif /* _MUTEX_TYPE_H */
