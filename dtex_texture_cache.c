#include "dtex_texture_cache.h"
#include "dtex_texture.h"

#include <string.h>
#include <assert.h>

#define MAX_SIZE 512

struct texture_with_key {
	struct texture_with_key *prev, *next;

	struct dtex_package* pkg;
	int idx;

	struct dtex_texture* tex;
};

struct texture_cache {
	int cap;
	int curr_cap;

	struct texture_with_key freelist[MAX_SIZE];
	int size;

	struct texture_with_key *head, *tail;
};

static struct texture_cache C;

void 
dtex_texture_cache_init(int cap) {
	memset(&C, 0, sizeof(C));
}

bool 
dtex_texture_cache_add(struct dtex_texture* tex, struct dtex_package* pkg, int idx) {
	int area = tex->width * tex->height;
	if (area > C.cap) {
		return false;
	}

	// pop
	while (C.size >= MAX_SIZE || (C.curr_cap + area > C.cap)) {
		assert(C.head);
		--C.size;
		C.curr_cap -= C.head->tex->width * C.head->tex->height;
		C.head = C.head->next;
		C.head->prev = NULL;
	}

	// push
	assert(C.size < MAX_SIZE);

	struct texture_with_key* node = &C.freelist[C.size++];

	if (!C.head) {
		assert(!C.tail);
		C.head = C.tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = C.tail;
		node->next = NULL;
		C.tail->next = node;
	}

	node->pkg = pkg;
	node->idx = idx;
	node->tex = tex;

	C.curr_cap += area;

	return true;
}

struct dtex_texture* 
dtex_texture_query(struct dtex_package* pkg, int idx) {
	if (C.size == 0) {
		return NULL;
	}

	// query
	struct texture_with_key* node = C.tail;
	while (node) {
		if (node->pkg == pkg && node->idx == idx) {
			break;
		}
		node = node->prev;
	}
	if (!node) {
		return NULL;
	}

	// reinsert
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}
	node->prev = C.tail;
	node->next = NULL;
	C.tail->next = node;

	return node->tex;
}
