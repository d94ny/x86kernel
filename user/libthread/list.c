#include <stdlib.h>
#include "list.h"
#include <malloc.h>
#include <assert.h>

list_t *list_init() {
	list_t *list = (list_t *)malloc(sizeof(list_t));
	
	/**
	 * If the malloc failed we return NULL, otherwise we initialize the 
	 * attributes of the linked list
	 */
	if (list == NULL) return NULL;

	list->size = 0;
	list->head = NULL;
	list->tail = NULL;

	return list;
}

/**
 * @brief Destroys a given list
 *
 * We deallocate all the nodes of the given list. Since a thread descriptor can 
 * be part of multiple lists, we should not deallocate the thread descriptors 
 * themselves.
 *
 * Thread descriptors should be deallocated only when they are removed from the 
 * "living threads" list.
 *
 * @param list the linked list to destroy
 * @return 0 on success, otherwise a negative error code
 */
int list_destroy(list_t *list) {

	/**
	 * Return an error if the list is NULL
	 */
	if (list == NULL) return -1;

	/**
	 * Otherwise iterate over the entire list and free everything
 	 */
	listnode_t *current = list->head;
	while (current != NULL) {

		/**
		 * We need to advance to the next node before actually freeing 
		 * the current node
		 */
		current = current->next;
		free(current->prev);
	}

	/**
	 * Also free the list descriptor itself.
	 */
	free(list);
	return 0;

}


/**
 * @brief Create a new Node for a thread descriptor
 *
 * The node is placed on the heap so that we can handle the pointer. Notice that 
 * the thread descriptor needs to be saved on the heap as well as we manipulate 
 * the pointer.
 *
 * @param desc a pointer to the thread descriptor
 * @return a copy of the list node
 */
listnode_t *list_node(int value) {
	
	/**
	 * Create a node with next and previous being NULL.
	 * We place the node on the heap.
	 * We need to remember to free it later, as well as the thread 
	 * descriptor desc.
	 */
	listnode_t *node = (listnode_t *)malloc(sizeof(listnode_t));
	
	/**
	 * If the allocation failed we return NULL
	 */
	if (node == NULL) return NULL;

	/**
	 * Otherwise we initialize the node with the thread descriptor and empty 
	 * pointers to next and previous.
	 */
	node->value = value;
	node->next = NULL;
	node->prev = NULL;
 
	return node;

}


/**
 * @brief Inserts a new thread descriptor at the head of the given linked list
 *
 * @param list the list to which we want to add an element
 * @param desc the thread descriptor to be added to the linked list
 */
void list_addFirst(list_t *list, int value) {

	/**
	 * First we create the new Node of the linked list.
	 */
	listnode_t *node = list_node(value);

	/**
	 * Increment the size of the list by 1
	 */
	list->size += 1;	

	/**
	 * If the head is NULL, this is the first element of in our linked list, 
	 * so we simpty point the head to the node, with both next and previous 
	 * being NULL. Point the tail to the node as well.
	 */
	if (list->head == NULL) {
		list->head = node;
		list->tail = node;
		return;
	}

	/**
	 * Otherwise we make the new node the head of our linked list. Thus he 
	 * point backwards to head, while head's next attribute now points to 
	 * the thread Node
	 */
	node->next = list->head;
	list->head->prev = node;
	list->head = node;

	return;
}


/**
 * @brief Inserts a new thread descriptor at the tail of the given linked list
 *
 * @param list the list to which we want to add an element
 * @param desc the thread descriptor to be added to the linked list
 */
void list_addLast(list_t *list, int value) {

	assert(list != NULL);
	/**
	 * First we create the new Node of the linked list.
	 */
	listnode_t *node = list_node(value);

	/**
	 * Increment the size of the list by 1
	 */
	list->size += 1;	

	/**
	 * If the tail is NULL this is the first element of the list, so we add 
	 * it to both the head and the tail, leaving the pointers next and prev 
	 * to NULL. 
	 */
	if (list->head == NULL) {
		list->head = node;
		list->tail = node;
		return;
	}

	/**
	 * Otherwise we make the new node the tail of our linked list. Thus his 
	 * next node/successor will be the current tail, while the tail will now 
	 * point back to the node
	 */
	node->prev = list->tail;
	list->tail->next = node;
	list->tail = node;

	return;

}

/**
 * @brief Remove the tail of the given list and returns it.
 *
 * Important Notice : Since thread descriptors can be in multiple lists we 
 * should not deallocate the thread descriptors themselves, only the node.
 *
 * @param list the linked list from which to remove/pop the tail
 */
int list_removeTail(list_t *list) {

	/**
	 * If the list is NULL, or empty we return NULL
	 */
	if (list == NULL) return -1;
	else if (list->tail == NULL) return -1;

	/**
	 * Otherwise we save the thread descriptor of the tail and then proceed 
	 * to update the list pointers. 
	 *
	 * Important : Free the memory of the node.
	 */
	listnode_t *target = list->tail;
	int value = target->value;
	list->tail = list->tail->prev;
	free(target);
	
	/**
	 * Decrement the size of the list by 1
	 */
	list->size -= 1;	
	
	/**
	 * If we just removed the only element of the list we need to set head 
	 * to NULL and we are done. Otherwise we need to change the successor of 
	 * the new tail to NULL.
	 */
	if (list->tail == NULL) list->head = NULL;
	else list->tail->next = NULL;

	return value;
}


/**
 * @brief Remove the head of the given list and returns it.
 *
 * Like for removeTail we should only free the node but not the thread 
 * descriptor itself, as it can be in multiple lists.
 *
 * @param list the linked list from which to remove/pop the head
 */
int list_removeHead(list_t *list) {

	/**
	 * If the list is NULL, or empty we return NULL
	 */
	if (list == NULL) return -1;
	else if (list->head == NULL) return -1;

	/**
	 * Otherwise we save the thread descriptor of the head and then proceed 
	 * to update the list pointers.
	 *
	 * Importnat : Free the memory of the node
	 */
	listnode_t *target = list->head;
	int value = target->value;
	list->head = list->head->next;
	free(target);

	/**
	 * Decrement the size of the list by 1
	 */
	list->size -= 1;	

	/**
	 * If we just removed the only element of the list we need to set tail 
	 * to NULL and we are done. Otherwise we need to change the predecessor 
	 * of the new head to NULL.
	 */
	if (list->head == NULL) list->tail = NULL;
	else list->head->prev = NULL;

	return value;
}


/**
 * @brief Returns the size of the given list
 *
 * @param list the list whose size we are interested in
 * @param a positive number for the size, a negative error code otherwise
 */
int list_size(list_t *list) {

	/**
	 * First we check that the list is not NULL. If so we return a negative 
	 * error code (-1)
	 */
	if (list == NULL) return -1;

	/**
	 * Otherwise return the size attribute of the list
	 */
	return list->size;

}
