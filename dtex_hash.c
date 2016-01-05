#include "dtex_hash.h"
#include "dtex_array.h"
#include "dtex_log.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

static const size_t HASH_SZ_TBL[] = {
	11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51439, 102881, 202881, 402881
};

struct hash_node {
	struct hash_node* next;
	void* key;
	void* val;
};

struct changeable {
	struct hash_node* buf;
	size_t buf_sz;

	struct hash_node* freelist;

	size_t node_used;

	struct hash_node** hashlist;
	size_t hash_sz;
};

struct dtex_hash {
	float rehash_weight;

	unsigned int (*hash_func)(int hash_sz, void* key);
	bool (*equal_func)(void* key0, void* key1);

	struct changeable* c;
};

static inline size_t
_find_next_hash_sz(int n) {
	int size = sizeof(HASH_SZ_TBL) / sizeof(HASH_SZ_TBL[0]);
	for (int i = 0; i < size; ++i) {
		if (HASH_SZ_TBL[i] > n) {
			return HASH_SZ_TBL[i];
		}
	}
	return 0;
}

static inline void
_init_changeable(struct changeable* c, size_t buf_sz, size_t hash_sz) {
	size_t free_list_sz = sizeof(struct hash_node) * buf_sz;
	size_t hash_list_sz = sizeof(struct hash_node*) * hash_sz;

	memset(c + 1, 0, free_list_sz + hash_list_sz);

 	c->buf = (struct hash_node*)(c + 1);
	c->buf_sz = buf_sz;
	c->freelist = c->buf;
	c->node_used = 0;
 	for (int i = 0; i < buf_sz - 1; ++i) {
 		struct hash_node* hn = &c->buf[i];
 		hn->next = &c->buf[i + 1];
 	}
 	c->buf[buf_sz - 1].next = NULL;
 
 	c->hash_sz = hash_sz;
 	c->hashlist = (struct hash_node**)((intptr_t)c + sizeof(struct changeable) + free_list_sz);
}

struct dtex_hash* 
dtex_hash_create(int capacity, int hash_size, float rehash_weight,
				 unsigned int (*hash_func)(int hash_sz, void* key),
				 bool (*equal_func)(void* key0, void* key1)) {
	if (capacity <= 0 || hash_size <= 0) {
		return NULL;
	}

	size_t hash_sz = _find_next_hash_sz(hash_size);
	if (hash_sz == 0) {
		return NULL;
	}

	struct dtex_hash* hash = (struct dtex_hash*)malloc(sizeof(struct dtex_hash));
	if (!hash) {
		return NULL;
	}

	hash->rehash_weight = rehash_weight;

	hash->hash_func = hash_func;
	hash->equal_func = equal_func;

	size_t free_list_sz = sizeof(struct hash_node) * capacity;
	size_t hash_list_sz = sizeof(struct hash_node*) * hash_sz;
	struct changeable* c = (struct changeable*)malloc(sizeof(struct changeable) + free_list_sz + hash_list_sz);
	if (!c) {
		free(hash);
		return NULL;
	}
	_init_changeable(c, capacity, hash_sz);
	hash->c = c;

	return hash;
}

void 
dtex_hash_release(struct dtex_hash* hash) {
	free(hash);
}

void* 
dtex_hash_query(struct dtex_hash* hash, void* key) {
	unsigned int idx = hash->hash_func(hash->c->hash_sz, key);
	struct hash_node* hn = hash->c->hashlist[idx];
	while (hn) {
		if (hash->equal_func(key, hn->key)) {
			return hn->val;
		}
		hn = hn->next;
	}
	return NULL;
}

void 
dtex_hash_query_all(struct dtex_hash* hash, void* key, struct dtex_array* ret) {
	unsigned int idx = hash->hash_func(hash->c->hash_sz, key);
	struct hash_node* hn = hash->c->hashlist[idx];
	while (hn) {
		if (hash->equal_func(key, hn->key)) {
			dtex_array_add(ret, &hn->val);
		}
		hn = hn->next;
	}
}

static inline void
_enlarge_freelist(struct dtex_hash* hash) {
	struct changeable* old = hash->c;

	dtex_info(" [hash] node resize %d -> %d", old->buf_sz, old->buf_sz * 2);
	size_t new_buf_sz = old->buf_sz * 2;
	size_t free_list_sz = sizeof(struct hash_node) * new_buf_sz;
	size_t hash_list_sz = sizeof(struct hash_node*) * old->hash_sz;
	struct changeable* new = (struct changeable*)malloc(sizeof(struct changeable) + free_list_sz + hash_list_sz);
	if (!new) {
		return;
	}

	memset(new + 1, 0, free_list_sz + hash_list_sz);

	new->buf = (struct hash_node*)(new + 1);
	new->buf_sz = new_buf_sz;

	new->freelist = &new->buf[old->buf_sz];
	for (int i = old->buf_sz; i < new_buf_sz - 1; ++i) {
		new->buf[i].next = &new->buf[i + 1];
	}
	new->buf[new_buf_sz - 1].next = NULL;

	new->node_used = old->node_used;
	memcpy(new->buf, old->buf, sizeof(struct hash_node) * old->buf_sz);
	for (int i = 0; i < old->buf_sz; ++i) {
	 	struct hash_node* node_old = &old->buf[i];
	 	struct hash_node* node_new = &new->buf[i];
		if (old) {
			if (node_old->next) {
	 			int ptr_idx = node_old->next - old->buf;
	 			node_new->next = new->buf + ptr_idx;
			} else {
				node_new->next = NULL;
			}
		}
	}

	new->hash_sz = old->hash_sz;
	new->hashlist = (struct hash_node**)((intptr_t)(new + 1) + free_list_sz);
	for (int i = 0; i < new->hash_sz; ++i) {
		if (old->hashlist[i]) {
			int ptr_idx = old->hashlist[i] - old->buf;
			new->hashlist[i] = new->buf + ptr_idx;
		} else {
			new->hashlist[i] = NULL;
		}
	}

	free(old);
	hash->c = new;
}

static inline struct hash_node*
_new_hash_node(struct dtex_hash* hash) {
	if (hash->c->freelist) {
		struct hash_node* n = hash->c->freelist;
		hash->c->freelist = n->next;
		++hash->c->node_used;
		return n;
	} else {
		_enlarge_freelist(hash);
		return _new_hash_node(hash);
	}
}

static inline void
_enlarge_hashlist(struct dtex_hash* hash) {
	struct changeable* old = hash->c;

	int old_buf_sz = old->buf_sz;
	void* keys[old_buf_sz];
	void* vals[old_buf_sz];
	for (int i = 0; i < old_buf_sz; ++i) {
		struct hash_node* node = &hash->c->buf[i];
		if (node) {
			keys[i] = node->key;
			keys[i] = node->val;

			int zz = node->key;
			if (zz > 0 && zz < 100) {
				int z1 = 0;
			}

		} else {
			keys[i] = NULL;
		}
	}

	size_t new_hash_sz = _find_next_hash_sz(old->hash_sz);
	dtex_info(" [hash] rehash %d -> %d", old->hash_sz, new_hash_sz);
	size_t free_list_sz = sizeof(struct hash_node) * old->buf_sz;
	size_t hash_list_sz = sizeof(struct hash_node*) * new_hash_sz;
	struct changeable* new = (struct changeable*)malloc(sizeof(struct changeable) + free_list_sz + hash_list_sz);
	if (!new) {
		return;
	}
	
	memset(new + 1, 0, free_list_sz + hash_list_sz);

	new->buf = (struct hash_node*)(new + 1);
	new->buf_sz = old->buf_sz;
	new->freelist = new->buf;
	new->node_used = 0;

	free(old);
	hash->c = new;

	new->hash_sz = new_hash_sz;
	new->hashlist = (struct hash_node**)((intptr_t)(new + 1) + free_list_sz);
	for (int i = 0; i < old_buf_sz; ++i) {
		if (!keys[i]) {
			continue;
		}
		dtex_hash_insert(hash, keys[i], vals[i], true);
	}

	int zz = 0;
}

void 
dtex_hash_insert(struct dtex_hash* hash, void* key, void* val, bool force) {
	if (hash->rehash_weight != 0) {
// 		float weight = (float)hash->c->node_used / hash->c->hash_sz;
// 		if (weight > hash->rehash_weight) {
// 
// 			dtex_warning("dtex_hash_insert enlarge, free_used: %d, hash_sz: %d", hash->c->node_used, hash->c->hash_sz);
// 
// 			_enlarge_hashlist(hash);
// 		}
	}

	if (!force) {
		void* val = dtex_hash_query(hash, key);
		if (val) {
			return;
		}
	}

	struct hash_node* hn = _new_hash_node(hash);
	if (!hn) {
		return;
	}
	hn->key = key;
	hn->val = val;

	unsigned int idx = hash->hash_func(hash->c->hash_sz, key);
	hn->next = hash->c->hashlist[idx];
	hash->c->hashlist[idx] = hn;
}

void* 
dtex_hash_remove(struct dtex_hash* hash, void* key) {
	unsigned int idx = hash->hash_func(hash->c->hash_sz, key);
	struct hash_node* prev = NULL;
	struct hash_node* curr = hash->c->hashlist[idx];
	while (curr) {
		if (hash->equal_func(key, curr->key)) {
			break;
		}
		prev = curr;
		curr = curr->next;
	}

	if (!curr) {
		return NULL;
	}

	if (prev) {
		prev->next = curr->next;
	} else {
		hash->c->hashlist[idx] = curr->next;
	}

	curr->next = hash->c->freelist;
	hash->c->freelist = curr;
	assert(hash->c->node_used > 0);
	--hash->c->node_used;

	void* val = curr->val;
	memset(curr, 0, sizeof(struct hash_node));
	return val;
}

void 
dtex_hash_clear(struct dtex_hash* hash) {
	_init_changeable(hash->c, hash->c->buf_sz, hash->c->hash_sz);
}

unsigned int 
dtex_string_hash_func(int hash_sz, void* key) {
	const char* str = (const char*)key;

	// BKDR Hash Function
	unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
	unsigned int hash = 0;

	while (*str) {
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF) % hash_sz;
}

bool 
dtex_string_equal_func(void* key0, void* key1) {
	const char* str0 = (const char*)key0;
	const char* str1 = (const char*)key1;
	return (strcmp(str0, str1) == 0);
}