#include "dtex_sprite.h"
#include "dtex_texture.h"
#include "dtex_packer.h"

#include "package.h"
#include "sprite.h"

#include <stdlib.h>

struct ej_sprite* 
dtex_sprite_create(struct dtex_texture* dst_tex, struct dp_pos* pos) {
	size_t pkg_sz = sizeof(struct ej_package) + sizeof(struct texture);
	size_t pic_sz = sizeof(struct picture) + sizeof(struct picture_part);
	struct ej_sprite* spr = (struct ej_sprite*)malloc(sizeof(*spr) + pkg_sz + pic_sz);
	struct ej_package* pkg = (struct ej_package*)(spr + 1);
	struct picture* pic = (struct picture*)((uint8_t*)pkg + pkg_sz);

	// fill pkg
	pkg->name = NULL;
	pkg->ep = NULL;
	pkg->texture_n = 1;
	pkg->tex->id = dst_tex->tex;
	pkg->tex->id_alpha = 0;
	pkg->tex->width = 1.0f / dst_tex->width;
	pkg->tex->height = 1.0f / dst_tex->height;

	// fill pic
	pic->n = -1;
	struct picture_part* pp = pic->part;
	pp->texid = 0;

	pp->src[0] = pos->r.xmin;
	pp->src[1] = pos->r.ymax;
	pp->src[2] = pos->r.xmax;
	pp->src[3] = pos->r.ymax;
	pp->src[4] = pos->r.xmax;
	pp->src[5] = pos->r.ymin;
	pp->src[6] = pos->r.xmin;
	pp->src[7] = pos->r.ymin;

	int32_t hw = (pos->r.xmax - pos->r.xmin) * 8,
		    hh = (pos->r.ymax - pos->r.ymin) * 8;
	pp->screen[0] = -hw;
	pp->screen[1] = -hh;
	pp->screen[2] = hw;
	pp->screen[3] = -hh;
	pp->screen[4] = hw;
	pp->screen[5] = hh;
	pp->screen[6] = -hw;
	pp->screen[7] = hh;

	// fill sprite
	spr->pack = pkg;
	spr->ani = (struct animation*)pic;
	spr->frame = 0;
	spr->extra = 0;
	spr->action = 0;
	spr->flag = EJ_MATRIX;
	spr->color_trans = 0xffffffff;
	spr->color_additive = 0;
	spr->c[0] = NULL;
	static const float SCALE = 1.0f;
	spr->mat[0] = 1024 * SCALE;
	spr->mat[1] = 0;
	spr->mat[2] = 0;
	spr->mat[3] = 1024 * SCALE;
	spr->mat[4] = 0;
	spr->mat[5] = 0;

	return spr;	
}
