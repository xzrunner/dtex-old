#include "dtex_rrr.h"
#include "dtex_alloc.h"
#include "dtex_packer.h"
#include "dtex_pvr.h"
#include "dtex_math.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

	// for test
	dtex_rrr_load_texture(rrr);

	return rrr;
}

void 
dtex_rrr_release(struct dtex_rrr* rrr) {
	if (rrr) {
		assert(rrr->alloc);
		free(rrr->alloc);
	}
}

static inline void
_load_picture_to_texture(uint8_t* texture, int edge, struct dtex_packer* packer, struct rrr_picture* pic) {
	struct dp_position* pos = dtexpacker_add(packer, TO_4TIMES(pic->w), TO_4TIMES(pic->h));
	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
	int sx = pos->r.xmin >> 2,
		sy = pos->r.ymin >> 2;

	for (int i = 0; i < pic->part_sz; ++i) {
		struct rrr_part* part = &pic->part[i];

		int idx_src = 0;
		for (int y = part->y; y < part->y + part->h; ++y) {
			for (int x = part->x; x < part->x + part->w; ++x) {
				int idx_dst = dtex_pvr_get_morton_number(sx + x, sy + y);
				assert(idx_dst < edge * edge / 16);
				int64_t* src = (int64_t*)part->data + idx_src;
				int64_t* dst = (int64_t*)texture + idx_dst;
				memcpy(dst, src, sizeof(int64_t));
				++idx_src;
			}
		}
	}
}

void 
dtex_rrr_load_texture(struct dtex_rrr* rrr) {
	int edge = 1024;
	uint8_t* buf = dtex_pvr_init_blank(edge);

	struct dtex_packer* packer = dtexpacker_create(edge, edge, 100);
	for (int i = 0; i < rrr->pic_size; ++i) {
		struct rrr_picture* pic = &rrr->pictures[i];
		_load_picture_to_texture(buf, edge, packer, pic);
	}

	// for test
	dtex_pvr_write_file("F:/debug/rpack/test/rrr.pvr", buf, edge, edge);

 	dtexpacker_release(packer);
	free(buf);
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
