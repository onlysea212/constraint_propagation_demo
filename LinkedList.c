#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "LinkedList.h"
#include "Channel.h"
#include "Relation.h"

struct Node* newNode(void *data, int data_type){
	struct Node* newN = malloc(sizeof(struct Node));
	switch (data_type){
		case SEGMENT_TYPE:
		{
			struct Segment *tmp = malloc(sizeof(struct Segment));
			*tmp = *((struct Segment*)data);
			newN->data = tmp;
			newN->type = SEGMENT_TYPE;
			break;
		}
		case INT_TYPE:
		{
			int *tmp = malloc(sizeof(int));
			*tmp = *((int*) data);
			newN->data = tmp;
			newN->type = INT_TYPE;
			break;
		}
		case FILE_OWNER_TYPE:
		{
			struct FileOwner *tmp = malloc(sizeof(struct FileOwner));
			*tmp = *((struct FileOwner*)data);
			newN->data = tmp;
			newN->type = FILE_OWNER_TYPE;
			break;
		}
		case DATA_HOST_TYPE:
		{
			struct DataHost *tmp = malloc(sizeof(struct DataHost));
			*tmp = *((struct DataHost*)data);
			newN->data = tmp;
			newN->type = DATA_HOST_TYPE;
			break;
		}
		case PTHREAD_T_TYPE:
		{
			pthread_t *tmp = malloc(sizeof(pthread_t));
			*tmp = *((pthread_t*)data);
			newN->data = tmp;
			newN->type = PTHREAD_T_TYPE;
			break;
		}
		case STRING_TYPE:
		{
			char *input = (char*)data;
			char *tmp = malloc(strlen(input) + 1);
			strcpy(tmp, input);
			newN->data = tmp;
			newN->type = STRING_TYPE;
			break;
		}
		case CHANNEL_TYPE:
		{
			struct Channel *tmp = malloc(sizeof(struct Channel));
			*tmp = *((struct Channel*)data);
			newN->data = tmp;
			newN->type = CHANNEL_TYPE;
			break;
		}
		case RELATION_TYPE:
		{
			struct Relation *tmp = malloc(sizeof(struct Relation));
			*tmp = *(struct Relation*)data;
			newN->data = tmp;
			newN->type = RELATION_TYPE;
			break;
		}
		default:
			fprintf(stderr, "newNode: unknown data type\b");
			free(newN);
			newN = NULL;
			break;
	}
	return newN;
}

struct LinkedList* newLinkedList(){
	struct LinkedList *ll = malloc(sizeof(struct LinkedList));
	ll->head = NULL;
	ll->tail = NULL;
	ll->n_nodes = 0;
	//ll->lock_ll = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	//ll->cond_ll = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	return ll;
}

struct LinkedList* copyLinkedList(struct LinkedList *srcll){
	if (srcll == NULL)
		return NULL;

	struct LinkedList *dstll = newLinkedList();
	struct Node *it = srcll->head;
	for (; it != NULL; it = it->next){
		/* TODO: different route for FILE_OWNER_TYPE due to its LinkedList */
		struct Node *new_node = newNode(it->data, it->type);
		push(dstll, new_node);
	}
	return dstll;
}

void freeNode(struct Node *node){
	if (!node)
		return;
	switch(node->type){
		case FILE_OWNER_TYPE:
		{
			struct FileOwner *file = (struct FileOwner*)(node->data);
			destructLinkedList(file->host_list);
			break;
		}
	}
	free(node->data);
	free(node);
}

void destructLinkedList(void *arg){
	struct LinkedList *ll = (struct LinkedList*)arg;
	if (ll->n_nodes == 0){
		free(ll);
		return;
	}

	struct Node *it = ll->head;

	while (it != NULL){
		struct Node *tmp = it->next;
		freeNode(it);
		it = tmp;
	}

	free(ll);
}

int llistContain(struct LinkedList ll, struct Node *node){
	if (ll.n_nodes == 0){
		return 0;
	}

	struct Node *it = ll.head;
	for ( ; it != NULL; it = it->next){
		if (it == node)
			return 1;
	}

	return 0;
}

int insertNode(struct LinkedList *ll, struct Node *node, struct Node *offset){
	if (!ll || !node){
		fprintf(stderr, "insertNode: both arguments 'll' and 'node' must not be NULL\n");
		return INSERT_NODE_ARGS_NULL;
	}
	
	if (ll->n_nodes == 0){
		ll->n_nodes ++;
		ll->head = node;
		ll->head->prev = NULL;
		ll->head->next = NULL;
		ll->tail = node;
		return INSERT_NODE_SUCCESS;
	}

	if (!llistContain(*ll, offset) && offset != NULL){
		fprintf(stderr, "insertNode: offset is not a node in the linkedlist\n");
		return INSERT_NODE_NOT_IN_LL;
	}

	if (offset == NULL){
		//insert into the head
		struct Node *tmp = ll->head;
		ll->head = node;
		ll->head->next = tmp;
		ll->head->next->prev = ll->head;
	} else {
		struct Node *tmp = offset->next;
		offset->next = node;
		node->next = tmp;
		node->prev = offset;

		if (offset == ll->tail){
			ll->tail = node;
		} else {
			node->next->prev = node;
		}
	}
	ll->n_nodes += 1;
	return INSERT_NODE_SUCCESS;
}

int removeNode(struct LinkedList *ll, struct Node *node){
	if (!ll || !node){
		fprintf(stderr, "both arguments must not be NULL\n");
		return REMOVE_NODE_ARGS_NULL;
	}

	if (ll->n_nodes <= 0){
		fprintf(stderr, "the list is empty\n");
		return REMOVE_NODE_EMPTY_LL;
	}

	struct Node *prev = node->prev;
	struct Node *next = node->next;

	//printf("process prev and next\n");

	if (ll->n_nodes == 1){
		ll->head = NULL;
		ll->tail = NULL;
	} else if (ll->head == node){
		//remove the head
		ll->head = node->next;
		ll->head->prev = NULL;
	} else if (ll->tail == node){
		//remove the tail
		ll->tail = node->prev;
		ll->tail->next = NULL;
	} else {
		prev->next = next;
		next->prev = prev;
	}

	//printf("free node->data\n");
	switch (node->type){
		case FILE_OWNER_TYPE:
		{
			struct FileOwner *file = (struct FileOwner*)(node->data);
			destructLinkedList(file->host_list);
			break;
		}
		default:
		{
			free(node->data);
			break;
		}
	}

	//printf("free node\n");
	free(node);
	//printf("decrease n_nodes\n");
	ll->n_nodes --;
	//prev->next = next;
	//next->prev = prev;

	return REMOVE_NODE_SUCCESS;
}

void push(struct LinkedList *ll, struct Node *node){
	insertNode(ll, node, ll->tail);
}

struct Node* pop(struct LinkedList *ll){
	if (ll->n_nodes == 0)
		return NULL;
	struct Node *res = newNode(ll->head->data, ll->head->type);
	removeNode(ll, ll->head);
	return res;
}

int empty(struct LinkedList *ll){
	return !ll->n_nodes;
}
