//#include "dtex_b4r.h"
//#include "dtex_alloc.h"
//#include "dtex_packer.h"
//#include "dtex_pvr.h"
//#include "dtex_etc1.h"
//#include "dtex_math.h"
//#include "dtex_packer_ext.h"
//#include "dtex_vector.h"
//#include "dtex_loader.h"
//#include "dtex_gl.h"
//
//#include "ejoy2d.h"
//
//#include <assert.h>
//#include <stdlib.h>
//#include <string.h>
//#include <math.h>
//
//static const uint16_t TEX_PVR = 1;
//static const uint16_t TEX_ETC1 = 2;
//
//struct b4r_picture {
//	int16_t id;
//	int16_t w, h;
//
//	int16_t x, y;
//
//	uint8_t* pixels;
//	uint8_t* flag;
//
//	struct dp_rect dst_r;
//};
//
//struct dtex_b4r {
//	struct alloc* alloc;
//
//	uint16_t tex_type;
//
//	struct dtex_vector* packers;
//
//	int16_t pic_size;
//	struct b4r_picture pictures[0];
//};
//
//static inline void
//_decode_picture(struct dtex_b4r* b4r, struct b4r_picture* pic, uint8_t** buf) {
//	uint8_t* ptr = *buf;
//
//	memcpy(&pic->id, ptr, sizeof(pic->id));
//	ptr += sizeof(pic->id);
//	memcpy(&pic->w, ptr, sizeof(pic->w));
//	ptr += sizeof(pic->w);
//	memcpy(&pic->h, ptr, sizeof(pic->h));
//	ptr += sizeof(pic->h);
//
//	// flag
//	int block_sz;
//	int w_p2 = next_p2(pic->w),
//		h_p2 = next_p2(pic->h);
//	if (b4r->tex_type == TEX_PVR) {
//		int edge = w_p2 > h_p2 ? w_p2 : h_p2;
//		block_sz = (edge >> 2) * (edge >> 2);
//	} else {
//		block_sz = (w_p2 >> 2) * (h_p2 >> 2);
//	}
//	int flag_sz = ceil(block_sz / 8.0f);
//	pic->flag = dtex_alloc(b4r->alloc, flag_sz);
//	memcpy(pic->flag, ptr, flag_sz);
//	ptr += flag_sz;
//
//	// pixels
//	int block_used_count = 0;
//	for (int i = 0; i < block_sz; ++i) {
//		if (pic->flag[i / 8] & (1 << (i % 8))) {
//			++block_used_count;
//		}
//	}
//	int data_sz = block_used_count * sizeof(int64_t);
//	if (b4r->tex_type == TEX_ETC1) {
//		data_sz *= 2;
//	}
//	pic->pixels = dtex_alloc(b4r->alloc, data_sz);
//	memcpy(pic->pixels, ptr, data_sz);
//	ptr += data_sz;
//
//	*buf = ptr;
//}
//
//static inline struct dtex_vector*
//_pack_pictures(struct dtex_b4r* b4r) {
//	size_t sz = b4r->pic_size;
//	for (int i = 0; i < sz; ++i) {
//		struct b4r_picture* pic = &b4r->pictures[i];
//		struct dp_rect* r = &pic->dst_r;
//		r->w = TO_4TIMES(pic->w);
//		r->h = TO_4TIMES(pic->h);
//		r->ud = &b4r->pictures[i];
//		r->dst_packer_idx = -1;
//		r->dst_pos = NULL;
//	}
//
//	struct dp_rect* rects_ptr[sz];
//	for (int i = 0; i < sz; ++i) {
//		rects_ptr[i] = &b4r->pictures[i].dst_r;
//	}
//
//	return dtex_packer_square_multi(rects_ptr, sz);
//}
//
//struct dtex_b4r* 
//dtex_b4r_create(void* data, int sz, int cap) {
//	uint8_t* ptr = data;
//
//	uint16_t pic_sz;
//	memcpy(&pic_sz, ptr, sizeof(pic_sz));
//	ptr += sizeof(pic_sz);
//
//	struct alloc* a = dtex_init_alloc(cap);
//	struct dtex_b4r* b4r = dtex_alloc(a, sizeof(*b4r) + pic_sz * sizeof(struct b4r_picture));
//	b4r->alloc = a;
//
//	memcpy(&b4r->tex_type, ptr, sizeof(b4r->tex_type));
//	ptr += sizeof(b4r->tex_type);
//
//	b4r->pic_size = pic_sz;
//	for (int i = 0; i < pic_sz; ++i) {
//		_decode_picture(b4r, &b4r->pictures[i], &ptr);		
//	}
//
//// 	// for test
//// 	dtex_b4r_load_texture(b4r);
//
//	b4r->packers = _pack_pictures(b4r);
//
//	return b4r;
//}
//
//static inline void
//_release_packers(struct dtex_b4r* b4r) {
//	if (!b4r->packers) return;
//
//	int sz = dtex_vector_size(b4r->packers);
//	for (int i = 0; i < sz; ++i) {
//		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, i);
//		dtexpacker_release(packer);
//	}
//	dtex_vector_release(b4r->packers);	
//}
//
//void 
//dtex_b4r_release(struct dtex_b4r* b4r) {
//	if (b4r) {
//		_release_packers(b4r);
//
//		assert(b4r->alloc);
//		free(b4r->alloc);
//	}
//}
//
//static inline void
//_load_picture_to_pvr_tex(uint8_t* texture, struct dp_pos* pos, struct b4r_picture* pic) {
//	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
//	int sx = pos->r.xmin >> 2,
//		sy = pos->r.ymin >> 2;
//
//	int w_p2 = next_p2(pic->w),
//		h_p2 = next_p2(pic->h);
//	int _edge = w_p2 > h_p2 ? w_p2 : h_p2;
//	int block = _edge >> 2;
//
//	int64_t* ptr_data = (int64_t*)pic->pixels;
//	for (int y = 0; y < block; ++y) {
//		for (int x = 0; x < block; ++x) {
//			int idx = y * block + x;
//			if (pic->flag[idx / 8] & (1 << (idx % 8))) {
//				int idx_dst = dtex_pvr_get_morton_number(sx + x, sy + y);
//				int64_t* dst = (int64_t*)texture + idx_dst;
//				memcpy(dst, ptr_data, sizeof(int64_t));
//				++ptr_data;
//			}
//		}
//	}
//}
//
//static inline void
//_load_picture_to_etc1_tex(uint8_t* texture, int w, int h, struct dp_pos* pos, struct b4r_picture* pic) {
//	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
//	int sx = pos->r.xmin >> 2,
//		sy = pos->r.ymin >> 2;
//
//	int packer_bw = w >> 2,
//		packer_bh = h >> 2;
//	int bw = next_p2(pic->w) >> 2,
//		bh = next_p2(pic->h) >> 2;
//	int64_t* ptr_data = (int64_t*)pic->pixels;
//	for (int y = 0; y < bh; ++y) {
//		for (int x = 0; x < bw; ++x) {
//			int idx = y * bw + x;
//			if (pic->flag[idx / 8] & (1 << (idx % 8))) {
//				int idx_dst = (sy + y) * packer_bw + sx + x;
//
//				int64_t* dst_rgb = (int64_t*)texture + idx_dst;
//				memcpy(dst_rgb, ptr_data++, sizeof(int64_t));
//
//				int64_t* dst_alpha = (int64_t*)texture + packer_bw * packer_bh + idx_dst;
//				memcpy(dst_alpha, ptr_data++, sizeof(int64_t));
//			}
//		}
//	}
//}
//
//// static inline void
//// _load_picture_to_pvr_tex(uint8_t* texture, int edge, struct dtex_packer* packer, struct b4r_picture* pic) {
//// 	struct dp_pos* pos = dtexpacker_add(packer, TO_4TIMES(pic->w), TO_4TIMES(pic->h), true);
//// 	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
//// 	int sx = pos->r.xmin >> 2,
//// 		sy = pos->r.ymin >> 2;
//// 
//// 	int w_p2 = next_p2(pic->w),
//// 		h_p2 = next_p2(pic->h);
//// 	int _edge = w_p2 > h_p2 ? w_p2 : h_p2;
//// 	int block = _edge >> 2;
//// 
//// 	int64_t* ptr_data = (int64_t*)pic->pixels;
//// 	for (int y = 0; y < block; ++y) {
//// 		for (int x = 0; x < block; ++x) {
//// 			int idx = y * block + x;
//// 			if (pic->flag[idx / 8] & (1 << (idx % 8))) {
//// 				int idx_dst = dtex_pvr_get_morton_number(sx + x, sy + y);
//// 				assert(idx_dst < edge * edge / 16);
//// 				int64_t* dst = (int64_t*)texture + idx_dst;
//// 				memcpy(dst, ptr_data, sizeof(int64_t));
//// 				++ptr_data;
//// 			}
//// 		}
//// 	}
//// }
//
//// void 
//// dtex_b4r_load_texture(struct dtex_b4r* b4r) {
//// 	int edge = 1024;
//// 	uint8_t* buf = dtex_pvr_init_blank(edge);
//// 
//// 	struct dtex_packer* packer = dtexpacker_create(edge, edge, 100);
//// 	for (int i = 0; i < b4r->pic_size; ++i) {
//// 		struct b4r_picture* pic = &b4r->pictures[i];
//// 		_load_picture_to_pvr_tex(buf, edge, packer, pic);
//// 	}
//// 
//// 	// for test
//// 	dtex_pvr_write_file("F:/debug/rpack/test/b4r.pvr", buf, edge, edge);
//// 
//// 	dtexpacker_release(packer);
//// 	free(buf);
//// }
//
//void 
//dtex_b4r_preload_to_pkg(struct dtex_b4r* b4r, struct dtex_package* pkg) {
//	int packers_sz = dtex_vector_size(b4r->packers);
//	for (int i = 0; i < packers_sz; ++i) {
//		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, i);
//		int w, h;
//		dtexpacker_get_size(packer, &w, &h);
//		assert(w == h);
//
//		struct dtex_raw_tex* dst = &pkg->textures[pkg->tex_size++];
//		dst->id = dst->id_alpha = 0;
//		dst->width = w;
//		dst->height = h;
//		dst->format = TEXTURE8;
//	}
//}
//
//static inline struct dtex_raw_tex* 
//_load_pvr_tex(struct dtex_b4r* b4r, struct dtex_package* pkg, int tex_idx) {
//	struct dtex_raw_tex* tex = &pkg->textures[tex_idx];
//
//	struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, tex_idx);
//	int w, h;
//	dtexpacker_get_size(packer, &w, &h);
//	assert(w == h);
//	uint8_t* buf = dtex_pvr_init_blank(w);
//	for (int i = 0; i < b4r->pic_size; ++i) {
//		struct dp_rect* r = &b4r->pictures[i].dst_r;
//		assert(r->dst_pos && r->dst_packer_idx >= 0);
//		if (r->dst_packer_idx == tex_idx) {
//			_load_picture_to_pvr_tex(buf, r->dst_pos, (struct b4r_picture*)r->ud);
//		}
//	}
//
//	tex->width = w;
//	tex->height = h;
//	tex->id = dtex_gl_create_texture(DTEX_TF_PVR4, w, h, buf, 0);
//	assert(tex->id != 0);
//	tex->id_alpha = 0;
//	tex->format = PVRTC;
//
//	return tex;
//}
//
//static inline struct dtex_raw_tex* 
//_load_etc1_tex(struct dtex_b4r* b4r, struct dtex_package* pkg, int tex_idx) {
//	struct dtex_raw_tex* tex = &pkg->textures[tex_idx];
//
//	struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, tex_idx);
//	int w, h;
//	dtexpacker_get_size(packer, &w, &h);
//
//	size_t sz = w * h;
//	uint8_t* buf = (uint8_t*)malloc(sz);
//	memset(buf, 0, sz);
//
//	for (int i = 0; i < b4r->pic_size; ++i) {
//		struct dp_rect* r = &b4r->pictures[i].dst_r;
//		assert(r->dst_pos && r->dst_packer_idx >= 0);
//		if (r->dst_packer_idx == tex_idx) {
//			_load_picture_to_etc1_tex(buf, w, h, r->dst_pos, (struct b4r_picture*)r->ud);
//		}
//	}
//
//	tex->width = w;
//	tex->height = h;
//	tex->id = dtex_gl_create_texture(DTEX_TF_ETC1, w, h, buf, 0);
//	tex->id_alpha = dtex_gl_create_texture(DTEX_TF_ETC1, w, h, buf + ((width * height) >> 1), 1);
//	tex->format = PKMC;
//
//	return tex;
//}
//
//struct dtex_raw_tex* 
//dtex_b4r_load_tex(struct dtex_b4r* b4r, struct dtex_package* pkg, int tex_idx) {
//	struct dtex_raw_tex* ret = NULL;
//	if (b4r->tex_type == TEX_PVR) {
//		ret = _load_pvr_tex(b4r, pkg, tex_idx);
//	} else if (b4r->tex_type == TEX_ETC1) {
//		ret = _load_etc1_tex(b4r, pkg, tex_idx);
//	} else {
//		assert(0);
//	}
//	return ret;
//}
//
//void 
//dtex_b4r_relocate(struct dtex_b4r* b4r, struct dtex_package* pkg) {
//	struct ej_sprite_pack* ej_pkg = pkg->ej_pkg;
//	for (int id = 0; id < ej_pkg->n; ++id) {
//		int type = ej_pkg->type[id];
//		if (type != TYPE_PICTURE) {
//			continue;
//		}
//
//		struct ej_pack_picture* ej_pic = (struct ej_pack_picture*)ej_pkg->data[id];
//		for (int j = 0; j < ej_pic->n; ++j) {
//			struct ej_pack_quad* ej_q = &ej_pic->rect[j];
//			assert(ej_q->texture_coord[0] < 0);
//			int idx = -ej_q->texture_coord[0];
//			assert(idx > 0 && idx <= b4r->pic_size);
//			struct b4r_picture* rp = &b4r->pictures[idx-1];
//			struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(b4r->packers, rp->dst_r.dst_packer_idx);
//
//			int w, h;
//			dtexpacker_get_size(packer, &w, &h);
//
//			struct dtex_rect* dr = &rp->dst_r.dst_pos->r;
//			ej_q->texture_coord[0] = ej_q->texture_coord[2] = dr->xmin;
//			ej_q->texture_coord[4] = ej_q->texture_coord[6] = dr->xmax;
//			ej_q->texture_coord[1] = ej_q->texture_coord[7] = h - dr->ymin;
//			ej_q->texture_coord[3] = ej_q->texture_coord[5] = h - dr->ymax;
//			ej_q->texid = rp->dst_r.dst_packer_idx;
//		}
//	}
//}
//
//#ifdef EXPORT_RRR
//
//size_t 
//dtex_b4r_size(void* data, int sz) {
//	struct dtex_b4r* b4r = dtex_b4r_create(data, sz, 0);
//	size_t size = dtex_alloc_size(b4r->alloc);
//	dtex_b4r_release(b4r);
//	return size;
//}
//
//#endif // EXPORT_RRR
