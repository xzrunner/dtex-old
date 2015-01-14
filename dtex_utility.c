#include "dtex_utility.h"
#include "dtex_typedef.h"
#include "dtex_c2.h"

#include "package.h"

#include <assert.h>
#include <stdlib.h>

static inline void
_traverse_animation(struct ej_package* pkg, int id, struct dtex_pos* src, struct dtex_pos* dst, void (*pic_func)(), void* ud) {
	struct ejoypic* ep = pkg->ep;
	if (id < 0 || ep->max_id < id) {
		return;
	}
	struct animation* ani = ep->spr[id];
	if (ani == NULL) {
		return;
	}

	if (ani->part_n <= 0) {
		pic_func(pkg, (struct picture*)ani, src, dst, ud);
	} else {
		for (int i = 0; i < ani->part_n; ++i) {
			struct animation_part* part = &ani->part[i];
			if (!part->text) {
				_traverse_animation(pkg, part->id, src, dst, pic_func, ud);
			}
		}
	}
}

static inline void
_relocate_spr(struct ej_package* pkg, struct picture* pic, struct dtex_pos* src, struct dtex_pos* dst, void* ud) {
	assert(pic->n < 0);
	for (int i = 0; i < -pic->n; ++i) {
		struct picture_part* part = &pic->part[i];
		assert(pkg->texture_n > part->texid);
		struct texture* tex = &pkg->tex[part->texid];
		if (tex->id == src->id) {
			assert(tex->id_alpha == src->id_alpha
				&& tex->width == src->inv_width
				&& tex->height == src->inv_height);
			int16_t sw = src->rect.xmax - src->rect.xmin,
					sh = src->rect.ymax - src->rect.ymin;
			int16_t dw = dst->rect.xmax - dst->rect.xmin,
					dh = dst->rect.ymax - dst->rect.ymin;
			for (int i = 0; i < 4; ++i) {
				part->src[i*2]   = (part->src[i*2]   - src->rect.xmin) * dw / sw + dst->rect.xmin;
				part->src[i*2+1] = (part->src[i*2+1] - src->rect.ymin) * dh / sh + dst->rect.ymin;				
			}
		}
	}
}

static inline void
_relocate_tex(struct ej_package* pkg, struct picture* pic, struct dtex_pos* src, struct dtex_pos* dst, void* ud) {
	assert(pic->n < 0);
	for (int i = 0; i < -pic->n; ++i) {
		struct picture_part* part = &pic->part[i];
		assert(pkg->texture_n > part->texid);
		struct texture* tex = &pkg->tex[part->texid];
		if (tex->id == src->id) {
			assert(tex->id_alpha == src->id_alpha
				&& tex->width == src->inv_width
				&& tex->height == src->inv_height);
			tex->id = dst->id;
			tex->id_alpha = dst->id_alpha;
			tex->width = dst->inv_width;
			tex->height = dst->inv_height;
		}
	}
}

void 
dtex_relocate_spr(struct ej_package* pkg, int id, struct dtex_pos* src, struct dtex_pos* dst) {
	_traverse_animation(pkg, id, src, dst, &_relocate_spr, NULL);
	_traverse_animation(pkg, id, src, dst, &_relocate_tex, NULL);	
}

static inline void
_relocate_c2_key(struct ej_package* pkg, struct picture* pic, struct dtex_pos* src, struct dtex_pos* dst, void* ud) {
	assert(pic->n < 0);
	struct dtex_c2* c2 = (struct dtex_c2*)ud;
	for (int i = 0; i < -pic->n; ++i) {
		struct picture_part* part = &pic->part[i];
		assert(pkg->texture_n > part->texid);
		struct texture* tex = &pkg->tex[part->texid];
		if (tex->id == src->id) {
			assert(tex->id_alpha == src->id_alpha
				&& tex->width == src->inv_width
				&& tex->height == src->inv_height);
			int16_t sw = src->rect.xmax - src->rect.xmin,
					sh = src->rect.ymax - src->rect.ymin;
			int16_t dw = dst->rect.xmax - dst->rect.xmin,
					dh = dst->rect.ymax - dst->rect.ymin;
			int16_t dst_src[8];
			for (int i = 0; i < 4; ++i) {
				dst_src[i*2]   = (part->src[i*2]   - src->rect.xmin) * dw / sw + dst->rect.xmin;
				dst_src[i*2+1] = (part->src[i*2+1] - src->rect.ymin) * dh / sh + dst->rect.ymin;				
			}

			struct dtex_rect src_rect, dst_rect;
			dtex_get_pic_src_rect(part->src, &src_rect);
			dtex_get_pic_src_rect(dst_src, &dst_rect);
			dtexc2_change_key(c2, src->id, &src_rect, dst->id, &dst_rect);
		}
	}
}

void 
dtex_relocate_c2_key(struct dtex_c2* c2, struct ej_package* pkg, int id, struct dtex_pos* src, struct dtex_pos* dst) {
	_traverse_animation(pkg, id, src, dst, &_relocate_c2_key, c2);
}

void dtex_relocate_pic_part(int16_t part_src[8], struct dtex_inv_size* src_sz, struct dtex_rect* src_rect, 
	struct dtex_inv_size* dst_sz, struct dtex_rect* dst_rect, bool rotate, float trans_vb[16], float dst_vb[8]) {
	float src_xmin = src_rect->xmin * src_sz->inv_w,
	      src_xmax = src_rect->xmax * src_sz->inv_w,
	      src_ymin = src_rect->ymin * src_sz->inv_h,
	      src_ymax = src_rect->ymax * src_sz->inv_h;
	float dst_xmin = dst_rect->xmin * dst_sz->inv_w,
	      dst_xmax = dst_rect->xmax * dst_sz->inv_w,
	      dst_ymin = dst_rect->ymin * dst_sz->inv_h,
	      dst_ymax = dst_rect->ymax * dst_sz->inv_h;
	float vd_xmin = dst_xmin * 2 - 1,
          vd_xmax = dst_xmax * 2 - 1,
          vd_ymin = dst_ymin * 2 - 1,
          vd_ymax = dst_ymax * 2 - 1;

    if (part_src == NULL || part_src[0] < 0) {
		trans_vb[0] = vd_xmin; 	trans_vb[1] = vd_ymin;
		trans_vb[2] = src_xmin; trans_vb[3] = src_ymin;
		trans_vb[4] = vd_xmin; 	trans_vb[5] = vd_ymax;
		trans_vb[6] = src_xmin; trans_vb[7] = src_ymax;
		trans_vb[8] = vd_xmax; 	trans_vb[9] = vd_ymax;
		trans_vb[10]= src_xmax; trans_vb[11]= src_ymax;
		trans_vb[12]= vd_xmax; 	trans_vb[13]= vd_ymin;
		trans_vb[14]= src_xmax; trans_vb[15]= src_ymin;
		dst_vb[0] = dst_xmin; dst_vb[1] = dst_ymin;
		dst_vb[4] = dst_xmax; dst_vb[5] = dst_ymax;
		dst_vb[2] = dst_xmin; dst_vb[3] = dst_ymax;
		dst_vb[6] = dst_xmax; dst_vb[7] = dst_ymin;
    } else {
		float cx = 0, cy = 0;
		for (int i = 0; i < 4; ++i) {
			cx += part_src[i*2];
			cy += part_src[i*2+1];
		}
		cx *= 0.25f;
		cy *= 0.25f;

	    if (part_src[0] < cx) {
			trans_vb[2] = src_xmin; trans_vb[10]= src_xmax;
			trans_vb[0] = vd_xmin; trans_vb[8] = vd_xmax;
			dst_vb[0] = dst_xmin; dst_vb[4] = dst_xmax;    	
	    } else {
			trans_vb[2] = src_xmax; trans_vb[10]= src_xmin;
			trans_vb[0] = vd_xmax; trans_vb[8] = vd_xmin;
			dst_vb[0] = dst_xmax; dst_vb[4] = dst_xmin;
	    }
	    if (part_src[2] < cx) {
			trans_vb[6] = src_xmin; trans_vb[14]= src_xmax;
			trans_vb[4] = vd_xmin; trans_vb[12] = vd_xmax;
			dst_vb[2] = dst_xmin; dst_vb[6] = dst_xmax;
	    } else {
			trans_vb[6] = src_xmax; trans_vb[14]= src_xmin;
			trans_vb[4] = vd_xmax; trans_vb[12] = vd_xmin;
			dst_vb[2] = dst_xmax; dst_vb[6] = dst_xmin;
	    }
	    if (part_src[1] < cy) {
			trans_vb[3] = src_ymin; trans_vb[11]= src_ymax;
			trans_vb[1] = vd_ymin; trans_vb[9] = vd_ymax;
			dst_vb[1] = dst_ymin; dst_vb[5] = dst_ymax;
	    } else {
			trans_vb[3] = src_ymax; trans_vb[11]= src_ymin;
			trans_vb[1] = vd_ymax; trans_vb[9] = vd_ymin;
			dst_vb[1] = dst_ymax; dst_vb[5] = dst_ymin;
	    }
	    if (part_src[3] < cy) {
			trans_vb[7] = src_ymin; trans_vb[15]= src_ymax;
			trans_vb[5] = vd_ymin; trans_vb[13] = vd_ymax;
			dst_vb[3] = dst_ymin; dst_vb[7] = dst_ymax;
	    } else {
			trans_vb[7] = src_ymax; trans_vb[15]= src_ymin;
			trans_vb[5] = vd_ymax; trans_vb[13] = vd_ymin;
			dst_vb[3] = dst_ymax; dst_vb[7] = dst_ymin;
	    }
    }

    if (rotate) {
		float x, y;

		x = trans_vb[2]; y = trans_vb[3];
		trans_vb[2] = trans_vb[6];  trans_vb[3] = trans_vb[7];
		trans_vb[6] = trans_vb[10]; trans_vb[7] = trans_vb[11];
		trans_vb[10]= trans_vb[14]; trans_vb[11]= trans_vb[15];
		trans_vb[14]= x;            trans_vb[15]= y;

		x = dst_vb[6]; y = dst_vb[7];
		dst_vb[6] = dst_vb[4]; dst_vb[7] = dst_vb[5];
		dst_vb[4] = dst_vb[2]; dst_vb[5] = dst_vb[3];
		dst_vb[2] = dst_vb[0]; dst_vb[3] = dst_vb[1];
		dst_vb[0] = x;         dst_vb[1] = y;
    }

    // todo padding    
}

void 
dtex_get_pic_src_rect(int16_t* src, struct dtex_rect* rect) {
	rect->xmin = rect->ymin = INT16_MAX;
	rect->xmax = rect->ymax = 0;
	for (int i = 0; i < 4; ++i) {
		if (src[i*2] < rect->xmin) rect->xmin = src[i*2];
		if (src[i*2] > rect->xmax) rect->xmax = src[i*2];
		if (src[i*2+1] < rect->ymin) rect->ymin = src[i*2+1];
		if (src[i*2+1] > rect->ymax) rect->ymax = src[i*2+1];
	}
}