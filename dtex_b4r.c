#include "dtex_b4r.h"
#include "dtex_alloc.h"
#include "dtex_packer.h"
#include "dtex_pvr.h"
#include "dtex_math.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct dtex_b4r {
	struct alloc* alloc;

	int16_t pic_size;
	struct b4r_picture pictures[0];
};

static inline void
_decode_picture(struct dtex_b4r* b4r, struct b4r_picture* pic, uint8_t** buf) {
	uint8_t* ptr = *buf;

	memcpy(&pic->id, ptr, sizeof(pic->id));
	ptr += sizeof(pic->id);
	memcpy(&pic->w, ptr, sizeof(pic->w));
	ptr += sizeof(pic->w);
	memcpy(&pic->h, ptr, sizeof(pic->h));
	ptr += sizeof(pic->h);

	assert(pic->w == pic->h && pic->w % 4 == 0);
	int block_sz = (pic->w >> 2) * (pic->w >> 2);
	int flag_sz = ceil(block_sz / 8.0f);
	pic->flag = dtex_alloc(b4r->alloc, flag_sz);
	memcpy(pic->flag, ptr, flag_sz);
	ptr += flag_sz;

	int data_sz = 0;
	for (int i = 0; i < block_sz; ++i) {
		if (pic->flag[i / 8] & (1 << (i % 8))) {
			data_sz += sizeof(int64_t);
		}
	}
	pic->pixels = dtex_alloc(b4r->alloc, data_sz);
	memcpy(pic->pixels, ptr, data_sz);
	ptr += data_sz;

	*buf = ptr;
}

struct dtex_b4r* 
dtex_b4r_create(void* data, int sz, int cap) {
	uint8_t* ptr = data;

	int32_t pic_sz;
	memcpy(&pic_sz, ptr, sizeof(pic_sz));
	ptr += sizeof(pic_sz);

	struct alloc* a = dtex_init_alloc(cap);
	struct dtex_b4r* b4r = dtex_alloc(a, sizeof(*b4r) + pic_sz * sizeof(struct b4r_picture));
	b4r->alloc = a;

	b4r->pic_size = pic_sz;
	for (int i = 0; i < pic_sz; ++i) {
		_decode_picture(b4r, &b4r->pictures[i], &ptr);		
	}

	// for test
	dtex_b4r_load_texture(b4r);

	return b4r;
}

void 
dtex_b4r_release(struct dtex_b4r* b4r) {
	if (b4r) {
		assert(b4r->alloc);
		free(b4r->alloc);
	}
}

static inline void
_load_picture_to_texture(uint8_t* texture, int edge, struct dtex_packer* packer, struct b4r_picture* pic) {
	struct dp_position* pos = dtexpacker_add(packer, TO_4TIMES(pic->w), TO_4TIMES(pic->h));
	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
	int sx = pos->r.xmin >> 2,
		sy = pos->r.ymin >> 2;

	int block = pic->w >> 2;
	int64_t* ptr_data = (int64_t*)pic->pixels;
	for (int y = 0; y < block; ++y) {
		for (int x = 0; x < block; ++x) {
			int idx = y * block + x;
			if (pic->flag[idx / 8] & (1 << (idx % 8))) {
				int idx_dst = dtex_pvr_get_morton_number(sx + x, sy + y);
				assert(idx_dst < edge * edge / 16);
				int64_t* dst = (int64_t*)texture + idx_dst;
				memcpy(dst, ptr_data, sizeof(int64_t));
				++ptr_data;
			}
		}
	}
}

void 
dtex_b4r_load_texture(struct dtex_b4r* b4r) {
	int edge = 1024;
	uint8_t* buf = dtex_pvr_init_blank(edge);

	struct dtex_packer* packer = dtexpacker_create(edge, edge, 100);
	for (int i = 0; i < b4r->pic_size; ++i) {
		struct b4r_picture* pic = &b4r->pictures[i];
		_load_picture_to_texture(buf, edge, packer, pic);
	}

	// for test
	dtex_pvr_write_file("F:/debug/rpack/test/b4r.pvr", buf, edge, edge);

	dtexpacker_release(packer);
	free(buf);
}

#ifdef EXPORT_RRR

size_t 
dtex_b4r_size(void* data, int sz) {
	struct dtex_b4r* b4r = dtex_b4r_create(data, sz, 0);
	size_t size = dtex_alloc_size(b4r->alloc);
	dtex_b4r_release(b4r);
	return size;
}

#endif // EXPORT_RRR
