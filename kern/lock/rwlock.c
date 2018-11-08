/**
 * @file rwlock.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Implements reader/writer locks
 * 
 * This implementation of the reader/writer locks ensures that the writers
 * will never starve, by granting them priority whenever they want to access
 * the lock. The lock is protected by a global mutex that ensures that
 * operations on the data structure will be performed without outside
 * interruption. Waiting is put in place via two conditions, one for the
 * writers (no_threads_in), and one for the readers (no_writers_in). In any
 * case where both conditions become true, the no_threads_in condition will be
 * signaled before the no_writers_in condition, ensuring that the writers
 * waiting have priority to get in.
 * 
 * @bugs The implementation allows starvation of the readers
 */

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <types.h>
#include <lock.h>

/**
 * @brief Initializes the lock
 * 
 * Allocates data structures and puts default values in the lock fields. The
 * behavior is undefined if any operation is performed on the lock while it is
 * initializing (it doesn't protect against interleaving)
 */
void rwlock_init(rwlock_t *rwlock) {
	mutex_init(&rwlock->mutex);
	cond_init(&rwlock->no_threads_in);
	cond_init(&rwlock->no_writers_in);

	// Initial values
	rwlock->writer_in = FALSE;
	rwlock->readers_in = 0;
	rwlock->writers_waiting = 0;
	rwlock->readers_waiting = 0;
}

/**
 * @brief Locks the lock in the selected mode
 * 
 * To enter the lock in read mode, it is sufficient that no writer thread is
 * currently holding the lock. To enter in write mode, the lock has to be letf
 * by any thread currently in it for the writing thread to enter.
 * 
 * @param type RWLOCK_READ to read, RWLOCK_WRITE to write in the critical
 * section
 */
void rwlock_lock(rwlock_t *rwlock, int type) {
	mutex_lock(&rwlock->mutex);

	switch(type) {
		case RWLOCK_READ:
			// We want to read, just wait that no writers are waiting
			while (rwlock->writer_in || rwlock->writers_waiting > 0) {
				/* While because broadcast can not wake the thread up before
				 * the next writer arrives
				 */
				++rwlock->readers_waiting;
				cond_wait(&rwlock->no_writers_in, &rwlock->mutex);
				--rwlock->readers_waiting;
			}
			// We're in, update the readers count
			++rwlock->readers_in;
			break;
		case RWLOCK_WRITE:
			// We want to write, we have to wait that the lock is empty
			if (rwlock->writer_in || rwlock->readers_in > 0) {
				++rwlock->writers_waiting;
				cond_wait(&rwlock->no_threads_in, &rwlock->mutex);
				--rwlock->writers_waiting;
			}
			// We're in
			rwlock->writer_in = TRUE;
			break;
		default:
			assert(FALSE);
	}

	// We're in
	mutex_unlock(&rwlock->mutex);
}

/**
 * @brief Unlocks the lock
 * 
 * We release the lock to the next writer waiting, if there is one, or to the
 * whole pool of waiting readers otherwise.
 */
void rwlock_unlock(rwlock_t *rwlock) {
	mutex_lock(&rwlock->mutex);

	if (rwlock->writer_in) {
		// We are a writer
		if (rwlock->writers_waiting) {
			/* Other fellow writers are waiting, we wake them up, and don't
			 * signal that we left so no evil readers can sneak in
			 */
			cond_signal(&rwlock->no_threads_in);
		} else {
			// No one's writing, we let the readers in
			rwlock->writer_in = FALSE;
			cond_broadcast(&rwlock->no_writers_in);
		}
	} else {
		// We are a reader, so we leave
		--rwlock->readers_in;
		if (rwlock->readers_in == 0 && rwlock->writers_waiting > 0) {
			/* If we were the last one, we kindly signal the writers that they
			 * can get in
			 */
			cond_signal(&rwlock->no_threads_in);
		}
	}

	mutex_unlock(&rwlock->mutex);
}

/**
 * @brief Passes from writer mode to reader mode
 * 
 * This method atomically adds a reader while removing the writer from the loc
 * The readers are notified if no writers are waiting.
 */
void rwlock_downgrade(rwlock_t *rwlock) {
	// We have to be a writer
	assert(rwlock->writer_in);

	mutex_lock(&rwlock->mutex);

	rwlock->writer_in = FALSE;
	++rwlock->readers_in;

	cond_broadcast(&rwlock->no_writers_in);

	mutex_unlock(&rwlock->mutex);
}

/**
 * @brief Destroys the lock
 * 
 * The effects of doing an operation on a lock that is being destroyed are
 * undefined. The function doesn't provide a protection against interleaving.
 */
void rwlock_destroy(rwlock_t *rwlock) {
	// No one can be in the lock or waiting on it
	assert(rwlock->readers_in == 0);
	assert(!rwlock->writer_in);
	assert(rwlock->writers_waiting == 0);
	assert(rwlock->readers_waiting == 0);

	// Free the mutex and condvars
	mutex_destroy(&rwlock->mutex);
	cond_destroy(&rwlock->no_threads_in);
	cond_destroy(&rwlock->no_writers_in);
}
