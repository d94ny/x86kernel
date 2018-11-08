/**
 * @file condvar.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Implements the condition variable mechanism
 * 
 * The condition variables are protected by a mutex that ensures that only one
 * thread at a time can act on the condvar's data structures. Threads register
 * themselves in a waiting list and are awaken when signals are emitted,
 * either one at a time or via a broadcast call.
 * 
 * @bugs No bugs known
 */

#include <stdlib.h>
#include <syscall.h>
#include <assert.h>
#include <simics.h>

#include <inc/cond.h>
#include <inc/mutex.h>

#include "mutex_type.h"
#include "cond_type.h"

#include "list.h"
#include "p2thread.h"

#include "condvar.h"

/**
 * @brief Initializes the given condition
 * 
 * The behavior when initializing an already initialized condvar or when
 * acting on a condvar while it is initializing is undefined. The init
 * function doesn't protect against interleaving.
 * 
 * @return 0 if success, negative number if error
 */
int cond_init(cond_t *cv) {
	// Condition mutex creation
	cv->mutex = malloc(sizeof(mutex_t));
	if (cv->mutex == NULL) return -1;
	if (mutex_init(cv->mutex) != 0) return -1;

	// Data structure initialization
	cv->waiting_list = list_init();
	if (cv->waiting_list == NULL) return -3;

	// We're done, we can use the condition
	cv->initialized = TRUE;

	return 0;
}

/**
 * @brief Destroys the given condition
 * 
 * The behavior when destroying a condvar while it is being acted on. The
 * destroy function doesn't protect against interleaving.
 */
void cond_destroy(cond_t *cv) {
	// Can't destroy an uninitialized condvar
	assert(cv->initialized);

	// We can't use the condvar anymore
	cv->initialized = FALSE;

	// Destroy the list
	assert(list_size(cv->waiting_list) == 0);
	list_destroy(cv->waiting_list);
	cv->waiting_list = NULL;

	// Destroy the mutex
	mutex_destroy(cv->mutex);
	free(cv->mutex);
	cv->mutex = NULL;
}

/**
 * @brief Waits for the given condition to be signaled
 * 
 * The thread adds itself to the waiting list, and deschedules itself until it
 * is awaken by a signal or broadcast call. Ensuring that the unlocking of the
 * mutex and the descheduling are "atomical" is taken care of in
 * awaken_first_thread.
 * 
 * @param mp the mutex to release while we are waiting for the signal, and to
 * reacquire afterwards
 */
void cond_wait(cond_t *cv, mutex_t *mp) {
	// Can't wait on an uninitialized mutex
	assert(cv->initialized);

	// Enter critical section
	mutex_lock(cv->mutex);

	// We register ourselves on the list
	list_addLast(cv->waiting_list, gettid());

	// We release the mutex
	mutex_unlock(mp);

	// We deschedule ourselves
	int i = 0;
	mutex_unlock(cv->mutex);
	deschedule(&i);

	// We're back, we acquire the mutex and continue execution
	mutex_lock(mp);
}

/**
 * @brief Wakes the first thread in the waiting list
 * 
 * If no threads wait on the condition (at the moment we call it, or more
 * exactly when we acquire the mutex), this function has no effect (the signal
 * is lost).
 */
void cond_signal(cond_t *cv) {
	// Can't signal on an uninitialized condvar
	assert(cv->initialized);

	// Enter critical section
	mutex_lock(cv->mutex);

	// We have a thread to wake
	if (list_size(cv->waiting_list) != 0) {
		assert(awaken_first_thread(cv->waiting_list) == 0);
	}
	
	// We're done, end critical section
	mutex_unlock(cv->mutex);
}

/**
 * @brief Wakes all the threads in the waiting list
 * 
 * If no threads wait on the condition (at the moment we call it, or more
 * exactly when we acquire the mutex), this function has no effect (the signal
 * is lost).
 */
void cond_broadcast(cond_t *cv) {
	// Can't broadcart on an uninitialized condvar
	assert(cv->initialized);

	// Enter critical section
	mutex_lock(cv->mutex);

	// Remove successively all threads in the list
	while(list_size(cv->waiting_list) != 0) {
		assert(awaken_first_thread(cv->waiting_list) == 0);
	}

	// We're done. End critical section
	mutex_unlock(cv->mutex);
}

/**
 * @brief Actually wakes the first thread in the list
 * 
 * This function ensures that the awaken thread was indeed asleep when he was
 * awaken by us by looping and yielding to it so it can achieve its call to
 * deschedule() before we can actually wake it up.
 */
int awaken_first_thread(list_t *list) {
	int awaken = list_removeHead(list);
	while(make_runnable(awaken) != 0) yield(awaken);
	return 0;
}
