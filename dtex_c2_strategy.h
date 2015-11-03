#ifdef __cplusplus
extern "C"
{
#endif

#ifndef dynamic_texture_c2_strategy_h
#define dynamic_texture_c2_strategy_h

#include "ejoy2d.h"

struct dtex_c2_strategy;

struct dtex_c2_stg_cfg {
	bool clear_enable;

	int single_max_count;
	int diff_spr_count;
	int tot_count;
};

void dtex_c2_strategy_init(int max_no_update_count);

struct dtex_c2_strategy* dtex_c2_strategy_create(int n, struct dtex_c2_stg_cfg* cfg);
void dtex_c2_strategy_release(struct dtex_c2_strategy*);

void dtex_c2_on_draw_query_fail(struct ej_sprite* spr);
void dtex_c2_strategy_update();

void dtex_c2_strategy_clear(struct dtex_package*);

bool dtex_c2_insert_can_clear(struct dtex_c2_strategy*);

#endif // dynamic_texture_c2_strategy_h

#ifdef __cplusplus
}
#endif