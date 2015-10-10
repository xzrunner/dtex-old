//#include "dtex_rrr.h"
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
//#include <assert.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//
//static const uint16_t TEX_PVR = 1;
//static const uint16_t TEX_ETC1 = 2;
//
//struct rrr_part {
//	int16_t x, y;
//	int16_t w, h;
//
//	uint64_t* data;
//};
//
//struct rrr_picture {
//	int16_t id;
//	//	int16_t x, y;
//	int16_t w, h;
//
//	int16_t part_sz;
//	struct rrr_part* part;
//
//	struct dp_rect dst_r;
//};
//
//struct dtex_rrr {
//	struct alloc* alloc;
//
//	uint16_t tex_type;
//
//	struct dtex_vector* packers;
//
//	uint16_t pic_size;
//	struct rrr_picture pictures[0];
//};
//
//static inline void
//_decode_part(struct dtex_rrr* rrr, struct rrr_part* part, uint8_t** buf) {
//	uint8_t* ptr = *buf;
//
//	memcpy(&part->x, ptr, sizeof(part->x));
//	ptr += sizeof(part->x);
//	memcpy(&part->y, ptr, sizeof(part->x));
//	ptr += sizeof(part->y);
//	memcpy(&part->w, ptr, sizeof(part->w));
//	ptr += sizeof(part->w);
//	memcpy(&part->h, ptr, sizeof(part->h));
//	ptr += sizeof(part->h);
//
//	if (part->x < 0) {
//		part->w += part->x;
//		part->x = 0;
//	}
//	if (part->y < 0) {
//		part->h += part->y;
//		part->y = 0;
//	}
//
//	assert(rrr->tex_type == TEX_PVR || rrr->tex_type == TEX_ETC1);
//	size_t sz = sizeof(uint64_t) * part->w * part->h;
//	if (rrr->tex_type == TEX_ETC1) {
//		sz *= 2;
//	}
//	part->data = dtex_alloc(rrr->alloc, sz);
//	memcpy(part->data, ptr, sz);
//	ptr += sz;
//
//	*buf = ptr;
//}
//
//static inline void
//_decode_picture(struct dtex_rrr* rrr, struct rrr_picture* pic, uint8_t** buf) {
//	uint8_t* ptr = *buf;
//
//	memcpy(&pic->id, ptr, sizeof(pic->id));
//	ptr += sizeof(pic->id);
//	memcpy(&pic->w, ptr, sizeof(pic->w));
//	ptr += sizeof(pic->w);
//	memcpy(&pic->h, ptr, sizeof(pic->h));
//	ptr += sizeof(pic->h);
//	memcpy(&pic->part_sz, ptr, sizeof(pic->part_sz));
//	ptr += sizeof(pic->part_sz);
//
//	pic->rect = dtex_alloc(rrr->alloc, pic->part_sz * sizeof(struct rrr_part));
//	for (int i = 0; i < pic->part_sz; ++i) {
//		_decode_part(rrr, &pic->rect[i], &ptr);
//	}
//
//	*buf = ptr;
//}
//
//static inline struct dtex_vector*
//_pack_pictures(struct dtex_rrr* rrr) {
//	size_t sz = rrr->pic_size;
//	for (int i = 0; i < sz; ++i) {
//		struct rrr_picture* pic = &rrr->pictures[i];
//		struct dp_rect* r = &pic->dst_r;
//		r->w = TO_4TIMES(pic->w);
//		r->h = TO_4TIMES(pic->h);
//		r->ud = &rrr->pictures[i];
//		r->dst_packer_idx = -1;
//		r->dst_pos = NULL;
//	}
//
//	struct dp_rect* rects_ptr[sz];
//	for (int i = 0; i < sz; ++i) {
//		rects_ptr[i] = &rrr->pictures[i].dst_r;
//	}
//
//	return dtex_packer_square_multi(rects_ptr, sz);
//}
//
//struct dtex_rrr* 
//dtex_rrr_create(void* data, int sz, int cap) {
//	uint8_t* ptr = data;
//
//	uint16_t pic_sz;
//	memcpy(&pic_sz, ptr, sizeof(pic_sz));
//	ptr += sizeof(pic_sz);
//
//	struct alloc* a = dtex_init_alloc(cap);
//	struct dtex_rrr* rrr = dtex_alloc(a, sizeof(*rrr) + pic_sz * sizeof(struct rrr_picture));
//	rrr->alloc = a;
//
//	memcpy(&rrr->tex_type, ptr, sizeof(rrr->tex_type));
//	ptr += sizeof(rrr->tex_type);
//
//	rrr->pic_size = pic_sz;
//	for (int i = 0; i < pic_sz; ++i) {
//		_decode_picture(rrr, &rrr->pictures[i], &ptr);
//	}
//
////	// for test
////	dtex_rrr_load_to_c3(rrr, NULL);
//
//	rrr->packers = _pack_pictures(rrr);
//
//	return rrr;
//}
//
//static inline void
//_release_packers(struct dtex_rrr* rrr) {
//	if (!rrr->packers) return;
//
//	int sz = dtex_vector_size(rrr->packers);
//	for (int i = 0; i < sz; ++i) {
//		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(rrr->packers, i);
//		dtexpacker_release(packer);
//	}
//	dtex_vector_release(rrr->packers);	
//}
//
//void 
//dtex_rrr_release(struct dtex_rrr* rrr) {
//	if (rrr) {
//		_release_packers(rrr);
//
//		assert(rrr->alloc);
//		free(rrr->alloc);
//	}
//}
//
//static inline void
//_load_picture_to_pvr_tex(uint8_t* texture, struct dp_pos* pos, struct rrr_picture* pic) {
//	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
//	int sx = pos->r.xmin >> 2,
//		sy = pos->r.ymin >> 2;
//
//
//	for (int i = 0; i < pic->part_sz; ++i) {
//		struct rrr_part* part = &pic->rect[i];
//
//		int idx_src = 0;
//		for (int y = part->y; y < part->y + part->h; ++y) {
//			for (int x = part->x; x < part->x + part->w; ++x) {
//				int idx_dst = dtex_pvr_get_morton_number(sx + x, sy + y);
//				//assert(idx_dst < edge * edge / 16);
//				int64_t* src = (int64_t*)part->data + idx_src;
//				int64_t* dst = (int64_t*)texture + idx_dst;
//				memcpy(dst, src, sizeof(int64_t));
//				++idx_src;
//			}
//		}
//	}
//}
//
//static inline void
//_load_picture_to_etc1_tex(uint8_t* texture, int w, int h, struct dp_pos* pos, struct rrr_picture* pic) {
//	assert(IS_4TIMES(pos->r.xmin) && IS_4TIMES(pos->r.ymin));
//	int sx = pos->r.xmin >> 2,
//		sy = pos->r.ymin >> 2;
//
//	int bw = w / 4,
//		bh = h / 4;
//	for (int i = 0; i < pic->part_sz; ++i) {
//		struct rrr_part* part = &pic->rect[i];
//
//		int idx_src = 0;
//		for (int y = part->y; y < part->y + part->h; ++y) {
//			for (int x = part->x; x < part->x + part->w; ++x) {
//				int idx_dst = (sy + y) * bw + sx + x;
//
//				int64_t* src_rgb = (int64_t*)part->data + idx_src++;
//				int64_t* dst_rgb = (int64_t*)texture + idx_dst;
//				memcpy(dst_rgb, src_rgb, sizeof(int64_t));
//
//				int64_t* src_alpha = (int64_t*)part->data + idx_src++;
//				int64_t* dst_alpha = (int64_t*)texture + bw * bh + idx_dst;
//				memcpy(dst_alpha, src_alpha, sizeof(int64_t));
//			}
//		}
//	}
//}
//
////void 
////dtex_rrr_load_to_c3(struct dtex_rrr* rrr, struct dtex_c3* c3) {
////	size_t sz = rrr->pic_size;
////	struct dp_rect rects[sz];
////	for (int i = 0; i < sz; ++i) {
////		struct rrr_picture* pic = &rrr->pictures[i];
////		struct dp_rect* r = &rects[i];
////		r->w = TO_4TIMES(pic->w);
////		r->h = TO_4TIMES(pic->h);
////		r->ud = &rrr->pictures[i];
////		r->dst_packer_idx = -1;
////		r->dst_pos = NULL;
////	}
////
////	struct dp_rect* rects_ptr[sz];
////	for (int i = 0; i < sz; ++i) {
////		rects_ptr[i] = &rects[i];
////	}
////
////	struct dtex_vector* packers = dtex_packer_square_multi(rects_ptr, sz);
////
////	int packers_sz = dtex_vector_size(packers);
////	uint8_t* bufs[packers_sz];
////	for (int i = 0; i < packers_sz; ++i) {
////		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(packers, i);
////		int w, h;
////		dtexpacker_get_size(packer, &w, &h);
////		assert(w == h);
////		bufs[i] = dtex_pvr_init_blank(w);
////	}
////
////	for (int i = 0; i < sz; ++i) {
////		struct dp_rect* r = rects_ptr[i];
////		assert(r->dst_pos && r->dst_packer_idx >= 0);
////		_load_picture_to_pvr_tex(bufs[r->dst_packer_idx], r->dst_pos, (struct rrr_picture*)r->ud);
////	}
////
////// 	// for test
////// 	for (int i = 0; i < packers_sz; ++i) {
////// 		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(packers, i);
////// 		int w, h;
////// 		dtexpacker_get_size(packer, &w, &h);
////// 
////// 		char str_buf[50];
////// 		sprintf(str_buf, "F:/debug/rpack/test/rrr_part%d.pvr", i);
////// 		//sprintf(str_buf, "E:/debug/rpack/rrr/rrr_part%d.pvr", i);
////// 		dtex_pvr_write_file(str_buf, bufs[i], w, h);
////// 
////// 		dtexpacker_release(packer);
////// 		free(bufs[i]);
////// 	}
////
////	// to c3
////	for (int i = 0; i < packers_sz; ++i) {
//// 		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(packers, i);
//// 		int w, h;
//// 		dtexpacker_get_size(packer, &w, &h);
////
////		
////
//// 		dtexpacker_release(packer);
//// 		free(bufs[i]);		
////	}
////}
//
//void 
//dtex_rrr_preload_to_pkg(struct dtex_rrr* rrr, struct dtex_package* pkg) {
//	int packers_sz = dtex_vector_size(rrr->packers);
//	for (int i = 0; i < packers_sz; ++i) {
//		struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(rrr->packers, i);
//		int w, h;
//		dtexpacker_get_size(packer, &w, &h);
//		assert(w == h);
//		
// 		struct dtex_raw_tex* dst = &pkg->textures[pkg->tex_size++];
// 		dst->id = dst->id_alpha = 0;
// 		dst->width = w;
// 		dst->height = h;
// 		dst->format = TEXTURE8;
//	}
//}
//
//static inline struct dtex_raw_tex* 
//_load_pvr_tex(struct dtex_rrr* rrr, struct dtex_package* pkg, int tex_idx) {
//	struct dtex_raw_tex* tex = &pkg->textures[tex_idx];
//
//	struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(rrr->packers, tex_idx);
//	int w, h;
//	dtexpacker_get_size(packer, &w, &h);
//	assert(w == h);
//	uint8_t* buf = dtex_pvr_init_blank(w);
//	for (int i = 0; i < rrr->pic_size; ++i) {
//		struct dp_rect* r = &rrr->pictures[i].dst_r;
//		assert(r->dst_pos && r->dst_packer_idx >= 0);
//		if (r->dst_packer_idx == tex_idx) {
//			_load_picture_to_pvr_tex(buf, r->dst_pos, (struct rrr_picture*)r->ud);
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
//_load_etc1_tex(struct dtex_rrr* rrr, struct dtex_package* pkg, int tex_idx) {
//	struct dtex_raw_tex* tex = &pkg->textures[tex_idx];
//
//	struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(rrr->packers, tex_idx);
//	int w, h;
//	dtexpacker_get_size(packer, &w, &h);
//
//	size_t sz = w * h;
//	uint8_t* buf = (uint8_t*)malloc(sz);
//	memset(buf, 0, sz);
//
//	for (int i = 0; i < rrr->pic_size; ++i) {
//		struct dp_rect* r = &rrr->pictures[i].dst_r;
//		assert(r->dst_pos && r->dst_packer_idx >= 0);
//		if (r->dst_packer_idx == tex_idx) {
//			_load_picture_to_etc1_tex(buf, w, h, r->dst_pos, (struct rrr_picture*)r->ud);
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
//dtex_rrr_load_tex(struct dtex_rrr* rrr, struct dtex_package* pkg, int tex_idx) {
//	struct dtex_raw_tex* ret = NULL;
//	if (rrr->tex_type == TEX_PVR) {
//		ret = _load_pvr_tex(rrr, pkg, tex_idx);
//	} else if (rrr->tex_type == TEX_ETC1) {
//		ret = _load_etc1_tex(rrr, pkg, tex_idx);
//	} else {
//		assert(0);
//	}
//	return ret;
//}
//
//void 
//dtex_rrr_relocate(struct dtex_rrr* rrr, struct dtex_package* pkg) {
//	struct ejoypic* ep = pkg->ej_pkg->ep;
//	for (int id = 0; id < ep->max_id; ++id) {
//		struct animation * ani = ep->spr[id];
//		if (ani == NULL || ani->part_n > 0) {
//			continue;
//		}
//
//		struct ej_pack_picture* pic = (struct ej_pack_picture*)ani;
//		for (int j = 0; j < -pic->n; ++j) {
//			struct pack_quad* part = &pic->rect[j];
//			assert(part->src[0] < 0);
//			int idx = -part->src[0];
//			assert(idx > 0 && idx <= rrr->pic_size);
//			struct rrr_picture* rp = &rrr->pictures[idx-1];
//			struct dtex_packer* packer = (struct dtex_packer*)dtex_vector_get(rrr->packers, rp->dst_r.dst_packer_idx);
//
//			int w, h;
//			dtexpacker_get_size(packer, &w, &h);
//
//			struct dtex_rect* dr = &rp->dst_r.dst_pos->r;
//			part->src[0] = part->src[2] = dr->xmin;
//			part->src[4] = part->src[6] = dr->xmax;
//			part->src[1] = part->src[7] = h - dr->ymin;
//			part->src[3] = part->src[5] = h - dr->ymax;
//			part->texid = rp->dst_r.dst_packer_idx;
//		}
//	}
//}
//
//#ifdef EXPORT_RRR
//
//size_t 
//dtex_rrr_size(void* data, int sz) {
//	struct dtex_rrr* rrr = dtex_rrr_create(data, sz, 0);
//	size_t size = dtex_alloc_size(rrr->alloc);
//	dtex_rrr_release(rrr);
//	return size;
//}
//
//#endif // EXPORT_RRR
