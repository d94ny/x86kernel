/**
 * @file thrlist.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Prototypes and structures for Thread list management
 */

#ifndef __P3_THRLIST_H_
#define __P3_THRLIST_H_

// Above include to avoid circular dependency
typedef struct thrlist_t thrlist_t;

#include <thread.h>

/* The structure of our double linked list */
struct thrlist_t {
	thread_t *head;
	thread_t *tail;
	unsigned int size;
};

/* List Management */
void thrlist_init(thrlist_t *list);
int thrlist_destroy(thrlist_t *list);

/* Removing nodes */
int thrlist_remove(thread_t *thread);

/* Adding nodes */
int thrlist_add_tail(thread_t *thread, thrlist_t *list);
int thrlist_add_head(thread_t *thread, thrlist_t *list);
int thrlist_add_sorted(thread_t *thread, thrlist_t *list);

/* Accessing nodes */
thread_t *thrlist_find(unsigned int, thrlist_t *);

#endif /* !__P3_THRLIST_H_ */
