#include "dtex_statistics.h"

struct dtex_statistics {
	int draw_call;
	int max_draw_call;

	bool in_draw;
};

static struct dtex_statistics STAT = { 0, 0, false };

void 
dtex_stat_draw_start() {
	STAT.in_draw = true;
}

void 
dtex_stat_draw_end() {
	if (STAT.draw_call > STAT.max_draw_call) {
		STAT.max_draw_call = STAT.draw_call;
	}
	STAT.draw_call = 0;

	STAT.in_draw = false;
}

void 
dtex_add_drawcall() {
	if (STAT.in_draw) {
		++STAT.draw_call;
	}
}

int 
dtex_get_drawcall() {
	return STAT.max_draw_call;
}