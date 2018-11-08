#ifndef _P2_LIST_H_
#define _P2_LIST_H_

typedef struct listnode listnode_t;
struct listnode {
	int value;
	listnode_t *next;
	listnode_t *prev;
};

typedef struct {
	listnode_t *head;
	listnode_t *tail;
	int size;
} list_t;

list_t *list_init();
int list_destroy(list_t *);
listnode_t *list_node(int);
void list_addFirst(list_t *, int);
void list_addLast(list_t *, int);
int list_removeTail(list_t *);
int list_removeHead(list_t *);
int list_size(list_t *);

#endif /* _P2_LIST_H_ */