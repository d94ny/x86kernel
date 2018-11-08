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
 * @section Previous Design Attempt
 * Before using the current design we tried the following design :
 *
 * The mutexes include a "waiting list". This list is a linked list who at any
 * time has to be in one of the following states (td is for thread descriptor):
 * 1. Empty: the field mp->last_waiting_thread is NULL, as is mp-
 * next_thread_to_run.
 * 2. Singleton: the field mp->last_waiting_thread points to a thread, and mp-
 * next_thread_to_run points to the same thread.
 * 3. Multiple: the field mp->last_waiting_thread points to a thread that is
 * not the one pointed to by mp->next_thread_to_run.
 * Normally, it is impossible to have only one of mp->last_waiting_thread and
 * mp->next_thread_to_run being null
 * 
 * All modifications to the "waiting list" have to be made with the certitude
 * that no one else will interfere. With that in mind, we have a boolean value
 * mp->waiting_list_locked that we use to achieve mutual exclusion on the list.
 * As the list operations are short and the interference probability is really
 * low, it is not a big loss of performance to yield when we can't get the
 * lock the first time. We have to trust the scheduler to be reasonably fair
 * on this, though.
 * 
 * Every time a thread calls mutex_lock, it has to enqueue itself in the
 * waiting list, and then, if it is alone and the mutex is available, it will
 * take it, otherwise it will wait that its turn come. Normally, if the list
 * is not empty but the lock is available, that means that the head of the
 * waiting list is in the process of acquiring it and therefore the incomning
 * thread should NOT take the lock at that moment. Discipline, my young padawa
 * ...
 * 
 * The release process has to be atomical, in the sense that when the thread
 * releases the lock (after having locked the waiting list), depending on the
 * state of the list, it will do the following:
 * 1. The list is empty: simply exit the function
 * 2. The list is a singleton: wake the thread up, releases the lock and
 * yields to it (after having released the list). The awaken thread will then
 * itself acquire the lock and delete itself from the list.
 * 3. The list is multiple: same procedure
 * 
 * When the head of the waiting list is awaken, it FIRST acquires the waiting
 * list, then the lock, then deletes itself from the list, and puts its
 * successor (if necessary) in the mp->next_thread_to_run field. It then
 * releases the waiting list and return from the mutex_lock function, meaning
 * that the thread has now acquired the lock.
 * 
 * @bugs No bugs known
 */

#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>
#include <assert.h>
#include <inc/mutex.h>

#include "mutex_type.h"

#include "list.h"
#include "p2thread.h"
#include "mutex.h"

// For debugging purposes each mutex has an ID.
int MID = 100;

/**
 * @brief Initializes a mutex
 *
 * @param mp a pointer to a mutex
 * @return 0 on success, negative error code otherwise
 */
int mutex_init(mutex_t *mp) {

	/**
	 * If the mutex is already initialized the behavior is undefined. We 
	 * previously checked whether the field "initialized" was not null, but 
	 * it is very lickely that a newly created mutex contains junk and thus 
	 * is considered as initialized.
	 */
	mp->waiting_list = list_init();
	mp->owner = -1;
	mp->id = MID++;
	mp->initialized = TRUE;
	mp->mutex_lock = FALSE;

	return 0;
}

/**
 * @brief Destroys a mutex
 *
 * @param mp the mutex to destroy
 */
void mutex_destroy(mutex_t *mp) {

	// First get the lock to interact with the mutex
	boolean_t *mlock = &(mp->mutex_lock);

	/**
	 * We use yield(-1) here because we are not allowed to read or write to 
	 * the mutex, thus reading a potential field indicating who is currently 
	 * interacting with the mutex is not possible.
	 */
	while (testandset(mlock)) yield(-1);
	
	// make sure nobody owns the lock
	assert(mp->owner == -1);

	// Destroy everything
	list_destroy(mp->waiting_list);
	mp->waiting_list = NULL;
	mp->owner = -1;
	mp->initialized = FALSE;

	/**
	 * Don't release the mutex.
	 * Threads waiting will thus wait until it gets reinitialized.
	 */
	mp->mutex_lock = TRUE;

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
 * thread has aquired the mutex.
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


	// Trying to acquire an uninitialized mutex
	assert(mp->initialized);

	// Get the lock
	boolean_t *mlock = &(mp->mutex_lock);

	/**
	 * We get the lock to interact with the mutex. As explained before, we 
	 * cannot yield to the thread currently interacting with the mutex as 
	 * this requires a read which we are not allowed.
	 */
	while (testandset(mlock)) yield(-1);

	// At this point we can interact with the mutex 
	if (mp->owner == -1) {

		// If no one owns the mutex now go right in
		mp->owner = gettid();

	} else {

		int id = gettid();
		boolean_t waiting = FALSE;

		// Wait until someone makes us the owner
		while (mp->owner != id && mp->initialized) {

			// Add ourselves to the waitingList
			// unless we already did that
			if (!waiting) {
				list_addLast(mp->waiting_list, id);
				waiting = TRUE;
			}

			// Then release the "mlock"
			*mlock = FALSE;

			// Yield to the owner
			yield(mp->owner);

			// When we run again make sure we can still interact
			while (testandset(mlock)) yield(-1);
		}

	}

	// release the mutex interaction lock but keep the actual mutex
	*mlock = FALSE;

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

	// Trying to acquire an uninitialized mutex
	assert(mp->initialized);

	// Get the locks
	boolean_t *mlock = &(mp->mutex_lock);	

	// First we get the lock to interact with the mutex
	while (testandset(mlock)) yield(-1);

	// Now we can interact with the mutex

	// It is illegal for an application to unlock a mutex that is not 
	// locked.
	if (mp->owner == -1) {
		*mlock = FALSE;
		return;
	}

	// Now check if there are threads in the waiting list
	int next = list_removeHead(mp->waiting_list);

	if (next != -1) mp->owner = next;
	else mp->owner = -1;

	// release the mutex and transfer 
	*mlock = FALSE;
	yield(mp->owner);

}
