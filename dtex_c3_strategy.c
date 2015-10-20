#include "dtex_c3_strategy.h"

#include <stdlib.h>

struct dtex_c3_strategy {
	bool is_static;
};

struct dtex_c3_strategy* 
dtex_c3_strategy_create(bool is_static) {
	struct dtex_c3_strategy* stg = (struct dtex_c3_strategy*)malloc(sizeof(*stg));

	stg->is_static = is_static;

	return stg;
}

void 
dtex_c3_strategy_release(struct dtex_c3_strategy* stg) {
	free(stg);
}

bool 
dtex_c3_is_static(struct dtex_c3_strategy* stg) {
	return stg->is_static;
}
