#include "dtex_draw.h"
#include "dtex_target.h"
#include "dtex_buffer.h"
#include "dtex_typedef.h"
#include "dtex_shader.h"
#include "dtex_screen.h"
#include "dtex_gl.h"
#include "dtex_texture.h"

#include <ejoy2d.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static inline void
_draw(const float vb[16], struct dtex_texture* src) {
	if (src->type == TT_RAW && src->t.RAW.format == PKMC) {
//		shader_draw_separate(vb, src->id, src->id_alpha);
	} else {
		dtex_shader_texture(src->id);
		dtex_shader_draw(vb);
	}
}

static inline void
_before_draw(struct dtex_texture* tex) {
// 	if (format == KTX) {
// 		shader_program(PROGRAM_SPRITE_KTX);
// 	} else if (format == PKMC) {
// 		shader_program(PROGRAM_SPRITE_PKM);
// 	} else {
// 		shader_program(PROGRAM_SPRITE);
// 	} 

	if (tex->type == TT_RAW && tex->t.RAW.format == PKMC) {
		dtex_shader_program(PROGRAM_ETC1);
	} else {
		dtex_shader_program(PROGRAM_NORMAL);
	}
}

//void 
//dtex_draw_rrp(struct dtex_texture* src, struct rrp_picture* pic, 
//	struct draw_params* params, const int32_t part_screen[8]) {
//	assert(pic);
//	_before_draw(src->format);
//
//	int* m = params->mat;
//	struct screen_coord* scr = params->screen;
//
//    float inv_sw = 1.0f / src->width,
//          inv_sh = 1.0f / src->height;
//    float inv_dw = 1.0f / pic->w,
//          inv_dh = 1.0f / pic->h;
//	for (int i = 0; i < pic->part_sz; ++i) {
//		struct rrp_part* part = &pic->rect[i];
//
//		float vb[16];
//		// texture coords
//		float txmin = part->dst.x * inv_sw,
//		      txmax = (part->dst.x + part->dst.w) * inv_sw,
//		      tymin = (part->dst.y + part->dst.h) * inv_sh,
//		      tymax = (part->dst.y) * inv_sh;
//		vb[2] = txmin; vb[3] = tymax;
//		vb[6] = txmin; vb[7] = tymin;
//		vb[10]= txmax; vb[11]= tymin;
//		vb[14]= txmax; vb[15]= tymax;
//		if (part->is_rotated) {
//			float x, y;
//        	x     = vb[2],  y     = vb[3];
//			vb[2] = vb[14]; vb[3] = vb[15];
//			vb[14]= vb[10]; vb[15]= vb[11];
//			vb[10]= vb[6];  vb[11]= vb[7];
//			vb[6] = x;      vb[7] = y;  			
//		}
//		// vertices
//		float vxmin = part->src.x * inv_dw,
//		      vxmax = (part->src.x + part->src.w) * inv_dw,
//		      vymax = (part->src.y) * inv_dh,
//		      vymin = (part->src.y + part->src.h) * inv_dh;
//		float pscr[8];
//		pscr[0] = pscr[2] = part_screen[0] + (part_screen[6] - part_screen[0]) * vxmin;
//		pscr[4] = pscr[6] = part_screen[0] + (part_screen[6] - part_screen[0]) * vxmax;
//		pscr[1] = pscr[7] = part_screen[1] + (part_screen[3] - part_screen[1]) * vymin;
//		pscr[3] = pscr[5] = part_screen[1] + (part_screen[3] - part_screen[1]) * vymax;
//		if (m) {
//			for (int i = 0; i < 4; ++i) {
//				int xx = pscr[i*2],
//					yy = pscr[i*2+1];
//				int dx = (xx * m[0] + yy * m[2]) / 1024 + m[4],
//					dy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
//				if (params->mirror) {
//					dx = -dx;
//				}
//				int rx = params->x + dx - scr->xoffset,
//					ry = params->y + dy - scr->yoffset;
//				vb[i*4+0] = scr->wscale * rx - 1.0f;
//				vb[i*4+1] =-scr->hscale * ry + 1.0f;				
//			}
//		} else {
//			for (int i = 0; i < 4; ++i) {
//				int xx = pscr[i*2],
//					yy = pscr[i*2+1];
//				if (params->mirror) {
//					xx = -xx;
//				}
//				int rx = params->x + xx - scr->xoffset,
//					ry = params->y + yy - scr->yoffset;
//				vb[i*4+0] = scr->wscale * rx - 1.0f;
//				vb[i*4+1] =-scr->hscale * ry + 1.0f;					
//			}
//		}
//
//		_draw(vb, src);
//	}
//}
//
//void 
//dtex_draw_pts(struct dtex_texture* src_tex, struct dp_pos* src_pos, struct pts_picture* src_pts_pic,
//	struct draw_params* params, const int32_t part_screen[8]) {
//
//	assert(src_pts_pic);
//	_before_draw(TEXTURE8);
//
//	int* m = params->mat;
//	struct screen_coord* scr = params->screen;
//
//    float inv_sw = 1.0f / src_tex->width,
//          inv_sh = 1.0f / src_tex->height;
//	for (int i = 0; i < src_pts_pic->strip_size; ++i) {
//		struct pts_strip* strip = &src_pts_pic->strips[i];
//		assert(strip->pos_size >= 3 && strip->pos_size % 3 == 0);
//		for (int j = 0; j < strip->pos_size - 2; j += 3) {
//			int16_t xa[3], ya[3];
//			for (int k = 0; k < 3; ++k) {
//				xa[k] = strip->vertices[(j+k)*2];
//				ya[k] = strip->vertices[(j+k)*2+1];
//			}
//
//			float vb[16];
//			// texture coords
//			// todo rotate
//			for (int k = 0; k < 3; ++k) {
//				int16_t x = (src_pos->r.xmin + src_pos->r.xmax) / 2 + xa[k], 
//						y = (src_pos->r.ymin + src_pos->r.ymax) / 2 + ya[k];
//				vb[k*4+2] = inv_sw * x;
//				vb[k*4+3] = inv_sh * y;
//			}
//			// vertices
//			if (m) {
//				for (int k = 0; k < 3; ++k) {
//					int xx = xa[k] * 16,
//						yy = ya[k] * 16;
//					int dx = (xx * m[0] + yy * m[2]) / 1024 + m[4],
//						dy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
//					if (params->mirror) {
//						dx = -dx;
//					}
//					int rx = params->x + dx - scr->xoffset,
//						ry = params->y + dy - scr->yoffset;
//					vb[k*4+0] = scr->wscale * rx - 1.0f;
//					vb[k*4+1] =-scr->hscale * ry + 1.0f;
//				}
//			} else {
//                for (int k = 0; k < 3; ++k) {
//					int xx = xa[k] * 16,
//						yy = ya[k] * 16;
//					if (params->mirror) {
//						xx = -xx;
//					}
//					int rx = params->x + xx - scr->xoffset,
//						ry = params->y + yy - scr->yoffset;
//					vb[k*4+0] = scr->wscale * rx - 1.0f;
//					vb[k*4+1] =-scr->hscale * ry + 1.0f;
//                }
//			}
//			memcpy(&vb[12], &vb[8], sizeof(float) * 4);
//
//			shader_draw(vb, src_tex->tex);
//		}
//	}
//}

static inline void 
_before_target_draw(struct dtex_buffer* buf, struct dtex_texture* src, struct dtex_texture* dst, 
				    struct dtex_target** target, float* scr_w, float* scr_h) {

	*target = dtex_buf_fetch_target(buf);
	dtex_target_bind_texture(*target, dst->id);
	dtex_target_bind(*target);

	float s;
	dtex_get_screen(scr_w, scr_h, &s);
	dtex_gl_viewport(0, 0, dst->width, dst->height);

	_before_draw(src);
}

static inline void
_after_target_draw(struct dtex_buffer* buf, struct dtex_target* target, float scr_w, float scr_h) {
	dtex_shader_flush();

	dtex_gl_viewport(0, 0, scr_w, scr_h);

	dtex_target_unbind();  
	dtex_target_unbind_texture(target);

	dtex_buf_return_target(buf, target);
}

void dtex_draw_to_texture(struct dtex_buffer* buf, struct dtex_texture* src, struct dtex_texture* dst, const float vb[16]) {
	struct dtex_target* target = NULL;
	float scr_w, scr_h;
	_before_target_draw(buf, src, dst, &target, &scr_w, &scr_h);

	_draw(vb, src);

	_after_target_draw(buf, target, scr_w, scr_h);
}

//void 
//dtex_draw_rrp_to_tex(struct dtex_buffer* buf, struct dtex_texture* src, struct rrp_picture* pic, 
//	struct dtex_texture* dst, struct dp_pos* pos, bool rotate) {
//
//	assert(pic);
//
//	struct dtex_inv_size src_sz;
//	src_sz.inv_w = 1.0f / src->width;
//	src_sz.inv_h = 1.0f / src->height;
//
//	struct dtex_inv_size dst_sz;	
//	dst_sz.inv_w = 1.0f / dst->width;
//	dst_sz.inv_h = 1.0f / dst->height;
//
//	struct dtex_target* target = NULL;
//	struct ej_screen scr;
//	_before_target_draw(buf, src, dst, &target, &scr);
//
//	for (int i = 0; i < pic->part_sz; ++i) {
//		struct rrp_part* part = &pic->rect[i];
//
//		// todo
//		struct dtex_rect src_rect;
//		struct dtex_rect* c3_pos = part->dst_pos;
//		src_rect.xmin = c3_pos->xmin + part->dst.x;
//		src_rect.xmax = src_rect.xmin + part->dst.w;
//		src_rect.ymin = c3_pos->ymin + part->dst.y;		
//		src_rect.ymax = src_rect.ymin + part->dst.h;
//
//		struct dtex_rect dst_rect;
//		if (rotate) {
//// 			// anticlockwise
////   		dst_rect.xmin = pos->r.xmax - (part->src.y + part->src.h);
////   		dst_rect.ymin = pos->r.ymin + part->src.x;
//
//			// clockwise
//			dst_rect.xmin = pos->r.xmin + part->src.y;
//			dst_rect.ymin = pos->r.ymax - (part->src.x + part->src.w);
//
//			dst_rect.xmax = dst_rect.xmin + part->src.h;
//			dst_rect.ymax = dst_rect.ymin + part->src.w;
//		} else {
//			dst_rect.xmin = pos->r.xmin + part->src.x;
//			dst_rect.xmax = dst_rect.xmin + part->src.w;
//			dst_rect.ymin = pos->r.ymin + part->src.y;
//			dst_rect.ymax = dst_rect.ymin + part->src.h;
//		}
//
//		float trans_vb[16];
//		float dst_vb[16];
//		int rotate_times = 0;
//		if (rotate && !part->is_rotated) {
//			rotate_times = -1;
//		} else if (!rotate && part->is_rotated) {
//			rotate_times = 1;
//		}
//		dtex_relocate_quad(NULL, &src_sz, &src_rect, &dst_sz, &dst_rect, rotate_times, trans_vb, dst_vb);
//		_draw(trans_vb, src);
//	}
//
//	_after_target_draw(buf, target, &scr);
//}

void dtex_debug_draw(unsigned int texid) {
	dtex_debug_draw_with_pos(texid, -1, -1, 1, 1);
}

void dtex_debug_draw_with_pos(unsigned int texid, float xmin, float ymin, 
							  float xmax, float ymax) {
	assert(dtex_gl_istexture(texid));

	dtex_shader_program(PROGRAM_NORMAL);

	float vb[16];

	vb[2] = 0, vb[3] = 0;
	vb[6] = 0, vb[7] = 1;
	vb[10] = 1, vb[11] = 1;
	vb[14] = 1, vb[15] = 0;

	vb[0] = xmin, vb[1] = ymin;
	vb[4] = xmin, vb[5] = ymax;
	vb[8] = xmax, vb[9] = ymax;
	vb[12] = xmax, vb[13] = ymin;

	dtex_shader_texture(texid);
	dtex_shader_draw(vb);		
}

//void dtex_debug_draw(struct dtex_texture* tex) {
//	float vb[16];
//
//	vb[0] = 0, vb[1] = 0;
//	vb[2] = 0, vb[3] = 0;
//
//	vb[4] = 0, vb[5] = 1;
//	vb[6] = 0, vb[7] = 1;
//
//	vb[8] = 1, vb[9] = 1;
//	vb[10] = 1, vb[11] = 1;
//
//	vb[12] = 1, vb[13] = 0;
//	vb[14] = 1, vb[15] = 0;
//
//	dtex_shader_texture(tex->tex);
//	dtex_shader_draw(vb);
//}

void dtex_flush_shader() {
	dtex_shader_flush();
}