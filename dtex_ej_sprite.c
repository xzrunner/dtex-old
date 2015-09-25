#include "dtex_ej_sprite.h"
#include "dtex_typedef.h"
#include "dtex_package.h"
#include "dtex_texture_pool.h"
#include "dtex_screen.h"
#include "dtex_c2.h"
#include "dtex_shader.h"

#include <stdlib.h>
#include <assert.h>

struct ej_sprite* 
dtex_ej_sprite_create(struct ej_sprite_pack* ej_pkg, int spr_id) {
	int sz = ej_sprite_size(ej_pkg, spr_id);
	if (sz == 0) {
		return NULL;
	}

	struct ej_sprite* spr = (struct ej_sprite*)malloc(sz);
	ej_sprite_init(spr, ej_pkg, spr_id, sz);

	for (int i = 0; ; ++i) {
		int childid = ej_sprite_component(spr, i);
		if (childid < 0) {
			break;
		}

		struct ej_sprite* c = dtex_ej_sprite_create(ej_pkg, childid);
		if (c) {
			c->name = ej_sprite_childname(spr, i);
			sprite_mount(spr, i, c);
// 			if (spr->type == TYPE_ANIMATION) {
// 				update_anim_message(c, pack, id, i, spr->frame);
// 			} else if (spr->type == TYPE_PARTICLE3D) {
// 				update_p3d_message(c, pack, id, i);
// 			} else if (spr->type == TYPE_PARTICLE2D) {
// 				update_p2d_message(c, pack, id, i);
// 			}
		}
	}

	return spr;
}

static inline void
_draw_quad(struct dtex_package* pkg, struct dtex_c2* c2, struct ej_pack_picture* picture, 
		   const struct ej_srt* srt, const struct ej_sprite_trans* arg) {
   struct matrix tmp;
   if (arg->mat == NULL) {
	   ej_matrix_identity(&tmp);
   } else {
	   tmp = *arg->mat;
   }
   ej_matrix_srt(&tmp, srt);
   int *m = tmp.m;
   float vb[16];
   for (int i = 0; i < picture->n; i++) {
	   struct ej_pack_quad* q = &picture->rect[i];

	   struct dtex_raw_tex* tex = NULL;
	   if (q->texid < QUAD_TEXID_IN_PKG_MAX) {
		   assert(q->texid < pkg->tex_size);
		   tex = pkg->textures[q->texid];
	   } else {
		   tex = dtex_pool_query(q->texid - QUAD_TEXID_IN_PKG_MAX);
	   }

	   for (int j = 0; j < 4; j++) {
		   int xx = q->screen_coord[j*2+0];
		   int yy = q->screen_coord[j*2+1];
		   float vx = (xx * m[0] + yy * m[2]) / 1024 + m[4];
		   float vy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
		   dtex_screen_trans(&vx,&vy);

		   float tx = q->texture_coord[j*2+0];
		   float ty = q->texture_coord[j*2+1];
		   tx /= tex->width;
		   ty /= tex->height;

		   vb[j*4+0] = vx;
		   vb[j*4+1] = vy;
		   vb[j*4+2] = tx;
		   vb[j*4+3] = ty;
	   }

	   int texid = tex->id;

	   if (c2) {
		   int new_texid = 0;
		   float* tex_vb = dtex_c2_lookup_texcoords(c2, tex, vb, &new_texid);
		   if (tex_vb != NULL) {
			   memcpy(vb+2, tex_vb, 2*sizeof(float));
			   memcpy(vb+6, tex_vb+2, 2*sizeof(float));
			   memcpy(vb+10, tex_vb+4, 2*sizeof(float));
			   memcpy(vb+14, tex_vb+6, 2*sizeof(float));  
			   texid = new_texid;
		   }
	   }

	   dtex_shader_texture(texid);
	   dtex_shader_draw(vb);
   }
}

static inline int
_get_sprite_frame(struct ej_sprite* spr) {
	if (spr->type != TYPE_ANIMATION) {
		return spr->start_frame;
	}
	if (spr->total_frame <= 0) {
		return -1;
	}
	int f = spr->frame % spr->total_frame;
	if (f < 0) {
		f += spr->total_frame;
	}
	return f + spr->start_frame;
}

static inline void 
_draw(struct dtex_package* pkg, struct dtex_c2* c2, struct ej_sprite* spr, struct ej_srt* srt, struct ej_sprite_trans* ts);

static inline void
_draw_anim(struct dtex_package* pkg, struct dtex_c2* c2, struct ej_sprite* spr, struct ej_srt* srt, struct ej_sprite_trans* t) {
	int frame = _get_sprite_frame(spr);
	if (frame < 0) {
		return;
	}

	struct ej_pack_animation *ani = spr->s.ani;
	struct ej_pack_frame* pf = &ani->frame[frame];
	for (int i = 0; i < pf->n; i++) {
		struct pack_part *pp = &pf->part[i];
		int index = pp->component_id;
		struct ej_sprite* child = spr->data.children[index];
		if (child == NULL || (child->flags & SPRFLAG_INVISIBLE)) {
			continue;
		}
		struct sprite_trans temp2;
		struct matrix temp_matrix2;
		struct sprite_trans* ct = sprite_trans_mul(&pp->t, t, &temp2, &temp_matrix2);
		_draw(pkg, c2, child, srt, ct);
	}
}

static inline void 
_draw(struct dtex_package* pkg, struct dtex_c2* c2, struct ej_sprite* spr, struct ej_srt* srt, struct ej_sprite_trans* ts) {
	struct ej_sprite_trans temp;
	struct ej_matrix temp_matrix;
	struct ej_sprite_trans* t = ej_sprite_trans_mul(&spr->t, ts, &temp, &temp_matrix);

	switch (spr->type) {
	case TYPE_PICTURE:
		dtex_shader_program(PROGRAM_NORMAL);
		_draw_quad(pkg, c2, spr->s.pic, srt, t);
		break;
	case TYPE_ANIMATION:
		_draw_anim(pkg, c2, spr, srt, t);
		break;
	}
}

void 
dtex_ej_sprite_draw(struct dtex_package* pkg, struct dtex_c2* c2, struct ej_sprite* spr) {
	_draw(pkg, c2, spr, NULL, NULL);
}