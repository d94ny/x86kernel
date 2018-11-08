/**
 * @file thrlist.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Thread list management related functions
 */

#include <malloc.h>
#include <thread.h>	
#include <thrlist.h>
#include <simics.h>
#include <errors.h>

/**
 * @brief Initializes a list of threads
 *
 * Note on thread list : thread control blocks offer an embeded traversal
 * by making use of the 'next' and 'prev' pointers of the TCB data
 * structure. Yet this also means that a thread can be part of at most one
 * list at a time. This seems to be a reasonable assumptions since a thread
 * can not be running and sleeping at the same time for example.
 *
 * @param list the list to initialize
 */
void thrlist_init(thrlist_t *list) {
	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
}

/**
 * @brief Add a thread to the beginning of a list
 *
 * Note that a thread can be part of only one list at a time. Thus if the
 * next and prev pointers designed for an embeded traversal purpose are
 * already defined we return an error
 *
 * @param tcb the control block of the thread to be added
 * @param list the list to which we add the thread
 * @return 0 on success, a negative error code otherwise
 */
int thrlist_add_head(thread_t *thread, thrlist_t *list) {

	// Verify both arguments
	if (thread == NULL || list == NULL) return ERR_ARG_NULL;

	// Make sure the thread is not part of another list
	if (thread->list != NULL) return ERR_THR_IN_LIST;

	// Now we can update the list
	thread->next = list->head;
	thread->prev = NULL;

	// If the list is not empty
	if(list->size > 0) list->head->prev = thread;
	else list->tail = thread;

	list->head = thread;
	list->size += 1;

	thread->list = list;

	return 0;
}

/**
 * @brief Add a thread to the end of a list
 *
 * @param thread the control block of the thread to be added
 * @param list the list to which we add the thread
 * @return 0 on success, a negative error code otherwise
 */
int thrlist_add_tail(thread_t *thread, thrlist_t *list) {

	// Verify both arguments
	if (thread == NULL || list == NULL) return ERR_ARG_NULL;

	// Make sure the thread is not part of another list
	if (thread->list != NULL) return ERR_THR_IN_LIST;

	// Now we can update the list
	thread->prev = list->tail;
	thread->next = NULL;

	// If the list is not empty
	if(list->size > 0) list->tail->next = thread;
	else list->head = thread;

	list->tail = thread;
	list->size += 1;

	thread->list = list;

	return 0;
}

/**
 * @brief Add a thread in a sorted list based on the sleeping time
 *
 * @param thread the the thread to add
 * @param list the list to which the thread should be added
 * @return 0 on success, a negative error code otherwise
 */
int thrlist_add_sorted(thread_t *thread, thrlist_t *list) {

	// Verify both arguments
	if (thread == NULL || list == NULL) return ERR_ARG_NULL;

	// Make sure the thread is not part of another list
	if (thread->list != NULL) return ERR_THR_IN_LIST;

	// If the list is empty we just insert at the head
	if (list->size == 0) return thrlist_add_head(thread, list);

	// Iterate from the until we reach a thread with earlier wake time
	thread_t *current = list->tail;
	while (current != NULL && current->wake > thread->wake) {
		current = current->prev;
	}

	// if we reached the beginning or are still at the tail
	if (current == NULL) return thrlist_add_head(thread, list);
	if (current == list->tail) return thrlist_add_tail(thread,list);

	// We should be inserted just after current
	thread->prev = current;
	thread->next = current->next;
	if (current->next) current->next->prev = thread;
	current->next = thread;

	list->size += 1;
	thread->list = list;

	return 0;
}

/**
 * @brief Find a thread desciptor block in a given list
 *
 * @param tid the thread id of the thread we are looking for
 * @param list the list in which to look
 * @return the thread
 */
thread_t *thrlist_find(unsigned int tid, thrlist_t *list) {

	if (list == NULL) return NULL;

	thread_t *current = list->head;
	while (current != NULL && current->tid != tid) {
		current = current->next;
	}

	return current;
}

/**
 * @brief Remove the given thread from the list he is in
 *
 * Note that since a thread is part of at most one list we do not need to
 * provide that list as an argument.
 *
 * We are given the entire the thread control block and not simply the tid
 * since the calling function probably already has the tcb, and if not it
 * can use the thread hashmap to obtain it much faster.
 *
 * @param thread the tcb of the thread to be removed
 * @return 0 on success, 1 if the thread was not part of any list to begin
 * 	   with and a negative error code otherwise.
 */
int thrlist_remove(thread_t *thread) {
	
	// Verify the argument
	if (thread == NULL) return ERR_ARG_NULL;

	// If the thread is in no list, return 1
	if (thread->list == NULL) return 1;

	// If we have a predecessor
	if (thread->prev != NULL) thread->prev->next = thread->next;
	else thread->list->head = thread->next;

	// If we have a successor
	if (thread->next != NULL) thread->next->prev = thread->prev;
	else thread->list->tail = thread->prev;

	thread->list->size -= 1;
	thread->list = NULL;
	thread->next = NULL;
	thread->prev = NULL;

	return 0;

}

/**
 * @brief Destroys a list
 *
 * We also need to remove all the threads from the list of course
 *
 * @param list the list to be destroyed
 * @return 0 on success, a negative error code otherwise
 */
int thrlist_destroy(thrlist_t *list) {

	// Verify the list
	if (list == NULL) return ERR_ARG_NULL;

	// Remove the head until we are empty
	while (list->size != 0) thrlist_remove(list->head);
	
	// Free the list
	return 0;

}
