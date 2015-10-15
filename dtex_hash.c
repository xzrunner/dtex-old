#include "dtex_hash.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const size_t HASH_SZ_TBL[] = {
	11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51439, 102881
};

struct hash_node {
	struct hash_node* next;
	void* key;
	void* val;
};

struct changeable {
	struct hash_node* freelist;
	size_t free_next, free_cap;

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
_init_changeable(struct changeable* c, size_t free_sz, size_t hash_sz) {
	size_t free_list_sz = sizeof(struct hash_node) * free_sz;
	size_t hash_list_sz = sizeof(struct hash_node*) * hash_sz;

	memset(c, 0, free_list_sz + hash_list_sz);

 	c->free_next = 0;
 	c->free_cap = free_sz;
 	c->freelist = (struct hash_node*)(c + 1);
 	for (int i = 0; i < free_sz - 1; ++i) {
 		struct hash_node* hn = &c->freelist[i];
 		hn->next = &c->freelist[i + 1];
 	}
 	c->freelist[free_sz - 1].next = NULL;
 
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
	struct changeable* c = (struct changeable*)malloc(free_list_sz + hash_list_sz);
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

static inline void
_enlarge_freelist(struct dtex_hash* hash) {
	struct changeable* old = hash->c;

	size_t new_cap = old->free_cap * 2;
	size_t free_list_sz = sizeof(struct hash_node) * new_cap;
	size_t hash_list_sz = sizeof(struct hash_node*) * old->hash_sz;
	struct changeable* new = (struct changeable*)malloc(free_list_sz + hash_list_sz);
	if (!new) {
		return;
	}

	memset(new, 0, free_list_sz + hash_list_sz);

	new->free_next = old->free_next;
	new->free_cap = new_cap;
	new->freelist = (struct hash_node*)(new + 1);
	memcpy(new->freelist, old->freelist, sizeof(struct hash_node) * old->free_next);
	for (int i = 0; i < new->free_next; ++i) {
		struct hash_node* hn = &new->freelist[i];
		int ptr_idx = hn->next - old->freelist;
		hn->next = new->freelist + ptr_idx;
	}

	new->hash_sz = old->hash_sz;
	new->hashlist = (struct hash_node**)((intptr_t)new + sizeof(struct changeable) + free_list_sz);
	memcpy(new->hashlist, old->hashlist, sizeof(struct hash_node*) * old->hash_sz);
	for (int i = 0; i < new->hash_sz; ++i) {
		int ptr_idx = new->hashlist[i] - old->freelist;
		new->hashlist[i] = new->freelist + ptr_idx;
	}

	free(hash->c);
	hash->c = new;
}

static inline struct hash_node*
_new_hash_node(struct dtex_hash* hash) {
	if (hash->c->free_next == hash->c->free_cap) {
		_enlarge_freelist(hash);
		return _new_hash_node(hash);
	} else {
		return &hash->c->freelist[hash->c->free_next++];
	}
}

static inline void
_enlarge_hashlist(struct dtex_hash* hash) {
	struct changeable* old = hash->c;

	size_t new_hash_sz = _find_next_hash_sz(old->hash_sz);
	size_t free_list_sz = sizeof(struct hash_node) * old->free_cap;
	size_t hash_list_sz = sizeof(struct hash_node*) * new_hash_sz;
	struct changeable* new = (struct changeable*)malloc(free_list_sz + hash_list_sz);
	if (!new) {
		return;
	}
	
	memset(new, 0, free_list_sz + hash_list_sz);

	new->free_next = old->free_next;
	new->free_cap = old->free_cap;
	new->freelist = (struct hash_node*)(new + 1);
	memcpy(new->freelist, old->freelist, free_list_sz);

	free(hash->c);
	hash->c = new;

	new->hash_sz = old->hash_sz;
	new->hashlist = (struct hash_node**)((intptr_t)new + sizeof(struct changeable) + free_list_sz);
	for (int i = 0; i < new->free_next; ++i) {
		struct hash_node* hn = &new->freelist[i];
		dtex_hash_insert(hash, hn->key, hn->val, true);
	}
}

void 
dtex_hash_insert(struct dtex_hash* hash, void* key, void* val, bool force) {
	if (hash->rehash_weight != 0) {
		float weight = (float)hash->c->hash_sz / hash->c->free_cap;
		if (weight > hash->rehash_weight) {
			_enlarge_hashlist(hash);
		}
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

void 
dtex_hash_clear(struct dtex_hash* hash) {
	_init_changeable(hash->c, hash->c->free_cap, hash->c->hash_sz);
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