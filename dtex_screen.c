#include "dtex_screen.h"

struct dtex_screen {
	int w;
	int h;
	float scale;
	float invw;
	float invh;
};

static struct dtex_screen SCREEN;

void 
dtex_set_screen(float w, float h, float scale) {
	SCREEN.w = w;
	SCREEN.h = h;
	SCREEN.scale = scale;
	SCREEN.invw = 2.0f / SCALE / w;
	SCREEN.invh = -2.0f / SCALE / h;
}

void 
dtex_get_screen(float* w, float* h, float* scale) {
	*w = SCREEN.w;
	*h = SCREEN.h;
	*scale = SCREEN.scale;
}

void 
dtex_screen_trans(float* x, float* y) {
	*x *= SCREEN.invw;
	*y *= SCREEN.invh;
}