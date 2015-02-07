#include "dtex_vector.h"

#include <stdlib.h>
#include <string.h>

struct dtex_vector {
	void** items;
	int size;
	int capacity;
};

struct dtex_vector* 
dtex_vector_create(int cap) {
	if (cap <= 0) return NULL;

	struct dtex_vector* vec = (struct dtex_vector*)malloc(sizeof(*vec));
	vec->size = 0;
	vec->items = (void**)malloc(cap * sizeof(void*));

	return vec;
}

void 
dtex_vector_release(struct dtex_vector* vec) {
	free(vec->items);
	free(vec);
}

int 
dtex_vector_size(struct dtex_vector* vec) {
	return vec->size;
}

void 
dtex_vector_push_back(struct dtex_vector* vec, void* item) {
	if (vec->size == vec->capacity) {
		int cap = vec->capacity * 2;
		void** new_items = (void**)malloc(cap * sizeof(void*));
		vec->capacity = cap;
		memcpy(new_items, vec->items, vec->size * sizeof(void*));
		free(vec->items);
		vec->items = new_items;
	}

	vec->items[vec->size++] = item;
}

void 
dtex_vector_clear(struct dtex_vector* vec) {
	vec->size = 0;
}

void* 
dtex_vector_get(struct dtex_vector* vec, int idx) {
	if (idx < 0 || idx >= vec->size) {
		return NULL;
	} else {
		return vec->items[idx];
	}
}
