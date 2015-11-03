#include "dtex_c2_strategy.h"
#include "dtex_package.h"
#include "dtex_facade.h"

#include "dtex_log.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct dtex_c2_strategy {
	struct dtex_c2_stg_cfg cfg;

	int single_max_count;
	int diff_spr_count;
 	int tot_count;
	int spr_draw_count[1];
};

static int MAX_NO_UPDATE_COUNT;
static int NO_UPDATE_COUNT = 0;
static float DISCOUNT = 1;

void 
dtex_c2_strategy_init(int max_no_update_count) {
	MAX_NO_UPDATE_COUNT = max_no_update_count;
}

struct dtex_c2_strategy* 
dtex_c2_strategy_create(int n, struct dtex_c2_stg_cfg* cfg) {
	size_t sz = sizeof(struct dtex_c2_strategy) + sizeof(int) * n;
	struct dtex_c2_strategy* stg = (struct dtex_c2_strategy*)malloc(sz);
	memset(stg, 0, sz);

	stg->cfg = *cfg;

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

	struct ej_sprite_pack* ej_pkg = pkg->ej_pkg;
	int spr_ids[stg->diff_spr_count];
	int idx = 0;
	for (int i = 0; i < ej_pkg->n; ++i) {
		if (stg->spr_draw_count[i] > 0) {
			spr_ids[idx++] = i;
		}
	}

	if (pkg->c3_loading != 0) {
		return;
	}

	bool succ = false;
	if (pkg->c3_stg) {
		succ = dtexf_async_load_texture_with_c2_from_c3(pkg, spr_ids, stg->diff_spr_count);
	} else {
//		succ = dtexf_async_load_texture_with_c2(pkg, spr_ids, spr_count);

		// already exists
		// suppose 100% scale
		dtexf_c2_load_begin();
		for (int i = 0; i < stg->diff_spr_count; ++i) {
			int spr_id = spr_ids[i];
			dtexf_c2_load(pkg, spr_id);
		}
		dtexf_c2_load_end();
		succ = true;
	}
	if (!succ) {
		return;
	}

	dtex_c2_strategy_clear(pkg);

	NO_UPDATE_COUNT = 0;
	DISCOUNT = 1;
}

void 
dtex_c2_on_draw_query_fail(struct ej_sprite* spr) {
	struct dtex_package* pkg = spr->pkg;
	if (!pkg) {
		return;
	}
	struct dtex_c2_strategy* stg = pkg->c2_stg;
	if (!stg || stg->spr_draw_count[spr->id] < 0) {
		return;
	}

	++stg->tot_count;
	if (stg->spr_draw_count[spr->id] == 0) {
		++stg->diff_spr_count;
	}
	++stg->spr_draw_count[spr->id];
	if (stg->spr_draw_count[spr->id] > stg->single_max_count) {
		stg->single_max_count = stg->spr_draw_count[spr->id];
	}

	if (stg->single_max_count > stg->cfg.single_max_count * DISCOUNT) {
		dtex_info(" c2 single_max_count > %d", (int)(stg->cfg.single_max_count * DISCOUNT));
		_load_c2(pkg);
	} else if (stg->diff_spr_count > stg->cfg.diff_spr_count * DISCOUNT) {
		dtex_info(" c2 diff_spr_count > %d", (int)(stg->cfg.diff_spr_count * DISCOUNT));
		_load_c2(pkg);
	} else if (stg->tot_count > stg->cfg.tot_count * DISCOUNT) {
		dtex_info(" c2 tot_count > %d", (int)(stg->cfg.tot_count * DISCOUNT));
		_load_c2(pkg);
	} 
}

void 
dtex_c2_strategy_update() {
	++NO_UPDATE_COUNT;
	if (NO_UPDATE_COUNT <= MAX_NO_UPDATE_COUNT) {
		DISCOUNT = (float)(MAX_NO_UPDATE_COUNT - NO_UPDATE_COUNT) / MAX_NO_UPDATE_COUNT;
	}
}

void 
dtex_c2_strategy_clear(struct dtex_package* pkg) {
	struct dtex_c2_strategy* stg = pkg->c2_stg;
	if (!stg || !stg->cfg.clear_enable) {
		return;
	}
	assert(stg);
	struct ej_sprite_pack* ej_pkg = pkg->ej_pkg;
	stg->tot_count = 0;
	stg->single_max_count = 0;
	stg->diff_spr_count = 0;
	memset(stg->spr_draw_count, 0, sizeof(int) * ej_pkg->n);
}

bool 
dtex_c2_insert_can_clear(struct dtex_c2_strategy* stg) {
	return stg->cfg.clear_enable;
}