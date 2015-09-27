#include "dtex_statistics.h"

#include <memory.h>
#include <assert.h>

#define MAX_TEXTURES 512

struct dtex_stat {
	int draw_call;
	int max_draw_call;

	struct stat_texture textures[MAX_TEXTURES];
	int texture_count;

	bool in_draw;
};

static struct dtex_stat STAT;

void 
dtex_stat_init() {
	memset(&STAT, 0, sizeof(STAT));
}

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
dtex_stat_add_drawcall() {
	if (STAT.in_draw) {
		++STAT.draw_call;
	}
}

int 
dtex_stat_get_drawcall() {
	return STAT.max_draw_call;
}

void 
dtex_stat_add_texture(int texid, int width, int height) {
	assert(STAT.texture_count < MAX_TEXTURES);
	if (STAT.texture_count >= MAX_TEXTURES) {
		return;
	}

	struct stat_texture* tex = &STAT.textures[STAT.texture_count++];
	tex->id = texid;
	tex->w = width;
	tex->h = height;
}

void 
dtex_stat_delete_texture(int texid) {
	for (int i = 0; i < STAT.texture_count; ++i) {
		struct stat_texture* tex = &STAT.textures[i];
		if (tex->id == texid) {
			STAT.textures[i] = STAT.textures[--STAT.texture_count];			
			return;
		}		
	}
	assert(0);
}

void 
dtex_stat_get_texture(int* count, struct stat_texture** list) {
	*count = STAT.texture_count;
	*list = STAT.textures;
}