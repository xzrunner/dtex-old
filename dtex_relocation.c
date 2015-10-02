#include "dtex_relocation.h"
#include "dtex_array.h"
#include "dtex_package.h"
#include "dtex_typedef.h"
#include "dtex_c2.h"
#include "dtex_texture.h"

#include <assert.h>

void 
dtex_swap_quad_src_info(struct dtex_package* pkg, struct dtex_array* picture_ids) {	
	struct ej_sprite_pack* ej_pkg = pkg->ej_pkg;
	int size = dtex_array_size(picture_ids);
	for (int i = 0; i < size; ++i) {
		int spr_id = *(int*)dtex_array_fetch(picture_ids, i);
		int type = ej_pkg->type[spr_id];
		assert(type == TYPE_PICTURE);
		struct ej_pack_picture* ej_pic = (struct ej_pack_picture*)ej_pkg->data[spr_id];
		struct pic_ext_info* ext_pic = (struct pic_ext_info*)(pkg->spr_ext_info[spr_id]);
		for (int j = 0; j < ej_pic->n; ++j) {
			struct ej_pack_quad* ej = &ej_pic->rect[j];
			struct quad_ext_info* ext = &ext_pic->quads[j];

			int tmp_id = ext->texid;
			ext->texid = ej->texid;
			ej->texid = tmp_id;

			uint16_t tmp_texcoords[8];
			int sz = sizeof(tmp_texcoords);
			memcpy(tmp_texcoords, ext->texture_coord, sz);
			memcpy(ext->texture_coord, ej->texture_coord, sz);
			memcpy(ej->texture_coord, tmp_texcoords, sz);
		}
	}
}

void 
dtex_relocate_spr(struct dtex_package* pkg, int tex_idx, struct dtex_array* pictures, 
                  struct dtex_texture_with_rect* src, struct dtex_texture_with_rect* dst) {
	struct dtex_texture* tex = pkg->textures[tex_idx];
	assert(tex->type == TT_RAW);
	if (tex != src->tex) {
		return;
	}

	int sz;
	
	// relocate quad src info
	sz = dtex_array_size(pictures);
	for (int i = 0; i < sz; ++i) {
		int spr_id = *(int*)dtex_array_fetch(pictures, i);
		struct ej_pack_picture* ej_pic = (struct ej_pack_picture*)pkg->ej_pkg->data[spr_id];
		for (int j = 0; j < ej_pic->n; ++j) {
			struct ej_pack_quad* ej_q = &ej_pic->rect[j];
			if (ej_q->texid != tex_idx) {
				continue;
			}

			int16_t sw = src->rect.xmax - src->rect.xmin,
				    sh = src->rect.ymax - src->rect.ymin;
			int16_t dw = dst->rect.xmax - dst->rect.xmin,
				    dh = dst->rect.ymax - dst->rect.ymin;

			for (int k = 0; k < 4; ++k) {
				ej_q->texture_coord[k*2]   = (ej_q->texture_coord[k*2]   - src->rect.xmin) * dw / sw + dst->rect.xmin;
				ej_q->texture_coord[k*2+1] = (ej_q->texture_coord[k*2+1] - src->rect.ymin) * dh / sh + dst->rect.ymin;		
			}
		}
	}

	// relocate texture
	pkg->textures[tex_idx] = dst->tex;
}

void dtex_relocate_c2_key(struct dtex_c2* c2, struct dtex_package* pkg, int tex_idx, 
                          struct dtex_array* pictures, struct dtex_texture_with_rect* src, struct dtex_texture_with_rect* dst) {
	struct dtex_texture* tex = pkg->textures[tex_idx];
	assert(tex->type == TT_RAW);
	if (tex != src->tex) {
		return;
	}

	int sz = dtex_array_size(pictures);
	for (int i = 0; i < sz; ++i) {
		int spr_id = *(int*)dtex_array_fetch(pictures, i);
		struct ej_pack_picture* ej_pic = (struct ej_pack_picture*)pkg->ej_pkg->data[spr_id];
		for (int j = 0; j < ej_pic->n; ++j) {
			struct ej_pack_quad* ej_q = &ej_pic->rect[j];
			if (ej_q->texid != tex_idx) {
				continue;
			}

			int16_t sw = src->rect.xmax - src->rect.xmin,
					sh = src->rect.ymax - src->rect.ymin;
			int16_t dw = dst->rect.xmax - dst->rect.xmin,
					dh = dst->rect.ymax - dst->rect.ymin;
			uint16_t dst_src[8];
			for (int i = 0; i < 4; ++i) {
				dst_src[i*2]   = (ej_q->texture_coord[i*2]   - src->rect.xmin) * dw / sw + dst->rect.xmin;
				dst_src[i*2+1] = (ej_q->texture_coord[i*2+1] - src->rect.ymin) * dh / sh + dst->rect.ymin;				
			}

			struct dtex_texture_with_rect src_quad, dst_quad;
			src_quad.tex = src->tex;
			dst_quad.tex = dst->tex;
			dtex_get_texcoords_region(ej_q->texture_coord, &src_quad.rect);
			dtex_get_texcoords_region(dst_src, &dst_quad.rect);
			dtex_c2_change_key(c2, &src_quad, &dst_quad);
		}
	}
}

void dtex_relocate_quad(uint16_t part_src[8], struct dtex_inv_size* src_sz, struct dtex_rect* src_rect, 
	                    struct dtex_inv_size* dst_sz, struct dtex_rect* dst_rect, int rotate, float trans_vb[16], float dst_vb[8]) {
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

		dst_vb[0] = dst_xmin; dst_vb[1] = dst_ymax;
		dst_vb[4] = dst_xmax; dst_vb[5] = dst_ymin;
		dst_vb[2] = dst_xmin; dst_vb[3] = dst_ymin;
		dst_vb[6] = dst_xmax; dst_vb[7] = dst_ymax;
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

	if (rotate == 1) {
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
	} else if (rotate == -1) {
		float x, y;
		x = trans_vb[2]; y = trans_vb[3];
		trans_vb[2] = trans_vb[14];  trans_vb[3] = trans_vb[15];
		trans_vb[14] = trans_vb[10]; trans_vb[15] = trans_vb[11];
		trans_vb[10]= trans_vb[6]; trans_vb[11]= trans_vb[7];
		trans_vb[6]= x;            trans_vb[7]= y;	

		x = dst_vb[6]; y = dst_vb[7];
		dst_vb[6] = dst_vb[0]; dst_vb[7] = dst_vb[1];
		dst_vb[0] = dst_vb[2]; dst_vb[1] = dst_vb[3];
		dst_vb[2] = dst_vb[4]; dst_vb[3] = dst_vb[5];
		dst_vb[4] = x;         dst_vb[5] = y;
	}

    // todo padding    
}

void 
dtex_get_texcoords_region(uint16_t* texcoords, struct dtex_rect* region) {
	region->xmin = region->ymin = INT16_MAX;
	region->xmax = region->ymax = 0;
	for (int i = 0; i < 4; ++i) {
		if (texcoords[i*2]   < region->xmin) region->xmin = texcoords[i*2];
		if (texcoords[i*2]   > region->xmax) region->xmax = texcoords[i*2];
		if (texcoords[i*2+1] < region->ymin) region->ymin = texcoords[i*2+1];
		if (texcoords[i*2+1] > region->ymax) region->ymax = texcoords[i*2+1];
	}
}