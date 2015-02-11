#include "dtex_rrr.h"
#include "dtex_alloc.h"
#include "dtex_packer.h"
#include "dtex_pvr.h"
#include "dtex_math.h"
#include "dtex_packer_ext.h"
#include "dtex_vector.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

	if (part->x < 0) {
		part->w += part->x;
		part->x = 0;
	}
	if (part->y < 0) {
		part->h += part->y;
		part->y = 0;
	}

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
//	dtex_rrr_load_texture(rrr);
	dtex_rrr_load_to_c3(rrr, NULL);

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
_load_picture_to_texture(uint8_t* texture, struct dp_pos* pos, struct rrr_picture* pic) {
	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
	int sx = pos->r.xmin >> 2,
		sy = pos->r.ymin >> 2;

	for (int i = 0; i < pic->part_sz; ++i) {
		struct rrr_part* part = &pic->part[i];

		int idx_src = 0;
		for (int y = part->y; y < part->y + part->h; ++y) {
			for (int x = part->x; x < part->x + part->w; ++x) {
				int idx_dst = dtex_pvr_get_morton_number(sx + x, sy + y);
				//assert(idx_dst < edge * edge / 16);
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
		struct dp_pos* pos = dtexpacker_add(packer, TO_4TIMES(pic->w), TO_4TIMES(pic->h), true);
		_load_picture_to_texture(buf, pos, pic);
	}

	// for test
	dtex_pvr_write_file("F:/debug/rpack/test/rrr.pvr", buf, edge, edge);

 	dtexpacker_release(packer);
	free(buf);
}

void 
dtex_rrr_load_to_c3(struct dtex_rrr* rrr, struct dtex_c3* c3) {
	size_t sz = rrr->pic_size;
	struct dp_rect rects[sz];
	for (int i = 0; i < sz; ++i) {
		struct rrr_picture* pic = &rrr->pictures[i];
		struct dp_rect* r = &rects[i];
		r->w = TO_4TIMES(pic->w);
		r->h = TO_4TIMES(pic->h);
		r->ud = &rrr->pictures[i];
		r->dst_packer_idx = -1;
		r->dst_pos = NULL;
	}

	struct dp_rect* rects_ptr[sz];
	for (int i = 0; i < sz; ++i) {
		rects_ptr[i] = &rects[i];
	}

	struct dtex_vector* packers = dtex_packer_square_multi(rects_ptr, sz);

	int packers_sz = dtex_vector_size(packers);
	uint8_t* bufs[packers_sz];
	for (int i = 0; i < packers_sz; ++i) {
		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(packers, i);
		int w, h;
		dtexpacker_get_size(packer, &w, &h);
		assert(w == h);
		bufs[i] = dtex_pvr_init_blank(w);
	}

	for (int i = 0; i < sz; ++i) {
		struct dp_rect* r = rects_ptr[i];
		assert(r->dst_pos && r->dst_packer_idx >= 0);
// 		if (!r->dst_pos) {
// 			continue;
// 		}
		_load_picture_to_texture(bufs[r->dst_packer_idx], r->dst_pos, (struct rrr_picture*)r->ud);
	}

	// for test
	for (int i = 0; i < packers_sz; ++i) {
		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(packers, i);
		int w, h;
		dtexpacker_get_size(packer, &w, &h);

		char str_buf[50];
		//sprintf(str_buf, "F:/debug/rpack/test/rrr_part%d.pvr", i);
		sprintf(str_buf, "E:/debug/rpack/rrr/rrr_part%d.pvr", i);
		dtex_pvr_write_file(str_buf, bufs[i], w, h);

		dtexpacker_release(packer);
		free(bufs[i]);
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
