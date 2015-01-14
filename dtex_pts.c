#include "dtex_pts.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef EXPORT_PTS

struct alloc {
	int sz;
};

static inline struct alloc*
_init_alloc(int sz) {
	struct alloc* a = malloc(sizeof(*a));
	a->sz = 0;
	return a;
}

static inline void*
_alloc(struct alloc* a, int sz) {
	if (sz & 3) {
		sz = (sz + 3) & ~3;
	}
	a->sz += sz;
	return malloc(sz);
}

#else

struct alloc {
	int sz;
	char* free;
};

static inline struct alloc*
_init_alloc(int sz) {
	struct alloc* a = malloc(sizeof(*a) + sz); 
	a->sz = sz;
	a->free = (char*)(a + 1);
	return a;
}

static inline void*
_alloc(struct alloc* a, int sz) {
	if (sz & 3) {
		sz = (sz + 3) & ~3;
	}
	assert(sz <= a->sz);
	void* ret = a->free;
	a->sz -= sz;
	a->free += sz;
	return ret;
} 

#endif // EXPORT_PTS

struct dtex_pts {
	struct alloc* alloc;

	int16_t pic_size;
	struct pts_picture pictures[0];
};

static inline void
_decode_strip(struct dtex_pts* pts, struct pts_strip* strip, uint8_t** buf) {
	uint8_t* ptr = *buf;

	int16_t sz;
	memcpy(&sz, ptr, sizeof(sz));
	ptr += sizeof(sz);

	strip->pos_size = sz;
	strip->vertices = _alloc(pts->alloc, sz * sizeof(int16_t) * 2);
	for (int i = 0; i < sz; ++i) {
		memcpy(&strip->vertices[i*2], ptr, sizeof(int16_t));
		ptr += sizeof(int16_t);
		memcpy(&strip->vertices[i*2+1], ptr, sizeof(int16_t));
		ptr += sizeof(int16_t);
	}

	*buf = ptr;
}

static inline void
_decode_picture(struct dtex_pts* pts, struct pts_picture* pic, uint8_t** buf) {
	uint8_t* ptr = *buf;

	int16_t id, sz;
	memcpy(&id, ptr, sizeof(id));
	ptr += sizeof(id);
	memcpy(&sz, ptr, sizeof(sz));
	ptr += sizeof(sz);

	pic->id = id;
	pic->strips = _alloc(pts->alloc, sz * sizeof(struct pts_strip));
	pic->strip_size = sz;
	for (int i = 0; i < sz; ++i) {
		_decode_strip(pts, &pic->strips[i], &ptr);
	}

	*buf = ptr;
}

struct dtex_pts* 
dtex_pts_create(void* data, int sz, int cap) {
	uint8_t* ptr = data;

	int16_t pic_sz;
	memcpy(&pic_sz, ptr, sizeof(pic_sz));
	ptr += sizeof(pic_sz);

	struct alloc* a = _init_alloc(cap);
	struct dtex_pts* pts = _alloc(a, sizeof(*pts) + pic_sz * sizeof(struct pts_picture));
	pts->alloc = a;

	pts->pic_size = pic_sz;
	for (int i = 0; i < pic_sz; ++i) {
		_decode_picture(pts, &pts->pictures[i], &ptr);
	}

	return pts;
}

void 
dtex_pts_release(struct dtex_pts* pts) {
	if (pts) {
		assert(pts->alloc);
		free(pts->alloc);
	}
}

struct pts_picture* 
dtex_pts_get_pic(struct dtex_pts* pts, int id) {
	assert(id > 0);
	if (id > pts->pic_size) {
		return NULL;
	} else {
		return &pts->pictures[id - 1];
	}
}

#ifdef EXPORT_PTS

size_t 
dtex_pts_size(void* data, int sz) {
	struct dtex_pts* pts = dtex_pts_create(data, sz, 0);
	size_t ret = pts->alloc->sz;
	dtex_pts_release(pts);
	return ret;
}

#endif // EXPORT_PTS
