/**
 * @file semaphore.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Implements semaphores for synchronization
 * 
 * Semaphores are implemented using a combination of a mutex and a condition
 * variable, both used to protect and signal on a counter, which represents
 * the number of "free slots" available in the semaphore. A semaphore with a
 * value of 1 is equivalent to a mutex.
 * The implementation is based on the lecture slides, with adaptations to make
 * them fit in the mutex/condvar model. Details are presented in each
 * function's descriptions.
 * The role of the mutex is to protect the consistency of the semaphore 
 * notably the relationship between the free slots value and the condition
 * variable). The condition variable is signaled when the semaphore passes
 * from a locked to an unlocked state, namely when its value passes from
 * negative to 0
 * 
 * @bugs No bugs known
 */

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#include <inc/cond.h>
#include <inc/mutex.h>
#include <inc/sem.h>

/**
 * @brief Initializes the semaphore
 * 
 * This function allocates and initializes the required data structures for
 * the semaphore. Calling this function in conjunction with other semaphore
 * related functions has an undefined behavior. No interleaving protection is
 * given by the function.
 * 
 * @param count the initial value of the semaphore (the "free slots")
 * @return 0 on success, negative number on error
 */
int sem_init(sem_t *sem, int count){
	// Trying to initialize an already initialized semaphore
	assert(!sem->initialized);

	// Initial value
	sem->n = count;

	// Initialize the mutex
	sem->mutex = calloc(1, sizeof(mutex_t));
	if (sem->mutex == NULL) {
		return -1;
	}
	if (mutex_init(sem->mutex) != 0) {
		return -2;
	}

	// Initialize the condition variable
	sem->free_slots = calloc(1, sizeof(cond_t));
	if (sem->free_slots == NULL) {
		return -3;
	}
	if (cond_init(sem->free_slots) != 0) {
		return -4;
	}

	// All is set, we can begin to use the semaphore
	sem->initialized = TRUE;

	return 0;
}

/**
 * @brief Tries to enter the semaphore
 * 
 * The function decreases the semaphore value, marking its interest in going
 * in, and waits until a slot frees itself if all are occupied.
 */
void sem_wait(sem_t *sem){
	// We can't wait on a non-initialized semaphore
	assert(sem->initialized);

	// Entering critical section
	mutex_lock(sem->mutex);

	// Update semaphore value
	--sem->n;

	// If there are no slots for us, we wait for one
	if (sem->n < 0) {
		cond_wait(sem->free_slots, sem->mutex);
	}

	// We're in, end critical section
	mutex_unlock(sem->mutex);
}

/**
 * @brief Exits the section protected by the semaphore
 * 
 * The function increases the semaphore value to liberate a slot on the
 * semaphore. If all slots were taken (n < 0), the thread wakes the first
 * waiting thread to take that empty slot.
 */
void sem_signal(sem_t *sem){
	// We can't signal on an uninitialized semaphore
	assert(sem->initialized);

	// Entering critical section
	mutex_lock(sem->mutex);

	// We free a slot
	++sem->n;

	// If there was no free slot inside, we signal that there is one now
	if (sem->n <= 0) {
		/* The condition is not mandatory, but it avoids that the semaphore
		 * send a signal to the condition when no one is waiting, for
		 * performance reasons.
		 */
		cond_signal(sem->free_slots);
	}

	// We're out, end critical section
	mutex_unlock(sem->mutex);
}

/**
 * @brief Destroys the given semaphore
 * 
 * The call makes the semaphore unusable until the next call to sem_init().
 * Like sem_init, this function has undefined behavior if an other semaphore
 * function interleaves with it.
 */
void sem_destroy(sem_t *sem){
	// We can't destroy an already destroyed thread
	assert(sem->initialized);

	// Disables the semaphore
	sem->initialized = FALSE;

	/* We free the mutex. mutex_destroy takes care of checking if people were
	 * waiting.
	 */
	mutex_destroy(sem->mutex);
	free(sem->mutex);
	sem->mutex = NULL;

	/* We free the condition. cond_destroy takes care of checking if people
	 * were waiting.
	 */
	cond_destroy(sem->free_slots);
	free(sem->free_slots);
	sem->free_slots = FALSE;
}
