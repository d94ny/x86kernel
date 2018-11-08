/**
 * @file thrhash.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Hash table containing all living threads
 */

#include <stdlib.h>
#include <thrhash.h>
#include <thread.h>

/* A table of threads */
static thread_t *table[HASH_ENTRIES] = { NULL };

/**
 * @brief Add a thread to the hash table
 * @param thr the thread to add
 */
void thrhash_add(thread_t *thr) {
	unsigned int entry = thrhash_entry(thr->tid);

	thr->hash_prev = NULL;
	thr->hash_next = table[entry];

	if (table[entry] != NULL) {
		table[entry]->hash_prev = thr;
	}
	table[entry] = thr;	
}

/**
 * @brief Removes a thread from the hash table
 * @param thr the thread to remove
 */
void thrhash_remove(thread_t *thr) {
	unsigned int entry = thrhash_entry(thr->tid);

	// If we have a predecessor
	if (thr->hash_prev != NULL)
		thr->hash_prev->hash_next = thr->hash_next;
	else table[entry] = thr->hash_next;

	// If we have a successor
	if (thr->hash_next != NULL)
		thr->hash_next->hash_prev = thr->hash_prev;

	thr->hash_next = NULL;
	thr->hash_prev = NULL;
}

/**
 * @brief Search for a thread given his tid
 * @param the thread on sucess, NULL otherwise
 */
thread_t *thrhash_find(unsigned int tid) {
	unsigned int entry = thrhash_entry(tid);

	thread_t *current = table[entry];

	while (current != NULL && current->tid != tid) {
		current = current->hash_next;
	}

	return current;
}

/**
 * @brief The hash function
 * @return the hash for the given tid
 */
unsigned int thrhash_entry(unsigned int tid) {
	return tid % HASH_ENTRIES;
}
