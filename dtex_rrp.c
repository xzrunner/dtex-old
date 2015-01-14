#include "dtex_rrp.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAX_TEXTURE_SIZE 8

#ifdef EXPORT_RRP

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

#endif // EXPORT_RRP

struct dtex_rrp {
	struct alloc* alloc;

	// struct dtex_raw_tex textures[MAX_TEXTURE_SIZE];
	// int16_t tex_size;

	int16_t pic_size;
	struct dr_picture pictures[0];
};

static inline void
_decode_part(struct dtex_rrp* rrp, struct dr_part* part, uint8_t** buf) {
	uint8_t* ptr = *buf;

	memcpy(&part->src.x, ptr, sizeof(part->src.x));
	ptr += sizeof(part->src.x);
	memcpy(&part->src.y, ptr, sizeof(part->src.x));
	ptr += sizeof(part->src.y);
	memcpy(&part->dst.x, ptr, sizeof(part->src.x));
	ptr += sizeof(part->dst.x);
	memcpy(&part->dst.y, ptr, sizeof(part->src.x));
	ptr += sizeof(part->dst.y);

	int16_t w, h;
	memcpy(&w, ptr, sizeof(w));
	ptr += sizeof(w);
	memcpy(&h, ptr, sizeof(h));
	ptr += sizeof(h);
	if (w < 0 && h < 0) {
		part->is_rotated = true;
		part->src.w = -w; part->src.h = -h;
		part->dst.w = -h; part->dst.h = -w;
		// part->src.w = part->dst.w = -w;
		// part->src.h = part->dst.h = -h;
	} else {
		assert(w > 0 && h > 0);
		part->is_rotated = false;
		part->src.w = part->dst.w = w;
		part->src.h = part->dst.h = h;		
	}

	*buf = ptr;
}

static inline void
_decode_picture(struct dtex_rrp* rrp, struct dr_picture* pic, uint8_t** buf) {
	uint8_t* ptr = *buf;

	memcpy(&pic->id, ptr, sizeof(pic->id));
	ptr += sizeof(pic->id);
	memcpy(&pic->w, ptr, sizeof(pic->w));
	ptr += sizeof(pic->w);
	memcpy(&pic->h, ptr, sizeof(pic->h));
	ptr += sizeof(pic->h);
	memcpy(&pic->part_sz, ptr, sizeof(pic->part_sz));
	ptr += sizeof(pic->part_sz);

	pic->part = _alloc(rrp->alloc, pic->part_sz * sizeof(struct dr_part));
	for (int i = 0; i < pic->part_sz; ++i) {
		_decode_part(rrp, &pic->part[i], &ptr);
	}

	*buf = ptr;
}

struct dtex_rrp* 
dtex_rrp_create(void* data, int sz, int cap) {
	uint8_t* ptr = data;

	int32_t pic_sz;
	memcpy(&pic_sz, ptr, sizeof(pic_sz));
	ptr += sizeof(pic_sz);

	struct alloc* a = _init_alloc(cap);
	struct dtex_rrp* rrp = _alloc(a, sizeof(*rrp) + pic_sz * sizeof(struct dr_picture));
	rrp->alloc = a;

	rrp->pic_size = pic_sz;
	for (int i = 0; i < pic_sz; ++i) {
		_decode_picture(rrp, &rrp->pictures[i], &ptr);
	}
	
	return rrp;
}

void 
dtex_rrp_release(struct dtex_rrp* rrp) {
	if (rrp) {
		assert(rrp->alloc);
		free(rrp->alloc);
	}
}

// id also is index
struct dr_picture* 
dtex_rrp_get_pic(struct dtex_rrp* rrp, int id) {
	assert(id > 0 && id <= rrp->pic_size);
	return &rrp->pictures[id - 1];
}

#ifdef EXPORT_RRP

size_t 
dtex_rrp_size(void* data, int sz) {
	struct dtex_rrp* rrp = dtex_rrp_create(data, sz, 0);
	size_t ret = rrp->alloc->sz;
	dtex_rrp_release(rrp);
	return ret;
}

#endif // EXPORT_RRP
