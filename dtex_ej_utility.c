#include "dtex_ej_utility.h"

void 
dtex_ej_pkg_traverse(struct ej_sprite_pack* ej_pkg, void (*pic_func)(int pic_id, struct ej_pack_picture* ej_pic, void* ud), void* ud) {
	for (int spr_id = 0; spr_id < ej_pkg->n; ++spr_id) {
		int type = ej_pkg->type[spr_id];
		if (type == TYPE_PICTURE) {
			struct ej_pack_picture* ej_pic = (struct ej_pack_picture*)ej_pkg->data[spr_id];
			pic_func(spr_id, ej_pic, ud);
		}
	}
}

void 
dtex_ej_spr_traverse(struct ej_sprite_pack* ej_pkg, int spr_id, void (*pic_func)(int pic_id, struct ej_pack_picture* ej_pic, void* ud), void* ud) {
	if (spr_id < 0 || spr_id >= ej_pkg->n || spr_id == ANCHOR_ID) {
		return;
	}

	int type = ej_pkg->type[spr_id];
	if (type == TYPE_PICTURE) {
		struct ej_pack_picture* pic = (struct ej_pack_picture*)ej_pkg->data[spr_id];
		pic_func(spr_id, pic, ud);
	} else if (type == TYPE_ANIMATION) {
		struct ej_pack_animation* anim = (struct ej_pack_animation*)ej_pkg->data[spr_id];		
		for (int i = 0; i < anim->component_number; ++i) {
			int cid = anim->component[i].id;
			dtex_ej_spr_traverse(ej_pkg, cid, pic_func, ud);
		}
	} else if (type == TYPE_PARTICLE3D) {
		struct p3d_emitter_cfg* p3d_cfg = (struct p3d_emitter_cfg*)ej_pkg->data[spr_id];
		for (int i = 0; i < p3d_cfg->symbol_count;  ++i) {
			uint32_t cid = (uint32_t)p3d_cfg->symbols[i].ud;
			dtex_ej_spr_traverse(ej_pkg, cid, pic_func, ud);
		}
	} else if (type == TYPE_PARTICLE2D) {
		struct p2d_emitter_cfg* p2d_cfg = (struct p2d_emitter_cfg*)ej_pkg->data[spr_id];
		for (int i = 0; i < p2d_cfg->symbol_count; ++i) {
			uint32_t cid = (uint32_t)p2d_cfg->symbols[i].ud;
			dtex_ej_spr_traverse(ej_pkg, cid, pic_func, ud);
		}
	}
}