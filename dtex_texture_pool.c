#include "dtex_texture_pool.h"
#include "dtex_loader.h"

#include <string.h>

#define MAX_TEXTURE 512

struct texture_pool {
	int count;
	struct dtex_raw_tex tex[MAX_TEXTURE];
};

static struct texture_pool POOL;

void 
dtedx_pool_init() {
	memset(&POOL, 0, sizeof(POOL));
}

int 
dtex_pool_add(struct dtex_raw_tex* tex) {
	int idx = -1;
	for (int i = 0; i < POOL.count; ++i) {
		struct dtex_raw_tex* tex = &POOL.tex[i];
		if (tex->id == 0) {
			idx = i;
			break;
		}
	}

	if (idx == -1) {
		if (POOL.count < MAX_TEXTURE) {
			idx = POOL.count++;
		}
	}

	if (idx != -1) {
		memcpy(&POOL.tex[idx], tex, sizeof(*tex));
	}
	return idx;
}

struct dtex_raw_tex* 
dtex_pool_query(int id) {
	if (id >= 0 && id < POOL.count) {
		return &POOL.tex[id];
	} else {
		return NULL;
	}
}

int 
dtex_pool_query_glid(int id) {
	if (id >= 0 && id < POOL.count) {
		return POOL.tex[id].id;
	} else {
		return 0;
	}
}