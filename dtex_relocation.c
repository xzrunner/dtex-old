#include "dtex_relocation.h"
#include "dtex_array.h"
#include "dtex_package.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>

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