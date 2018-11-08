/**
 * @file p2thread.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Header file for p2thread.h
 */

#ifndef __P2_P2THRLIST_H_
#define __P2_P2THRLIST_H_

#include "mutex_type.h"
#include "cond_type.h"

/**
 * Data structure for a thread descriptor. This contains the kernel ID and the 
 * user thread ID, as well as the base of the thread's stack.
 */
typedef struct thrdesc thrdesc_t;
struct thrdesc {
	int kid;
	int tid;
	int zombie;
	void **statusp;
	unsigned int sbase;
	mutex_t *mutex;
	cond_t *death;
	int joined;
	thrdesc_t *joined_by;
};

/**
 * Data structure for a Linked List Node. Contains pointers to previous and next 
 * nodes in the list, as well as a pointer to a thread desciptor.
 */
typedef struct thrnode_t thrnode_t;
struct thrnode_t {
	thrdesc_t *desc;
	thrnode_t *next;
	thrnode_t *prev;
};


/**
 * Data structure for double linked lists of thread descriptors. We simply save 
 * a pointer to both the head and the tail of the double linked list
 */
typedef struct thrlist {
	thrnode_t *head;
	thrnode_t *tail;
	int size;
} thrlist_t;

int thrlist_destroy(thrlist_t *);
thrlist_t *thrlist_init();
thrnode_t *thrlist_node(thrdesc_t *);
void thrlist_addFirst(thrlist_t *, thrdesc_t *);
void thrlist_addLast(thrlist_t *, thrdesc_t *);
thrdesc_t *thrlist_findkern(thrlist_t *, int);
thrdesc_t *thrlist_finduser(thrlist_t *, int);
thrdesc_t *thrlist_removeTail(thrlist_t *);
void thrlist_remove(thrlist_t *, int);
thrdesc_t *thrlist_removeHead(thrlist_t *);
int thrlist_size(thrlist_t *);

#endif /* !__P2_P2THRLIST_H_ */
