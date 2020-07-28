#ifndef STACK_H_
#define STACK_H_

typedef struct stackitem {
	void *item;
	int misc;
	struct stackitem *next;
} stack;

stack* push (void *newItem, int misc, stack *head);
void* pop (stack **head);
void clearStack (stack *head, void (*clear)(void*));

unsigned depth (stack *head);

#endif /* STACK_H_ */
