#include "dtex_array.h"
#include "dtex_log.h"

#include <stdlib.h>
#include <string.h>

struct dtex_array {
	void** data;
	size_t data_sz;

	int size;
	int capacity;
};

static inline void**
_create_data_array(int cap, size_t data_sz) {
	void** data = (void**)malloc(data_sz * cap);
	char* data_ptr = (char*)(data);
	for (int i = 0; i < cap; ++i) {
		data[i] = data_ptr;
		data_ptr += data_sz;
	}
	return data;
}

struct dtex_array* 
dtex_array_create(int cap, size_t data_sz) {
	if (cap <= 0) {
		return NULL;
	}

	struct dtex_array* array = (struct dtex_array*)malloc(sizeof(struct dtex_array));
	if (!array) {
		dtex_fault("dtex_array_create malloc fail.");
	}

	array->data = _create_data_array(cap, data_sz);
	array->data_sz = data_sz;

	array->size = 0;
	array->capacity = cap;

	return array;
}

void 
dtex_array_release(struct dtex_array* array) {
	free(array->data);
	free(array);
}

int 
dtex_array_size(struct dtex_array* array) {
	return array->size;
}

void* 
dtex_array_fetch(struct dtex_array* array, int idx) {
	if (idx < 0 || idx >= array->size) {
		return NULL;
	} else {
		return array->data[idx];
	}
}

void 
dtex_array_add(struct dtex_array* array, void* data) {
	if (array->size == array->capacity) {
		int cap = array->capacity * 2;
		array->capacity = cap;

		void** new_data = _create_data_array(array->capacity, array->data_sz);
		memcpy(new_data, array->data, array->size * array->data_sz);
		free(array->data);
		array->data = new_data;
	}

	memcpy(array->data[array->size++], data, array->data_sz);
}

void 
dtex_array_clear(struct dtex_array* array) {
	array->size = 0;
}