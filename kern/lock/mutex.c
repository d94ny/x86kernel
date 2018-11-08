/**
 * @file mutex.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Implements the mutual exclusion mechanism
 * 
 * @section Functionality
 * This file is responsible for providing a mutual exclusion mechanism with 
 * bounded waiting and progress.
 *
 * @section Architecture
 * Each mutex can be accessed (read or written) by only one thread at the time. 
 * To ensure that property we use a boolean lock on which we perform the 
 * testandset operation.
 *
 * To indicate whether a thread currently holds the mutex protecting a specific 
 * critial section we use the "owner" field. That field is also used to 
 * guarantee fair bounded waiting. See the mutex_lock and mutex_unlock function 
 * for further details.
 * 
 * @bugs No bugs known
 */

#include <stdlib.h>
#include <stdio.h>
#include <inc/syscall.h>
#include <assert.h>
#include <lock.h>
#include <types.h>
#include <errors.h>
#include <simics.h>

static boolean_t operational = FALSE;

/**
 * @brief Signals that mutexes are now operational
 */
void install_mutex(void) {
	operational = TRUE;
}

/**
 * @brief Initializes a mutex
 *
 * @param mp a pointer to a mutex
 * @return 0 on success, negative error code otherwise
 */
void mutex_init(mutex_t *mp) {
	mp->first_waiting = NULL;
	mp->last_waiting = NULL;
	mp->owner = NULL;
	mp->list_owner = NULL;
	mp->mutex_lock = FALSE;
	mp->previous_lock = NULL;
}

/**
 * @brief Destroys a mutex
 *
 * @param mp the mutex to destroy
 */
void mutex_destroy(mutex_t *mp) {

	// First get the lock to interact with the mutex
	boolean_t *mlock = &(mp->mutex_lock);
	thread_t *me = get_running();

	while (testandset(mlock)) _yield(mp->list_owner->tid);
	mp->list_owner = me;
	mp->previous_lock = NULL;

	// make sure nobody owns the lock
	assert(mp->owner == NULL);
}

/**
 * @brief Aquire the mutex
 *
 * Only one thread is allowed to interact (that is read or alter) any mutex 
 * at a given time. To make sure that property holds we use a lock which is just 
 * a boolean value and use the "testandset" operation to aquire it.
 *
 * To check whether a thread is inside the critical section protected by that 
 * mutex we use the "owner" field. If it is set to anything other than -1, a 
 * thread has acquired the mutex.
 *
 * To ensure bounded waiting in a fair manner each mutex has a waiting list of 
 * kernel ID's. When the thread which currently holds the mutex releases it, it 
 * removes the head of the waiting list and sets the owner field of the mutex to 
 * that kernel ID. This ensures that the next thread which will enter the 
 * critical section is the next on the waiting list.
 *
 * @param mp the mutex to aquire
 */
void mutex_lock(mutex_t *mp) {

	if (!operational) return;
	// Get the lock
	boolean_t *mlock = &(mp->mutex_lock);

	thread_t *me = get_self();

	/**
	 * We get the lock to interact with the mutex. As explained before, we 
	 * cannot yield to the thread currently interacting with the mutex as 
	 * this requires a read which we are not allowed.
	 */
	while (testandset(mlock)) _yield(mp->list_owner->tid);
	mp->list_owner = me;

	// At this point we can interact with the mutex 
	if (mp->owner == NULL) {

		// If no one owns the mutex now go right in
		mp->owner = me;

	} else {

		boolean_t waiting = FALSE;

		// Wait until someone makes us the owner
		while (mp->owner != me) {

			// Add ourselves to the waitingList
			// unless we already did that
			if (!waiting) {
				waitlist_addLast(mp, me);
				waiting = TRUE;
			}

			// Then release the "mlock"
			*mlock = FALSE;
			mp->list_owner = NULL;

			// Yield to the owner
			_yield(mp->owner->tid);

			// When we run again make sure we can still interact
			while (testandset(mlock)) _yield(mp->list_owner->tid);
			mp->list_owner = me;
		}

	}

	// Add this to our list of mutexes we hold
	if (mp == me->acquired_lock) panic("Relock!");
	mp->previous_lock = me->acquired_lock;
	me->acquired_lock = mp;

	// release the mutex interaction lock but keep the actual mutex
	*mlock = FALSE;
	mp->list_owner = NULL;
}


/**
 * @brief Releases a mutex
 *
 * Like with mutex_lock we first need to be able to interact with the mutex. 
 * Once that property is satified we check whether the waiting list is empty or 
 * not and in the ladder case make the head of the list the new owner of the 
 * mutex. If the list is empty we simply release the mutex.
 *
 * @param mp the mutex to release
 */
void mutex_unlock(mutex_t *mp) {

	if (!operational) return;

	// Get the locks
	boolean_t *mlock = &(mp->mutex_lock);

	thread_t *me = get_self();

	// First we get the lock to interact with the mutex
	while (testandset(mlock)) _yield(mp->list_owner->tid);
	mp->list_owner = me;

	// Now we can interact with the mutex

	// We remove the mutex from the list of acquired locks
	if (mp != me->acquired_lock) kernel_panic("We lost a mutex somewhere");
	me->acquired_lock = me->acquired_lock->previous_lock;
	mp->previous_lock = NULL;

	// It is illegal for an application to unlock a mutex that is not 
	// locked.
	if (mp->owner == NULL) {
		*mlock = FALSE;
		mp->list_owner = FALSE;
		return;
	}

	// Now check if there are threads in the waiting list
	do {
		mp->owner = waitlist_removeHead(mp);
	} while (mp->owner != NULL && mp->owner->state != THR_RUNNING);

	// release the mutex and transfer 
	*mlock = FALSE;
	mp->list_owner = FALSE;

	if (mp->owner != NULL) {
		_yield(mp->owner->tid);
	}
}

/**
 * @brief Adds the given thread at the end of the waiting list of mp
 */
void waitlist_addLast(mutex_t *mp, thread_t *thr) {
	if (!operational) return;

	if (mp->last_waiting == NULL) {
		// The list is empty
		mp->first_waiting = thr;
		mp->last_waiting = thr;
	} else {
		// Someone is waiting
		mp->last_waiting->mutex_nextwait = thr;
		mp->last_waiting = thr;
	}
}

/**
 * @brief Removes and returns the first element of the waiting list of mp
 */
thread_t *waitlist_removeHead(mutex_t *mp) {
	if (!operational) return NULL;

	thread_t *next = mp->first_waiting;
	if (next != NULL) {
		mp->first_waiting = next->mutex_nextwait;
		if (mp->last_waiting == next) {
			// There was only one thread in the list
			mp->last_waiting = NULL;
		}
	}
	return next;
}
