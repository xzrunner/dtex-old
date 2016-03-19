#include "dtex_utility.h"
#include "dtex_ej_utility.h"
#include "dtex_package.h"

#include <ds_array.h>

#include <stdlib.h>
#include <string.h>

static inline void
_get_picture_id(int pic_id, struct ej_pack_picture* ej_pic, void* ud) {
	struct ds_array* array = (struct ds_array*)ud;
	ds_array_add(array, &pic_id);
}

static inline int 
_compare_int(const void *arg1, const void *arg2) {
	int id1 = *(int*)(arg1);
	int id2 = *(int*)(arg2);
	if (id1 < id2) {
		return -1;
	} else if (id1 > id2) {
		return 1;
	} else {
		return 0;
	}
}

static inline void
_unique_int_array(struct ds_array* array) {
	int size = ds_array_size(array);
	if (size <= 1) {
		return;
	}

	int sorted_list[size];
	for (int i = 0; i < size; ++i) {
		sorted_list[i] = *(int*)ds_array_fetch(array, i);
	}
	qsort((void*)sorted_list, size, sizeof(int), _compare_int);

	ds_array_clear(array);
	int last = sorted_list[0];
	ds_array_add(array, &last);
	for (int i = 1; i < size; ++i) {
		if (sorted_list[i] != last) {
			last = sorted_list[i];
			ds_array_add(array, &last);
		}
	}
}

void
dtex_get_picture_id_unique_set(struct ej_sprite_pack* ej_pkg, 
							   int* spr_ids, 
							   int spr_count, 
                               struct ds_array* uni_set) {
	ds_array_clear(uni_set);
	for (int i = 0; i < spr_count; ++i) {
		dtex_ej_spr_traverse(ej_pkg, spr_ids[i], _get_picture_id, uni_set);
	}
	_unique_int_array(uni_set);
}

static inline void
_get_texture_id(int pic_id, struct ej_pack_picture* ej_pic, void* ud) {
	struct ds_array* array = (struct ds_array*)ud;
	for (int i = 0; i < ej_pic->n; ++i) {
		ds_array_add(array, &ej_pic->rect[i].texid);
	}
}

void
dtex_get_texture_id_unique_set(struct ej_sprite_pack* ej_pkg, 
							   int* spr_ids, 
							   int spr_count, 
                               struct ds_array* uni_set) {
    ds_array_clear(uni_set);
	for (int i = 0; i < spr_count; ++i) {
		dtex_ej_spr_traverse(ej_pkg, spr_ids[i], _get_texture_id, uni_set);
	}
	_unique_int_array(uni_set);
}