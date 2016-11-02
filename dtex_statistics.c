#include "dtex_statistics.h"
#include "dtex_typedef.h"

#include <memory.h>
#include <assert.h>

#include <stdio.h>

#define MAX_TEXTURES 512

struct texture {
	int id;
	int type;
	int w, h;
};

struct statistics {
	struct texture textures[MAX_TEXTURES];
	int texture_count;
};

static struct statistics STAT;

void 
dtex_stat_init() {
	memset(&STAT, 0, sizeof(STAT));
}

void 
dtex_stat_add_tex(int texid, int type, int width, int height) {
	assert(STAT.texture_count < MAX_TEXTURES);
	if (STAT.texture_count >= MAX_TEXTURES) {
		return;
	}

	struct texture* tex = &STAT.textures[STAT.texture_count++];
	tex->type = type;
	tex->id   = texid;
	tex->w    = width;
	tex->h    = height;
}

void 
dtex_stat_del_tex(int texid) {
	for (int i = 0; i < STAT.texture_count; ++i) {
		struct texture* tex = &STAT.textures[i];
		if (tex->id == texid) {
			STAT.textures[i] = STAT.textures[--STAT.texture_count];			
			return;
		}		
	}
	assert(0);
}

void 
dtex_stat_dump_tex() {
	for (int i = 0; i < STAT.texture_count; ++i) {
		struct texture* tex = &STAT.textures[i];
		printf("tex: %d %d %d\n", tex->type, tex->w, tex->h);
	}
}

static int
calc_texture_size(int format, int width, int height) {
	switch( format ) {
	case DTEX_TF_RGBA8 :
		return width * height * 4;
	case DTEX_TF_RGBA4 :
	case DTEX_TF_PVR2 :
		return width * height / 4;
	case DTEX_TF_PVR4 :
	case DTEX_TF_ETC1 :
		return width * height / 2;
	case DTEX_TF_ETC2:
		return width * height;
	default:
		return 0;
	}
}

int  
dtex_stat_tex_mem() {
//	printf("//////////////////////////////////////////////////////////////////////////\n");
	int mem = 0;
	for (int i = 0; i < STAT.texture_count; ++i) {
		struct texture* tex = &STAT.textures[i];
		mem += calc_texture_size(tex->type, tex->w, tex->h);
//		printf("type %d, w %d, h %d, mem %0.1f\n", tex->type, tex->w, tex->h, mem / 1024.0f / 1024.0f);
	}
	return mem;
}