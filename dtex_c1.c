#include "dtex_c1.h"
#include "dtex_typedef.h"
#include "dtex_texture.h"
#include "dtex_packer.h"
#include "dtex_buffer.h"
#include "dtex_fbo.h"
#include "dtex_draw.h"

#include "eploader.h"
#include "sprite.h"
#include "package.h"
#include "shader.h"
#include "ejoy2d.h"

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define NODE_SIZE 64
#define HASH_SIZE 101
#define PRELOAD_SIZE 64

struct dtex_node {
	// ori info
	struct animation* ani;
	int action;
	int frame;
	struct ej_rect rect;

	// dest info
	float vb[8];
	struct dp_position* pos;
};

struct hash_node {
	struct hash_node* next_hash;
	struct dtex_node n;	
};

struct preload_node {
	struct animation* ani;
	int action;
	int frame;
};

struct dtex_c1 {
	struct dtex_texture* texture;

	struct dtex_fbo *fbo0, *fbo1;
	bool is_fbo0_front;

	struct hash_node* freelist;
	struct hash_node* hash[HASH_SIZE];

	struct preload_node* preload_list[PRELOAD_SIZE];
	int preload_size;
};

struct dtex_c1* 
dtexc1_create(struct dtex_buffer* buf) {
	size_t nsize = NODE_SIZE * sizeof(struct hash_node);
	size_t psize = PRELOAD_SIZE * sizeof(struct preload_node);
	size_t sz = sizeof(struct dtex_c1) + nsize + psize;
	struct dtex_c1* dtex = (struct dtex_c1*)malloc(sz);
	memset(dtex, 0, sz);

	dtexbuf_reserve(buf, 1);
	dtex->texture = dtex_new_tex_with_packer(buf, PRELOAD_SIZE);

	dtex->fbo0 = dtexbuf_fetch_fbo(buf);
	dtex->fbo1 = dtexbuf_fetch_fbo(buf);
	dtex->is_fbo0_front = true;

	dtex->freelist = (struct hash_node*)(dtex + 1);
	for (int i = 0; i < NODE_SIZE - 1; ++i) {
		struct hash_node* hn = &dtex->freelist[i];
		hn->next_hash = &dtex->freelist[i+1];
	}
	dtex->freelist[NODE_SIZE-1].next_hash = NULL;

	struct preload_node* first_node = (struct preload_node*)((intptr_t)dtex->freelist + nsize);
	for (int i = 0; i < PRELOAD_SIZE; ++i) {
		dtex->preload_list[i] = first_node+i;
	}

	return dtex;
}

void 
dtexc1_release(struct dtex_c1* dtex, struct dtex_buffer* buf) {
	dtex_del_tex(buf, dtex->texture);

	dtexbuf_return_fbo(buf, dtex->fbo0);
	dtexbuf_return_fbo(buf, dtex->fbo1);

	free(dtex);
}

static inline void
_get_picture_region(struct ej_package* pkg, struct picture* pic, int* mat, void* ud) {
	assert(pic->n < 0);
	struct ej_rect* rect = (struct ej_rect*)ud;	
	for (int i = 0; i < -pic->n; ++i) {
		int32_t* vb = pic->part[i].screen;
		if (mat) {
			for (int i = 0; i < 4; ++i) {
				int xx = vb[i*2],
					yy = vb[i*2+1];
				int dx = (xx * mat[0] + yy * mat[2]) / 1024 + mat[4],
					dy = (xx * mat[1] + yy * mat[3]) / 1024 + mat[5];
				dx /= 16;
				dy /= 16;
				if (dx < rect->xmin) { rect->xmin = dx; }
				if (dx > rect->xmax) { rect->xmax = dx; }
				if (dy < rect->ymin) { rect->ymin = dy; }
				if (dy > rect->ymax) { rect->ymax = dy; }
			}
		} else {
			for (int i = 0; i < 4; ++i) {
				int xx = vb[i*2] / 16,
					yy = vb[i*2+1] / 16;
				if (xx < rect->xmin) { rect->xmin = xx; }
				if (xx > rect->xmax) { rect->xmax = xx; }
				if (yy < rect->ymin) { rect->ymin = yy; }
				if (yy > rect->ymax) { rect->ymax = yy; }
			}
		}
	}
}

struct draw_pic_params {
	int x, y;
	float scale;
	bool is_rotated;
    float tex_wscale, tex_hscale;
};

static inline void
_draw_picture(struct ej_package* pkg, struct picture* pic, int* mat, void* ud) {
	assert(pic->n < 0);
	struct draw_pic_params* params = (struct draw_pic_params*)ud;	
	for (int i = 0; i < -pic->n; ++i) {
		struct picture_part* part = &pic->part[i];
		assert(pkg->texture_n > part->texid);
		struct texture* t = &pkg->tex[part->texid];
		float vb[16];
		if (mat) {
			for (int i = 0; i < 4; ++i) {
				int xx = part->screen[i*2],
					yy = part->screen[i*2+1];
				int dx = (xx * mat[0] + yy * mat[2]) / 1024 + mat[4],
					dy = (xx * mat[1] + yy * mat[3]) / 1024 + mat[5];
				int rx = params->x + dx,
					ry = params->y + dy;
				vb[i*4+0]= rx * params->tex_wscale - 1.0f;
				//vb[i*4+1]= -ry * params->tex_hscale + 1.0f;
                vb[i*4+1]= ry * params->tex_hscale - 1.0f;
				vb[i*4+2] = t->width * part->src[i*2];
				vb[i*4+3] = t->height * part->src[i*2+1];
			}
		} else {
			for (int i = 0; i < 4; ++i) {
				int xx = part->screen[i*2],
					yy = part->screen[i*2+1];
				int rx = params->x + xx,
					ry = params->y + yy;
				vb[i*4+0]= rx * params->tex_wscale - 1.0f;
				//vb[i*4+1]= -ry * params->tex_hscale + 1.0f;
                vb[i*4+1]= ry * params->tex_hscale - 1.0f;
				vb[i*4+2] = t->width * part->src[i*2];
				vb[i*4+3] = t->height * part->src[i*2+1];
			}
		}
		if (params->is_rotated) {
			float x = vb[14], y = vb[15];
			vb[14] = vb[10]; vb[15] = vb[11];
			vb[10] = vb[6];  vb[11] = vb[7];
			vb[6]  = vb[2];  vb[7]  = vb[3];
			vb[2]  = x;      vb[3]  = y;
		}

		// todo dtex

		shader_draw(vb, t->id);
	}
}

static inline void
_traverse_animation(struct ej_package* pkg, struct animation* ani, int action, int frame, int* mat, void (*pic_func)(), void* ud) {
	if (ani == NULL || ani->part_n == 0) {
		return;
	}

	// draw picture
	if (ani->part_n < 0) {
		pic_func(pkg, (struct picture*)ani, mat, ud);
		return;
	}
	// draw children anim
	struct animation_action* act = &ani->action[action];
	if (act->n == 0) {
		return;
	}
	struct animation_frame* af = &act->frame[((unsigned int)frame) % act->n];
	struct ejoypic* ep = pkg->ep;
	for (int i = 0; i < af->n; ++i) {
		struct animation_component* c = &af->c[i];
		struct animation_part* part = &ani->part[c->index];
		// todo text
		if (part->text) {
			continue;
		}

		// todo mount

		assert(part->id >= 0 && part->id < ep->max_id);
		struct animation* subani = ep->spr[part->id];
		if (subani) {
			int tmp[6];
            int* sub_m = NULL;
            if (c->mat == NULL) {
                sub_m = mat;
            } else if (mat == NULL) {
                sub_m = c->mat;
            } else {
                matrix_mul(tmp, c->mat, mat);
                sub_m = tmp;
            }
			_traverse_animation(pkg, subani, 0, frame, sub_m, pic_func, ud);
		}
	}
}

static inline struct ej_rect
_get_animation_region(struct ej_package* pkg, struct animation* ani, int action) {
	struct ej_rect rect;
	rect.xmin = rect.ymin = INT_MAX;
	rect.xmax = rect.ymax = INT_MIN;
	if (ani->part_n < 0) {
		_traverse_animation(pkg, ani, action, 0, NULL, &_get_picture_region, &rect);		
	} else {
		struct animation_action* act = &ani->action[action];
		for (int i = 0; i < act->n; ++i) {
			_traverse_animation(pkg, ani, action, i, NULL, &_get_picture_region, &rect);
		}
	}
	return rect;
}

static inline void
_draw_animation(struct dtex_c1* dtex, struct ej_package* pkg, struct animation* ani, int action, int frame, 
				struct dp_position* pos, struct ej_rect* rect, bool need_clear, struct dtex_fbo* fbo) {
	struct dtex_texture* tex = dtex->texture;

	dtex_fbo_bind_texture(fbo, tex);
	dtex_fbo_bind(fbo);

	struct ej_screen scr;
	ejoy_get_screen(&scr);
	glViewport(0, 0, tex->width, tex->height);

	if (need_clear) {
		glScissor(pos->r.xmin, pos->r.ymin, pos->r.xmax - pos->r.xmin, pos->r.ymax - pos->r.ymin);
		glEnable(GL_SCISSOR_TEST);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
	}

	shader_program(PROGRAM_SPRITE);
	shader_color(0xffffffff, 0);

	struct draw_pic_params params;
	params.x = 0.5 * ((pos->r.xmin + pos->r.xmax) - (rect->xmin + rect->xmax)) * 16;
	params.y = 0.5 * ((pos->r.ymin + pos->r.ymax) - (rect->ymin + rect->ymax)) * 16;
	params.scale = 1;
	params.is_rotated = pos->is_rotated;
	params.tex_wscale = 1.0f / tex->width / 8;
	params.tex_hscale = 1.0f / tex->height / 8;
    
	_traverse_animation(pkg, ani, action, frame, NULL, &_draw_picture, &params);

	// todo cache
	shader_texture(0);  // force commit
	glViewport(0, 0, scr.w, scr.h); 
	dtex_fbo_unbind();
}

static inline unsigned int
_hash_anim(struct animation* ani, int action) {
	return ((uintptr_t)ani ^ (action * 97)) % HASH_SIZE;
}

static inline struct hash_node*
_query_anim(struct dtex_c1* dtex, struct animation* ani, int action) {
	int idx = _hash_anim(ani, action);
	struct hash_node* hn = dtex->hash[idx];
	while (hn) {
		struct dtex_node* n = &hn->n;
		if (n->ani == ani && n->action == action) {
			return hn;
		}
		hn = hn->next_hash;
	}
	return NULL;
}

static inline struct hash_node* 
_new_hash_node(struct dtex_c1* dtex) {
	if (dtex->freelist == NULL) {
		return NULL;
	}
	struct hash_node* ret = dtex->freelist;
	dtex->freelist = ret->next_hash;
	return ret;
}

void 
dtexc1_load_anim(struct dtex_c1* dtex, struct ej_package* pkg, struct animation* ani, int action) {
	struct hash_node* hn = _query_anim(dtex, ani, action);
	if (hn != NULL) {
		return;
	}

	// insert to packer
	struct ej_rect rect = _get_animation_region(pkg, ani, action);
	int w = rect.xmax - rect.xmin,
		h = rect.ymax - rect.ymin;
	struct dtex_texture* tex = dtex->texture;
	struct dp_position* pos = NULL;
	// todo padding and rotate
//	if (w >= h) {
		pos = dtexpacker_add(tex->packer, w, h);
//	} else {
//		pos = dtexpacker_add(tex->packer, h, w);
//	}
	if (pos == NULL) {
		return;
	}

	// save info
	pos->ud = &hn->n;	
	hn = _new_hash_node(dtex);
	if (hn == NULL) {
		return;
	}
	hn->n.ani = ani;
	hn->n.action = action;
	hn->n.frame = 0;
	hn->n.rect = rect;
	hn->n.pos = pos;
	float xmin = (float)pos->r.xmin / tex->width,
	      xmax = (float)pos->r.xmax / tex->width,
	      ymin = (float)pos->r.ymin / tex->height,
	      ymax = (float)pos->r.ymax / tex->height;
	if (pos->is_rotated) {
		hn->n.vb[0] = xmax; hn->n.vb[1] = ymin;
		hn->n.vb[2] = xmin; hn->n.vb[3] = ymin;
		hn->n.vb[4] = xmin; hn->n.vb[5] = ymax;
		hn->n.vb[6] = xmax; hn->n.vb[7] = ymin;
	} else {
		hn->n.vb[0] = xmin; hn->n.vb[1] = ymin;
		hn->n.vb[2] = xmin; hn->n.vb[3] = ymax;
		hn->n.vb[4] = xmax; hn->n.vb[5] = ymax;
		hn->n.vb[6] = xmax; hn->n.vb[7] = ymin;
	}

	unsigned int idx = _hash_anim(ani, action);
	hn->next_hash = dtex->hash[idx];
	dtex->hash[idx] = hn;

	// save image
	_draw_animation(dtex, pkg, ani, action, 0, pos, &rect, false, dtex->fbo0);
	_draw_animation(dtex, pkg, ani, action, 1, pos, &rect, false, dtex->fbo1);
	hn->n.frame = 0;
}

bool 
dtexc1_draw_anim(struct dtex_c1* dtex, struct ej_package* pkg, struct animation* ani, int action, int frame, struct draw_params* params) {
	struct hash_node* hn = _query_anim(dtex, ani, action);
	// need dtexc1_load_anim() first
	if (hn == NULL) {
		return false;
	}
    
    // todo
    
	// refresh this frame
	if (hn->n.frame != frame) {
        shader_flush();
		if (dtex->is_fbo0_front) {
			_draw_animation(dtex, pkg, ani, action, frame+1, hn->n.pos, &hn->n.rect, true, dtex->fbo0);
		} else {
			_draw_animation(dtex, pkg, ani, action, frame+1, hn->n.pos, &hn->n.rect, true, dtex->fbo1);
		}
		dtex->is_fbo0_front	= !dtex->is_fbo0_front;
		hn->n.frame = frame;
	}
    
	float vb[16];
	struct ej_rect* rect = &hn->n.rect;
	vb[0] = rect->xmin * 16; vb[1] = rect->ymin * 16;
	vb[4] = rect->xmin * 16; vb[5] = rect->ymax * 16;
	vb[8] = rect->xmax * 16; vb[9] = rect->ymax * 16;
	vb[12]= rect->xmax * 16; vb[13]= rect->ymin * 16;
	struct screen_coord* scr = params->screen;
	if (params->mat) {
		for (int i = 0; i < 4; ++i) {
			int* m = params->mat;
			int xx = vb[i*4],
				yy = vb[i*4+1];
			int dx = (xx * m[0] + yy * m[2]) / 1024 + m[4],
				dy = (xx * m[1] + yy * m[3]) / 1024 + m[5];
			int rx = params->x + dx - scr->xoffset,
				ry = params->y + dy - scr->yoffset;
			vb[i*4]   = scr->wscale * rx - 1.0f;
			vb[i*4+1] = -scr->hscale * ry + 1.0f;
		}
	} else {
		for (int i = 0; i < 4; ++i) {
			int xx = vb[i*4],
				yy = vb[i*4+1];
			int rx = params->x + xx - scr->xoffset,
				ry = params->y + yy - scr->yoffset;
			vb[i*4]   = scr->wscale * rx - 1.0f;
			vb[i*4+1] = -scr->hscale * ry + 1.0f;				
		}
	}

	for (int i = 0; i < 4; ++i) {
		vb[i*4+2] = hn->n.vb[i*2];
		vb[i*4+3] = hn->n.vb[i*2+1];
	}

	if (dtex->is_fbo0_front) {
		dtex_fbo_bind_texture(dtex->fbo0, dtex->texture);
	} else {
		dtex_fbo_bind_texture(dtex->fbo1, dtex->texture);
	}

	shader_program(PROGRAM_SPRITE);
	shader_color(params->color, params->additive);
	shader_draw(vb, dtex->texture->tex);
    
    return true;
}

void 
dtexc1_debug_draw(struct dtex_c1* dtex) {
	dtex_debug_draw(dtex->texture->tex);
}