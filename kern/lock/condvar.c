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
#include <inc/syscall.h>
#include <lock.h>

/**
 * @brief Initializes the given condition
 * 
 * The behavior when initializing an already initialized condvar or when
 * acting on a condvar while it is initializing is undefined. The init
 * function doesn't protect against interleaving.
 * 
 * @return 0 if success, negative number if error
 */
void cond_init(cond_t *cv) {
	// Condition mutex creation
	mutex_init(&cv->mutex);

	// Data structure initialization
	cv->first_waiting = NULL;
	cv->last_waiting = NULL;
}

/**
 * @brief Destroys the given condition
 * 
 * The behavior when destroying a condvar while it is being acted on. The
 * destroy function doesn't protect against interleaving.
 */
void cond_destroy(cond_t *cv) {
	// Destroy the list
	cv->first_waiting = NULL;
	cv->last_waiting = NULL;

	// Destroy the mutex
	mutex_destroy(&cv->mutex);
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
	mutex_lock(&cv->mutex);

	// We register ourselves on the list
	cond_waitlist_addLast(cv, get_running());

	mutex_unlock(&cv->mutex);
	mutex_unlock(mp);

	// We deschedule ourselves
	int reject = 0;
	_deschedule(&reject);

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
	mutex_lock(&cv->mutex);

	// This does nothing if there is no thread to wake
	awaken_first_thread(cv);
	
	mutex_unlock(&cv->mutex);
}

/**
 * @brief Wakes all the threads in the waiting list
 * 
 * If no threads wait on the condition (at the moment we call it, or more
 * exactly when we acquire the mutex), this function has no effect (the signal
 * is lost).
 */
void cond_broadcast(cond_t *cv) {
	mutex_lock(&cv->mutex);

	// Remove all threads in the list
	while(awaken_first_thread(cv) == 0) continue;

	mutex_unlock(&cv->mutex);
}

/**
 * @brief Actually wakes the first thread in the list
 */
int awaken_first_thread(cond_t *cv) {
	thread_t *awaken = cond_waitlist_removeHead(cv);
	if (awaken == NULL) return -1; // Signal we're at the end of the list

	while(_make_runnable(awaken->tid) != 0) _yield(awaken->tid);

	return 0;
}

void cond_waitlist_addLast(cond_t *cv, thread_t *thr) {
	if (cv->last_waiting == NULL) {
		// The list is empty
		cv->first_waiting = thr;
		cv->last_waiting = thr;
	} else {
		// Someone is waiting
		cv->last_waiting->cond_nextwait = thr;
		cv->last_waiting = thr;
	}
}

thread_t *cond_waitlist_removeHead(cond_t *cv) {
	thread_t *next = cv->first_waiting;
	if (next != NULL) {
		cv->first_waiting = next->cond_nextwait;
		if (cv->last_waiting == next) {
			// There was only one thread in the list
			cv->last_waiting = NULL;
		}
	}
	return next;
}
