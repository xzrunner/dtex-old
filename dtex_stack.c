#include "dtex_stack.h"
#include "dtex_log.h"

#include <stdlib.h>
#include <string.h>

struct dtex_stack {
	void* data;
	size_t data_sz;

	int size;
	int capacity;
};

struct dtex_stack* 
dtex_stack_create(int cap, size_t data_sz) {
	if (cap <= 0) {
		return NULL;
	}

	struct dtex_stack* array = (struct dtex_stack*)malloc(sizeof(struct dtex_stack));
	if (!array) {
		dtex_fault("dtex_stack_create malloc fail.");
	}

	array->data = malloc(data_sz * cap);
	array->data_sz = data_sz;

	array->size = 0;
	array->capacity = cap;

	return array;
}

void 
dtex_stack_release(struct dtex_stack* stack) {
	free(stack->data);
	free(stack);
}

bool 
dtex_stack_empty(struct dtex_stack* stack) {
	return stack->size == 0;
}

void 
dtex_stack_push(struct dtex_stack* stack, void* data) {
	if (stack->size == stack->capacity) {
		int cap = stack->capacity * 2;
		stack->capacity = cap;

		void* new_data = malloc(stack->capacity * stack->data_sz);
		memcpy(new_data, stack->data, stack->size * stack->data_sz);
		free(stack->data);
		stack->data = new_data;
	}

	memcpy((char*)stack->data + stack->data_sz * stack->size, data, stack->data_sz);
	++stack->size;
}

void* 
dtex_stack_top(struct dtex_stack* stack) {
	if (!dtex_stack_empty(stack)) {
		return (char*)stack->data + stack->data_sz * (stack->size - 1);
	} else {
		return NULL;
	}
}

void
dtex_stack_pop(struct dtex_stack* stack) {
	if (!dtex_stack_empty(stack)) {
		--stack->size;
	}
}
