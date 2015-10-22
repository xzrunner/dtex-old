#include "dtex_texture_cache.h"
#include "dtex_texture.h"
#include "dtex_package.h"

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
	struct texture_with_key* freenode;

	struct texture_with_key *head, *tail;
};

static struct texture_cache C;

void 
dtex_texture_cache_init(int cap) {
	memset(&C, 0, sizeof(C));

	C.cap = cap;

	struct texture_with_key* prev = NULL;
	for (int i = 0; i < MAX_SIZE; ++i) {
		C.freelist[i].prev = prev;
		if (i == MAX_SIZE - 1) {
			C.freelist[i].next = NULL;
		} else {
			C.freelist[i].next = &C.freelist[i+1];
		}
		prev = &C.freelist[i];
	}
	C.freenode = &C.freelist[0];
}

bool 
dtex_texture_cache_add(struct dtex_texture* tex, struct dtex_package* pkg, int idx) {
	struct dtex_texture* exists = dtex_texture_query(pkg, idx);
	if (exists) {
		return true;
	}

	int area = tex->width * tex->height;
	if (area > C.cap) {
		return false;
	}
	
	// pop
	while (!C.freenode || (C.curr_cap + area > C.cap)) {
		assert(C.head);

		if (!C.head->next) {
			int zz = 0;
		}

		C.curr_cap -= C.head->tex->width * C.head->tex->height;
		dtex_texture_release(C.head->tex);
		C.head->pkg->textures[C.head->idx] = NULL;

		struct texture_with_key* new_head = C.head->next;
		C.head->next = C.freenode;
		C.freenode->prev = C.head;
		C.freenode = C.head;

		C.head = new_head;
		C.head->prev = NULL;
	}

	// push
	struct texture_with_key* node = C.freenode;
	assert(node);
	C.freenode = C.freenode->next;

	if (!C.head) {
		assert(!C.tail);
		C.head = C.tail = node;
		node->prev = node->next = NULL;
	} else {
		node->prev = C.tail;
		node->next = NULL;
		C.tail->next = node;
		C.tail = node;
	}

	node->pkg = pkg;
	node->idx = idx;
	node->tex = tex;

	C.curr_cap += area;

	return true;
}

struct dtex_texture* 
dtex_texture_query(struct dtex_package* pkg, int idx) {
	if (!C.tail) {
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
	if (C.tail == node) {
		return node->tex;
	}

	if (C.head == node) {
		C.head = node->next;
	}
	if (node->prev) {
		node->prev->next = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}
	node->prev = C.tail;
	node->next = NULL;
	C.tail->next = node;
	C.tail = node;

	return node->tex;
}
