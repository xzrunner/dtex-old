#include "dtex_c2_strategy.h"
#include "dtex_package.h"
#include "dtex_facade.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_PKG_DRAW_COUNT 1024

struct dtex_c2_strategy {
 	int tot_draw_count;
	int spr_draw_count[1];
};

struct dtex_c2_strategy* 
dtex_c2_strategy_create(int n) {
	size_t sz = sizeof(struct dtex_c2_strategy) + sizeof(int) * n;
	struct dtex_c2_strategy* stg = (struct dtex_c2_strategy*)malloc(sz);
	memset(stg, 0, sz);
	return stg;
}

void 
dtex_c2_strategy_release(struct dtex_c2_strategy* stg) {
	free(stg);
}

static inline void
_load_c2(struct dtex_package* pkg) {
	struct dtex_c2_strategy* stg = pkg->c2_stg;
	assert(stg);

	int spr_count = 0;
 	for (int i = 0; i < pkg->ej_pkg->n; ++i) {
 		if (stg->spr_draw_count[i] > 0) {
			++spr_count;
 		}
 	}	

	int* spr_ids = malloc(sizeof(int) * spr_count);
	int idx = 0;
	for (int i = 0; i < pkg->ej_pkg->n; ++i) {
		if (stg->spr_draw_count[i] > 0) {
			spr_ids[idx++] = i;
		}
	}

	dtexf_async_load_texture_with_c2_from_c3(pkg, spr_ids, spr_count);

	stg->tot_draw_count = 0;
	for (int i = 0; i < pkg->ej_pkg->n; ++i) {
		if (stg->spr_draw_count[i] > 0) {
			stg->spr_draw_count[i] = -1;
		}
	}
}

void 
dtex_c2_on_draw_sprite(struct ej_sprite* spr) {
	struct dtex_package* pkg = spr->pkg;
	if (!pkg) {
		return;
	}
	struct dtex_c2_strategy* stg = pkg->c2_stg;
	if (!stg || stg->spr_draw_count[spr->id] < 0) {
		return;
	}

	++stg->tot_draw_count;
	++stg->spr_draw_count[spr->id];

	if (stg->tot_draw_count > MAX_PKG_DRAW_COUNT) {
		_load_c2(pkg);
	}
}
