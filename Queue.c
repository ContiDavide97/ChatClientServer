#include <stdio.h>
#include "Queue.h"

int push(Queue *c, Cell *newMessage) {
	if(newMessage == NULL) {
		return 0;
	}
	if(c->last == NULL) {
		c->first = newMessage;
	} else {
		c->last->next= newMessage;
	}
	c->last = newMessage;
	return 1;
}

Cell* pop(Queue *c) {
	Cell *tmp = c->first;
	if(c->first == NULL) {
		return NULL;
	}
	c->first = c->first->next;
	tmp->next = NULL;
	if(c->first == NULL) {
		c->last = NULL;
	}
	return tmp;
}