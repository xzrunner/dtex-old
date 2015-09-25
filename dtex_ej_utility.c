#include "dtex_ej_utility.h"

void 
dtex_ej_pkg_traverse(struct ej_sprite_pack* ej_pkg, void (*pic_func)(struct ej_pack_picture* ej_pic, void* ud), void* ud) {
	for (int id = 0; id < ej_pkg->n; ++id) {
		int type = ej_pkg->type[id];
		if (type == TYPE_PICTURE) {
			struct ej_pack_picture* ej_pic = (struct ej_pack_picture*)ej_pkg->data[id];
			pic_func(ej_pic, ud);
		}
	}
}

void 
dtex_ej_spr_traverse(struct ej_sprite_pack* ej_pkg, int spr_id, void (*pic_func)(struct ej_pack_picture* ej_pic, void* ud), void* ud) {
	if (spr_id < 0 || spr_id >= ej_pkg->n || spr_id == ANCHOR_ID) {
		return;
	}

	int type = ej_pkg->type[spr_id];
	if (type == TYPE_PICTURE) {
		struct ej_pack_picture* pic = (struct ej_pack_picture*)ej_pkg->data[spr_id];
		pic_func(pic, ud);
	} else if (type == TYPE_ANIMATION) {
		struct ej_pack_animation* anim = (struct ej_pack_animation*)ej_pkg->data[spr_id];		
		for (int i = 0; i < anim->component_number; ++i) {
			int cid = anim->component[i].id;
			dtex_ej_spr_traverse(ej_pkg, cid, pic_func, ud);
		}
	} else if (type == TYPE_PARTICLE3D) {
		struct ej_pack_particle3d* p3d = (struct ej_pack_particle3d*)ej_pkg->data[spr_id];
		for (int i = 0; i < p3d->cfg.symbol_count;  ++i) {
			uint32_t cid = (uint32_t)p3d->cfg.symbols[i].ud;
			dtex_ej_spr_traverse(ej_pkg, cid, pic_func, ud);
		}
	} else if (type == TYPE_PARTICLE2D) {
		struct ej_pack_particle2d* p2d = (struct ej_pack_particle2d*)ej_pkg->data[spr_id];
		for (int i = 0; i < p2d->cfg.symbol_count; ++i) {
			uint32_t cid = (uint32_t)p2d->cfg.symbols[i].ud;
			dtex_ej_spr_traverse(ej_pkg, cid, pic_func, ud);
		}
	}
}