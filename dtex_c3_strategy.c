#include "dtex_c3_strategy.h"

#include <stdlib.h>

struct dtex_c3_strategy {
	struct dtex_c3_stg_cfg cfg;
};

struct dtex_c3_strategy* 
dtex_c3_strategy_create(struct dtex_c3_stg_cfg* cfg) {
	struct dtex_c3_strategy* stg = (struct dtex_c3_strategy*)malloc(sizeof(*stg));
	stg->cfg = *cfg;
	return stg;
}

void 
dtex_c3_strategy_release(struct dtex_c3_strategy* stg) {
	free(stg);
}

bool 
dtex_c3_is_static(struct dtex_c3_strategy* stg) {
	return !stg->cfg.clear_enable;
}
