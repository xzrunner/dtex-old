#include "dtex_alloc.h"

#include <stdlib.h>
#include <assert.h>

#ifdef _MSC_VER
#define EXPORT_DATA
#endif // _MSC_VER

#ifdef EXPORT_DATA

struct alloc {
	int sz;
};

struct alloc*
dtex_init_alloc(int sz) {
	struct alloc* a = malloc(sizeof(*a));
	a->sz = 0;
	return a;
}

void*
dtex_alloc(struct alloc* a, int sz) {
	if (sz & 3) {
		sz = (sz + 3) & ~3;
	}
	a->sz += sz;
	return malloc(sz);
}

int 
dtex_alloc_size(struct alloc* a) {
	return a->sz;
}

#else

struct alloc {
	int sz;
	char* free;
};

struct alloc*
dtex_init_alloc(int sz) {
	struct alloc* a = malloc(sizeof(*a) + sz); 
	a->sz = sz;
	a->free = (char*)(a + 1);
	return a;
}

void*
dtex_alloc(struct alloc* a, int sz) {
	if (sz & 3) {
		sz = (sz + 3) & ~3;
	}
	assert(sz <= a->sz);
	void* ret = a->free;
	a->sz -= sz;
	a->free += sz;
	return ret;
} 

int 
dtex_alloc_size(struct alloc* a) {
	return a->sz;
}

#endif // EXPORT_DATA