/**
 * @file p2thrlist.c
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * @brief Functions to manipulate a list of thread descriptors
 *
 * @section Functionality
 *
 * @section Architecture
 *
 * @bugs None
 */

#include <simics.h>
#include <malloc.h>		/* malloc() */
#include <stdlib.h>		/* NULL */
#include <p2thrlist.h>


/**
 * @brief Initializes a list
 *
 * @return a pointer to the newly created list
 */
thrlist_t *thrlist_init() {

	/**
	 * Allocate the memory on the heap so that we don't pass copies of the 
	 * list arround. Altough a copy would be rather light given it has only 
	 * two pointers and a size attribute
	 */
	thrlist_t *list = (thrlist_t *)malloc(sizeof(thrlist_t));
	
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
int thrlist_destroy(thrlist_t *list) {

	/**
	 * Return an error if the list is NULL
	 */
	if (list == NULL) return -1;

	/**
	 * Otherwise iterate over the entire list and free everything
 	 */
	thrnode_t *current = list->head;
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
thrnode_t *thrlist_node(thrdesc_t *desc) {
	
	/**
	 * Create a node with next and previous being NULL.
	 * We place the node on the heap.
	 * We need to remember to free it later, as well as the thread 
	 * descriptor desc.
	 */
	thrnode_t *node = (thrnode_t *)malloc(sizeof(thrnode_t));
	
	/**
	 * If the allocation failed we return NULL
	 */
	if (node == NULL) return NULL;

	/**
	 * Otherwise we initialize the node with the thread descriptor and empty 
	 * pointers to next and previous.
	 */
	node->desc = desc;
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
void thrlist_addFirst(thrlist_t *list, thrdesc_t *desc) {

	/**
	 * First we create the new Node of the linked list.
	 */
	thrnode_t *node = thrlist_node(desc);

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
void thrlist_addLast(thrlist_t *list, thrdesc_t *desc) {

	/**
	 * First we create the new Node of the linked list.
	 */
	thrnode_t *node = thrlist_node(desc);

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
 * @brief Finds the thread descriptor of given kernel ID
 *
 * @param list the list in which to search for the thread descriptor
 * @param kid the kernel ID of the thread we are looking for
 */
thrdesc_t *thrlist_findkern(thrlist_t *list, int kid) {

	/**
	 * Make sure the list is not NULL
	 */
	if (list == NULL) return NULL;

	/**
	 * Otherwise we travers the linked list from the head until we find the
	 * thread descriptor with the same kernel ID
	 */
	thrnode_t *current = list->head;
	while (current != NULL && current->desc->kid != kid) {
		current = current->next;
	}

	/**
	 * If current is NULL we did not find the thread descriptor. In that 
	 * case we just return NULL. Otherwise we return the descriptor.
	 */
	if (current == NULL) return NULL;
	return current->desc;


}


/**
 * @brief Finds the thread descriptor of given user ID
 *
 * @param list the list in which to search for the thread descriptor
 * @param kid the kernel ID of the thread we are looking for
 */
thrdesc_t *thrlist_finduser(thrlist_t *list, int tid) {

	/**
	 * Make sure the list is not NULL
	 */
	if (list == NULL) return NULL;

	/**
	 * Otherwise we travers the linked list from the head until we find the
	 * thread descriptor with the same user ID
	 */
	thrnode_t *current = list->head;
	while (current != NULL && current->desc->tid != tid) {
		current = current->next;
	}

	/**
	 * If current is NULL we did not find the thread descriptor. In that 
	 * case we just return NULL. Otherwise we return the descriptor.
	 */
	if (current == NULL) return NULL;
	return current->desc;

}

/**
 * @brief Remove the tail of the given list and returns it.
 *
 * Important Notice : Since thread descriptors can be in multiple lists we 
 * should not deallocate the thread descriptors themselves, only the node.
 *
 * @param list the linked list from which to remove/pop the tail
 */
thrdesc_t *thrlist_removeTail(thrlist_t *list) {

	/**
	 * If the list is NULL, or empty we return NULL
	 */
	if (list == NULL) return NULL;
	else if (list->tail == NULL) return NULL;

	/**
	 * Otherwise we save the thread descriptor of the tail and then proceed 
	 * to update the list pointers. 
	 *
	 * Important : Free the memory of the node.
	 */
	thrnode_t *target = list->tail;
	thrdesc_t *desc = target->desc;
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

	return desc;
}


/**
 * @brief Remove the head of the given list and returns it.
 *
 * Like for removeTail we should only free the node but not the thread 
 * descriptor itself, as it can be in multiple lists.
 *
 * @param list the linked list from which to remove/pop the head
 */
thrdesc_t *thrlist_removeHead(thrlist_t *list) {

	/**
	 * If the list is NULL, or empty we return NULL
	 */
	if (list == NULL) return NULL;
	else if (list->head == NULL) return NULL;

	/**
	 * Otherwise we save the thread descriptor of the head and then proceed 
	 * to update the list pointers.
	 *
	 * Importnat : Free the memory of the node
	 */
	thrnode_t *target = list->head;
	thrdesc_t *desc = target->desc;
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

	return desc;
}


/**
 * @brief Remove the thread with given kernel ID from the list
 *
 * @param list the linked list from which to remove the kernel descriptor
 * @param kid the kernel ID of the thread to remove
 */
void thrlist_remove(thrlist_t *list, int kid) {

	/**
	 * Make sure the list is not NULL
	 */
	if (list == NULL) return;

	/**
	 * Otherwise we travers the linked list from the head until we find the 
	 * thread descriptor with the same kernel ID
	 */
	thrnode_t *current = list->head;
	while (current != NULL && current->desc->kid != kid) {
		current = current->next;
	}

	/**
	 * If current is NULL we did not find the thread descriptor. In that 
	 * case we just return.
	 */
	if (current == NULL) return;

	/**
	 * Decrement the size of the list by 1
	 */
	list->size -= 1;	

	/**
	 * Otherwise we need to update his successor and predecessor
	 * If we have no predecessor we are the head, so let removeHead do the 
	 * work.
	 */
	if (current->prev == NULL) {
		
		/**
		 * Make sure head is really the thread we are looking for. Then 
		 * we need to free the thread descriptor as well as the node 
		 * itself.
		 */
		if (list->head->desc->kid != kid) return;
		thrlist_removeHead(list);
		return;
	}

	/**
	 * If the the successor is NULL we are at the tail, so we let removeTail 
	 * do the work.
	 */
	if (current->next == NULL) {
		
		/**
		 * Make sure tail is really the thread we are looking for. Then 
		 * we need to free the thread descriptor as well as the node 
		 * itself.
		 */
		if (list->tail->desc->kid != kid) return;
		thrlist_removeTail(list);
		return;
	}

	/**
	 * Otherwise we have both a successor and a predecessor.
	 * Also remember to free the node.
	 */
	current->prev->next = current->next;
	current->next->prev = current->prev;
	
	/**
	 * We need to free both the node.
 	 */
	free(current);	return;

}


/**
 * @brief Returns the size of the given list
 *
 * @param list the list whose size we are interested in
 * @param a positive number for the size, a negative error code otherwise
 */
int thrlist_size(thrlist_t *list) {

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


