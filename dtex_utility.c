//#include "dtex_utility.h"
//#include "dtex_typedef.h"
//#include "dtex_c2.h"
//#include "dtex_texture_pool.h"
//
//#include <assert.h>
//#include <stdlib.h>
//#include <string.h>
//
//// static inline void
//// _traverse_anim_relocate(struct ej_sprite_pack* pkg, int id, struct dtex_img_pos* src, struct dtex_img_pos* dst, void (*pic_func)(), void* ud) {
//// 	struct ejoypic* ep = pkg->ep;
//// 	if (id < 0 || ep->max_id < id) {
//// 		return;
//// 	}
//// 	struct animation* ani = ep->spr[id];
//// 	if (ani == NULL) {
//// 		return;
//// 	}
//// 
//// 	if (ani->part_n <= 0) {
//// 		pic_func(pkg, (struct ej_pack_picture*)ani, src, dst, ud);
//// 	} else {
//// 		for (int i = 0; i < ani->part_n; ++i) {
//// 			struct animation_part* part = &ani->part[i];
//// 			if (!part->text) {
//// 				_traverse_anim_relocate(pkg, part->id, src, dst, pic_func, ud);
//// 			}
//// 		}
//// 	}
//// }
//
//static inline void
//_traverse_array_relocate(struct ej_sprite_pack* ej_pkg, struct int_array* array, struct dtex_img_pos* src, struct dtex_img_pos* dst, void (*pic_func)(), void* ud) {
//	for (int i = 0; i < array->size; ++i) {
//		int id = array->data[i];
//		if (id < 0 || id >= ej_pkg->n) {
//			continue;
//		}
//
//		int type = ej_pkg->type[id];
//		if (type != TYPE_PICTURE) {
//			continue;
//		}
//
//		struct ej_pack_picture* ej_pic = (struct ej_pack_picture*)ej_pkg->data[id];
//		pic_func(ej_pkg, ej_pic, src, dst, ud);
//	}
//}
//
//static inline void
//_traverse_complex_pic_id(struct ej_sprite_pack* ej_pkg, int id, void (*pic_func)(), void* ud) {
//	if (id < 0 || id >= ej_pkg->n) {
//		return;
//	}
//
//	int type = ej_pkg->type[id];
//	if (type == TYPE_PICTURE) {
//		pic_func(id, ud);
//	} else if (type == TYPE_ANIMATION) {
//		struct ej_pack_animation* anim = (struct ej_pack_animation*)ej_pkg->data[id];
//		for (int i = 0; i < anim->component_number; ++i) {
//			_traverse_complex_pic_id(ej_pkg, anim->component[i].id, pic_func, ud);
//		}
//	} else if (type == TYPE_PARTICLE3D) {
//		struct ej_pack_particle3d* p3d = (struct ej_pack_particle3d*)ej_pkg->data[id];
//		for (int i = 0; i < p3d->cfg.symbol_count;  ++i) {
//			uint32_t cid = (uint32_t)p3d->cfg.symbols[i].ud;
//			_traverse_complex_pic_id(ej_pkg, cid, pic_func, ud);
//		}
//	} else if (type == TYPE_PARTICLE2D) {
//		struct ej_pack_particle2d* p2d = (struct ej_pack_particle2d*)ej_pkg->data[id];
//		for (int i = 0; i < p2d->cfg.symbol_count;  ++i) {
//			uint32_t cid = (uint32_t)p2d->cfg.symbols[i].ud;
//			_traverse_complex_pic_id(ej_pkg, cid, pic_func, ud);
//		}
//	}
//}
//
//static inline void
//_get_pic_count(int id, void* ud) {
//	int* count = (int*)ud;
//	++*count;
//}
//
//static inline void
//_get_pic_id(int id, void* ud) {
//	struct int_array* array = (struct int_array*)ud;
//	array->data[array->size++] = id;
//}
//
//static inline int 
//_compare_id(const void *arg1, const void *arg2) {
//	int id1 = *(int*)(arg1);
//	int id2 = *(int*)(arg2);
//
//	if (id1 < id2) {
//		return -1;
//	} else if (id1 > id2) {
//		return 1;
//	} else {
//		return 0;
//	}
//}
//
//struct int_array*
//dtex_get_picture_id_set(struct ej_sprite_pack* ej_pkg, int id) {
//	int count = 0;
//	_traverse_complex_pic_id(ej_pkg, id, _get_pic_count, &count);
//
//	struct int_array* array = (struct int_array*)malloc(sizeof(struct int_array) + sizeof(int) * count);
//	array->size = 0;
//	array->data = (int*)(array + 1);
//	_traverse_complex_pic_id(ej_pkg, id, _get_pic_id, array);
//
//	if (array->size <= 1) {
//		return array;
//	}
//
//	qsort((void*)array->data, array->size, sizeof(int), _compare_id);
//
//	int tmp[array->size];
//	tmp[0] = array->data[0];
//	int tmp_sz = 1;
//
//	int last = array->data[0];
//	for (int i = 1; i < array->size; ++i) {
//		if (array->data[i] != last) {
//			tmp[tmp_sz++] = array->data[i];
//			last = array->data[i];
//		}
//	}
//
//	memcpy(array->data, tmp, sizeof(int) * tmp_sz);
//	array->size = tmp_sz;
//	
//	return array;
//}
//
//// static inline void
//// _relocate_spr(struct ej_sprite_pack* ej_pkg, struct ej_pack_picture* pic, struct dtex_img_pos* src, struct dtex_img_pos* dst, void* ud) {
//// 	for (int i = 0; i < pic->n; ++i) {
//// 		struct ej_pack_quad* ej_q = &pic->rect[i];
//// 		int* texid = (int*)ud;
//// 		if (ej_q->texid != *texid) {
//// 			continue;
//// 		}
//// 
//// 		struct dtex_raw_tex* tex = dtex_pool_query(ej_q->texid);
//// 		if (tex->id != src->id) {
//// 			continue;
//// 		}
//// 
//// 		assert(tex->id_alpha == src->id_alpha
//// 			&& tex->width == src->inv_width
//// 			&& tex->height == src->inv_height);
//// 		int16_t sw = src->rect.xmax - src->rect.xmin,
//// 				sh = src->rect.ymax - src->rect.ymin;
//// 		int16_t dw = dst->rect.xmax - dst->rect.xmin,
//// 				dh = dst->rect.ymax - dst->rect.ymin;
//// 
//// 		for (int i = 0; i < 4; ++i) {
//// 			ej_q->texture_coord[i*2]   = (ej_q->texture_coord[i*2]   - src->rect.xmin) * dw / sw + dst->rect.xmin;
//// 			ej_q->texture_coord[i*2+1] = (ej_q->texture_coord[i*2+1] - src->rect.ymin) * dh / sh + dst->rect.ymin;		
//// 		}
//// 	}
//// }
//
//// static inline void
//// _relocate_tex(struct ej_sprite_pack* ej_pkg, struct ej_pack_picture* pic, struct dtex_img_pos* src, struct dtex_img_pos* dst, void* ud) {
//// 	for (int i = 0; i < pic->n; ++i) {
//// 		struct ej_pack_quad* ej_q = &pic->rect[i];
//// 		int* texid = (int*)ud;
//// 		if (ej_q->texid != *texid) {
//// 			continue;
//// 		}
//// 
//// 		struct dtex_raw_tex* tex = dtex_pool_query(ej_q->texid);
//// 		if (tex->id != src->id) {
//// 			continue;
//// 		}
//// 
//// 		assert(tex->id_alpha == src->id_alpha
//// 			&& tex->width == src->inv_width
//// 			&& tex->height == src->inv_height);
//// 		tex->id = dst->id;
//// 		tex->id_alpha = dst->id_alpha;
//// 		tex->width = dst->inv_width;
//// 		tex->height = dst->inv_height;
//// 	}
//// }
//
//// void 
//// dtex_relocate_spr(struct ej_sprite_pack* ej_pkg, struct int_array* array, int tex_idx, struct dtex_img_pos* src, struct dtex_img_pos* dst) {
//// 	_traverse_array_relocate(ej_pkg, array, src, dst, &_relocate_spr, &tex_idx);
//// 	_traverse_array_relocate(ej_pkg, array, src, dst, &_relocate_tex, &tex_idx);
//// }
//
//static inline void
//_relocate_c2_key(struct ej_sprite_pack* ej_pkg, struct ej_pack_picture* pic, struct dtex_img_pos* src, struct dtex_img_pos* dst, void* ud) {
//	struct dtex_c2* c2 = (struct dtex_c2*)ud;
//	for (int i = 0; i < pic->n; ++i) {
//		struct ej_pack_quad* ej_q = &pic->rect[i];
//		struct dtex_raw_tex* tex = dtex_pool_query(ej_q->texid);
//		if (tex->id != src->id) {
//			continue;
//		}
//
//		assert(tex->id_alpha == src->id_alpha
//			&& tex->width == src->inv_width
//			&& tex->height == src->inv_height);
//		int16_t sw = src->rect.xmax - src->rect.xmin,
//				sh = src->rect.ymax - src->rect.ymin;
//		int16_t dw = dst->rect.xmax - dst->rect.xmin,
//				dh = dst->rect.ymax - dst->rect.ymin;
//		uint16_t dst_src[8];
//		for (int i = 0; i < 4; ++i) {
//			dst_src[i*2]   = (ej_q->texture_coord[i*2]   - src->rect.xmin) * dw / sw + dst->rect.xmin;
//			dst_src[i*2+1] = (ej_q->texture_coord[i*2+1] - src->rect.ymin) * dh / sh + dst->rect.ymin;				
//		}
//
//		struct dtex_rect src_rect, dst_rect;
//		dtex_get_texcoords_region(ej_q->texture_coord, &src_rect);
//		dtex_get_texcoords_region(dst_src, &dst_rect);
//		dtex_c2_change_key(c2, src->id, &src_rect, dst->id, &dst_rect);
//	}
//}
//
//void 
//dtex_relocate_c2_key(struct dtex_c2* c2, struct ej_sprite_pack* ej_pkg, struct int_array* array, struct dtex_img_pos* src, struct dtex_img_pos* dst) {
//	_traverse_array_relocate(ej_pkg, array, src, dst, &_relocate_c2_key, c2);
//}
//
