#include "dtex_rrr.h"
#include "dtex_alloc.h"

#include <assert.h>
#include <stdlib.h>

struct dtex_rrr {
	struct alloc* alloc;

	int16_t pic_size;
	struct rrr_picture pictures[0];
};

static inline void
_decode_part(struct dtex_rrr* rrr, struct rrr_part* part, uint8_t** buf) {
	uint8_t* ptr = *buf;

	memcpy(&part->x, ptr, sizeof(part->x));
	ptr += sizeof(part->x);
	memcpy(&part->y, ptr, sizeof(part->x));
	ptr += sizeof(part->y);
	memcpy(&part->w, ptr, sizeof(part->w));
	ptr += sizeof(part->w);
	memcpy(&part->h, ptr, sizeof(part->h));
	ptr += sizeof(part->h);

	size_t sz = sizeof(uint64_t) * part->w * part->h;
	part->data = dtex_alloc(rrr->alloc, sz);
	memcpy(part->data, ptr, sz);
	ptr += sz;

	*buf = ptr;
}

static inline void
_decode_picture(struct dtex_rrr* rrr, struct rrr_picture* pic, uint8_t** buf) {
	uint8_t* ptr = *buf;

	memcpy(&pic->id, ptr, sizeof(pic->id));
	ptr += sizeof(pic->id);
	memcpy(&pic->w, ptr, sizeof(pic->w));
	ptr += sizeof(pic->w);
	memcpy(&pic->h, ptr, sizeof(pic->h));
	ptr += sizeof(pic->h);
	memcpy(&pic->part_sz, ptr, sizeof(pic->part_sz));
	ptr += sizeof(pic->part_sz);

	pic->part = dtex_alloc(rrr->alloc, pic->part_sz * sizeof(struct rrr_part));
	for (int i = 0; i < pic->part_sz; ++i) {
		_decode_part(rrr, &pic->part[i], &ptr);
	}

	*buf = ptr;
}

struct dtex_rrr* 
dtex_rrr_create(void* data, int sz, int cap) {
	uint8_t* ptr = data;

	int32_t pic_sz;
	memcpy(&pic_sz, ptr, sizeof(pic_sz));
	ptr += sizeof(pic_sz);

	struct alloc* a = dtex_init_alloc(cap);
	struct dtex_rrr* rrr = dtex_alloc(a, sizeof(*rrr) + pic_sz * sizeof(struct rrr_picture));
	rrr->alloc = a;

	rrr->pic_size = pic_sz;
	for (int i = 0; i < pic_sz; ++i) {
		_decode_picture(rrr, &rrr->pictures[i], &ptr);		
	}

	return rrr;
}

void 
dtex_rrr_release(struct dtex_rrr* rrr) {
	if (rrr) {
		assert(rrr->alloc);
		free(rrr->alloc);
	}
}

#ifdef EXPORT_RRR

size_t 
dtex_rrr_size(void* data, int sz) {
	struct dtex_rrr* rrr = dtex_rrr_create(data, sz, 0);
	size_t size = dtex_alloc_size(rrr->alloc);
	dtex_rrr_release(rrr);
	return size;
}

#endif // EXPORT_RRR
