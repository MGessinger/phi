#include <stdlib.h>
#include "stack.h"

stack* push (void *new, int misc, stack *head)
{
	stack *newHead = malloc(sizeof(stack));
	if (newHead == NULL)
		return head;
	newHead->misc = misc;
	newHead->item = new;
	newHead->next = head;
	return newHead;
}

void* pop (stack **head)
{
	if (head == NULL || *head == NULL)
		return NULL;
	stack *oldHead = *head;
	*head = oldHead->next;
	void *oldItem = oldHead->item;
	free(oldHead);
	return oldItem;
}

void clearStack (stack *head, void (*clear)(void*))
{
	stack *nextHead;
	while (head != NULL) {
		nextHead = head->next;
		if (clear != NULL && head->item != NULL)
			clear(head->item);
		free(head);
		head = nextHead;
	}
}

unsigned depth (stack *head)
{
	unsigned height = 0;
	while (head != NULL)
	{
		height++;
		head = head->next;
	}
	return height;
}
