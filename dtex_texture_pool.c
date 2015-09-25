#include "dtex_texture_pool.h"
#include "dtex_loader.h"
#include "dtex_statistics.h"

#include "opengl.h"

#include <string.h>
#include <stdlib.h>

#define MAX_TEXTURE 512

struct texture_pool {
	int count;
	struct dtex_raw_tex tex[MAX_TEXTURE];
};

static struct texture_pool POOL;

void 
dtex_pool_init() {
	memset(&POOL, 0, sizeof(POOL));
	for (int i = 0; i < MAX_TEXTURE; ++i) {
		POOL.tex[i].id = -1;
	}
}

struct dtex_raw_tex*
dtex_pool_add() {
	int idx = -1;
	for (int i = 0; i < POOL.count; ++i) {
		struct dtex_raw_tex* tex = &POOL.tex[i];
		if (tex->id == -1) {
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
		POOL.tex[idx].idx = idx;
		return &POOL.tex[idx];
	} else {
		return NULL;
	}
}

static inline void
_release_texture(struct dtex_raw_tex* tex) {
	if (tex->id != 0) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &tex->id);
		dtex_stat_delete_texture(tex->id, tex->width, tex->height);
		tex->id = 0;
	}
   if (tex->id_alpha != 0) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &tex->id_alpha);
		dtex_stat_delete_texture(tex->id_alpha, tex->width, tex->height);
		tex->id_alpha = 0;
   }
   free(tex->filepath);
   memset(tex, 0, sizeof(*tex));
   tex->id = -1;
}

void 
dtex_pool_remove(struct dtex_raw_tex* tex) {
	for (int i = 0; i < POOL.count; ++i) {
		if (&POOL.tex[i] == tex) {
			_release_texture(tex);
			return;
		}
	}
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