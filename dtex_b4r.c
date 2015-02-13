#include "dtex_b4r.h"
#include "dtex_alloc.h"
#include "dtex_packer.h"
#include "dtex_pvr.h"
#include "dtex_math.h"
#include "dtex_packer_ext.h"
#include "dtex_vector.h"
#include "dtex_loader.h"
#include "dtex_gl.h"

#include "package.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct b4r_picture {
	int16_t id;
	int16_t w, h;

	int16_t x, y;

	uint8_t* pixels;
	uint8_t* flag;

	struct dp_rect dst_r;
};

struct dtex_b4r {
	struct alloc* alloc;

	struct dtex_vector* packers;

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

	int w_p2 = next_p2(pic->w),
		h_p2 = next_p2(pic->h);
	int edge = w_p2 > h_p2 ? w_p2 : h_p2;

	int block_sz = (edge >> 2) * (edge >> 2);
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

static inline struct dtex_vector*
_pack_pictures(struct dtex_b4r* b4r) {
	size_t sz = b4r->pic_size;
	for (int i = 0; i < sz; ++i) {
		struct b4r_picture* pic = &b4r->pictures[i];
		struct dp_rect* r = &pic->dst_r;
		r->w = TO_4TIMES(pic->w);
		r->h = TO_4TIMES(pic->h);
		r->ud = &b4r->pictures[i];
		r->dst_packer_idx = -1;
		r->dst_pos = NULL;
	}

	struct dp_rect* rects_ptr[sz];
	for (int i = 0; i < sz; ++i) {
		rects_ptr[i] = &b4r->pictures[i].dst_r;
	}

	return dtex_packer_square_multi(rects_ptr, sz);
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

// 	// for test
// 	dtex_b4r_load_texture(b4r);

	b4r->packers = _pack_pictures(b4r);

	return b4r;
}

static inline void
_release_packers(struct dtex_b4r* b4r) {
	if (!b4r->packers) return;

	int sz = dtex_vector_size(b4r->packers);
	for (int i = 0; i < sz; ++i) {
		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, i);
		dtexpacker_release(packer);
	}
	dtex_vector_release(b4r->packers);	
}

void 
dtex_b4r_release(struct dtex_b4r* b4r) {
	if (b4r) {
		_release_packers(b4r);

		assert(b4r->alloc);
		free(b4r->alloc);
	}
}

static inline void
_load_picture_to_texture(uint8_t* texture, struct dp_pos* pos, struct b4r_picture* pic) {
	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
	int sx = pos->r.xmin >> 2,
		sy = pos->r.ymin >> 2;

	int w_p2 = next_p2(pic->w),
		h_p2 = next_p2(pic->h);
	int _edge = w_p2 > h_p2 ? w_p2 : h_p2;
	int block = _edge >> 2;

	int64_t* ptr_data = (int64_t*)pic->pixels;
	for (int y = 0; y < block; ++y) {
		for (int x = 0; x < block; ++x) {
			int idx = y * block + x;
			if (pic->flag[idx / 8] & (1 << (idx % 8))) {
				int idx_dst = dtex_pvr_get_morton_number(sx + x, sy + y);
				int64_t* dst = (int64_t*)texture + idx_dst;
				memcpy(dst, ptr_data, sizeof(int64_t));
				++ptr_data;
			}
		}
	}
}

// static inline void
// _load_picture_to_texture(uint8_t* texture, int edge, struct dtex_packer* packer, struct b4r_picture* pic) {
// 	struct dp_pos* pos = dtexpacker_add(packer, TO_4TIMES(pic->w), TO_4TIMES(pic->h), true);
// 	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
// 	int sx = pos->r.xmin >> 2,
// 		sy = pos->r.ymin >> 2;
// 
// 	int w_p2 = next_p2(pic->w),
// 		h_p2 = next_p2(pic->h);
// 	int _edge = w_p2 > h_p2 ? w_p2 : h_p2;
// 	int block = _edge >> 2;
// 
// 	int64_t* ptr_data = (int64_t*)pic->pixels;
// 	for (int y = 0; y < block; ++y) {
// 		for (int x = 0; x < block; ++x) {
// 			int idx = y * block + x;
// 			if (pic->flag[idx / 8] & (1 << (idx % 8))) {
// 				int idx_dst = dtex_pvr_get_morton_number(sx + x, sy + y);
// 				assert(idx_dst < edge * edge / 16);
// 				int64_t* dst = (int64_t*)texture + idx_dst;
// 				memcpy(dst, ptr_data, sizeof(int64_t));
// 				++ptr_data;
// 			}
// 		}
// 	}
// }

// void 
// dtex_b4r_load_texture(struct dtex_b4r* b4r) {
// 	int edge = 1024;
// 	uint8_t* buf = dtex_pvr_init_blank(edge);
// 
// 	struct dtex_packer* packer = dtexpacker_create(edge, edge, 100);
// 	for (int i = 0; i < b4r->pic_size; ++i) {
// 		struct b4r_picture* pic = &b4r->pictures[i];
// 		_load_picture_to_texture(buf, edge, packer, pic);
// 	}
// 
// 	// for test
// 	dtex_pvr_write_file("F:/debug/rpack/test/b4r.pvr", buf, edge, edge);
// 
// 	dtexpacker_release(packer);
// 	free(buf);
// }

void 
dtex_b4r_preload_to_pkg(struct dtex_b4r* b4r, struct dtex_package* pkg) {
	int packers_sz = dtex_vector_size(b4r->packers);
	for (int i = 0; i < packers_sz; ++i) {
		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, i);
		int w, h;
		dtexpacker_get_size(packer, &w, &h);
		assert(w == h);

		struct dtex_raw_tex* dst = &pkg->textures[pkg->tex_size++];
		dst->id = dst->id_alpha = 0;
		dst->width = w;
		dst->height = h;
		dst->format = TEXTURE8;
	}
}

struct dtex_raw_tex* 
dtex_b4r_load_tex(struct dtex_b4r* b4r, struct dtex_package* pkg, int tex_idx) {
	struct dtex_raw_tex* tex = &pkg->textures[tex_idx];

	struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, tex_idx);
	int w, h;
	dtexpacker_get_size(packer, &w, &h);
	assert(w == h);
	uint8_t* buf = dtex_pvr_init_blank(w);
	for (int i = 0; i < b4r->pic_size; ++i) {
		struct dp_rect* r = &b4r->pictures[i].dst_r;
		assert(r->dst_pos && r->dst_packer_idx >= 0);
		if (r->dst_packer_idx == tex_idx) {
			_load_picture_to_texture(buf, r->dst_pos, (struct b4r_picture*)r->ud);
		}
	}

	tex->width = w;
	tex->height = h;
	tex->id = dtex_pvr_gen_texture(buf, COMPRESSED_RGBA_PVRTC_4BPPV1_IMG, w, h);
	assert(tex->id != 0);
	tex->id_alpha = 0;
	tex->format = PVRTC;

	return tex;
}

void 
dtex_b4r_relocate(struct dtex_b4r* b4r, struct dtex_package* pkg) {
	struct ejoypic* ep = pkg->ej_pkg->ep;
	for (int id = 0; id < ep->max_id; ++id) {
		struct animation * ani = ep->spr[id];
		if (ani == NULL || ani->part_n > 0) {
			continue;
		}

		struct picture* pic = (struct picture*)ani;
		for (int j = 0; j < -pic->n; ++j) {
			struct picture_part* part = &pic->part[j];
			assert(part->src[0] < 0);
			int idx = -part->src[0];
			assert(idx > 0 && idx <= b4r->pic_size);
			struct b4r_picture* rp = &b4r->pictures[idx-1];
			struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, rp->dst_r.dst_packer_idx);

			int w, h;
			dtexpacker_get_size(packer, &w, &h);

			struct dtex_rect* dr = &rp->dst_r.dst_pos->r;
			part->src[0] = part->src[2] = dr->xmin;
			part->src[4] = part->src[6] = dr->xmax;
			part->src[1] = part->src[7] = h - dr->ymin;
			part->src[3] = part->src[5] = h - dr->ymax;
			part->texid = rp->dst_r.dst_packer_idx;
		}
	}
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
