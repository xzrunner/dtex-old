#include "dtex_texture.h"
#include "dtex_log.h"
#include "dtex_packer.h"
#include "dtex_buffer.h"
#include "dtex_gl.h"
#include "dtex_target.h"

#include <string.h>

#define MAX_TEXTURE 512

struct texture_pool {
	int count;
	struct dtex_texture textures[MAX_TEXTURE];
};

static struct texture_pool POOL;

static inline struct dtex_texture* 
_add_texture() {
	struct dtex_texture* tex = NULL;
	for (int i = 0; i < POOL.count; ++i) {
		if (POOL.textures[i].type == DTEX_TT_INVALID) {
			tex = &POOL.textures[i];
			tex->uid = i + QUAD_TEXID_IN_PKG_MAX;
			break;
		}
	}

	if (!tex && POOL.count < MAX_TEXTURE) {
		tex = &POOL.textures[POOL.count++];
		tex->uid = POOL.count - 1 + QUAD_TEXID_IN_PKG_MAX;
	}

	return tex;
}

struct dtex_texture* 
dtex_texture_create_raw() {
	struct dtex_texture* tex = _add_texture();
	if (!tex) {
		dtex_fault("dtex_create_raw_texture fail.");
		return NULL;
	}

	tex->type = DTEX_TT_RAW;
	tex->t.RAW.scale = 1;

	return tex;
}

struct dtex_texture* 
dtex_texture_create_mid(struct dtex_buffer* buf) {
	struct dtex_texture* tex = _add_texture();
	if (!tex) {
		dtex_fault("dtex_texture_create_mid fail.");
		return NULL;
	}

	unsigned int tex_id = dtexbuf_fetch_texid(buf);
	if (tex_id == 0) {
		return NULL;
	}

	tex->type = DTEX_TT_MID;
	tex->id = tex_id;
	int edge = dtexbuf_get_tex_edge(buf);
	tex->width = tex->height = edge;
	tex->inv_width = tex->inv_height = 1.0f / edge;
	tex->t.MID.packer = NULL;

	return tex;
}

void 
dtex_texture_release(struct dtex_buffer* buf, struct dtex_texture* tex) {
	if (!tex) { return; }

	if (tex->type == DTEX_TT_RAW) {
		if (tex->id != 0) {
			dtex_gl_release_texture(tex->id, 0);
		}
		if (tex->t.RAW.id_alpha != 0) {
			dtex_gl_release_texture(tex->t.RAW.id_alpha, 1);
		}
	} else if (tex->type == DTEX_TT_MID) {
		if (tex->id != 0 && !dtexbuf_return_texid(buf, tex->id)) {
			dtex_gl_release_texture(tex->id, 0);
		}
		if (tex->t.MID.packer != NULL) {
			dtexpacker_release(tex->t.MID.packer); 
		}
	}

	memset(tex, 0, sizeof(*tex));
	tex->type = DTEX_TT_INVALID;
}

void 
dtex_texture_clear(struct dtex_buffer* buf, struct dtex_texture* tex) {
	if (!tex) {
		return;
	}

	struct dtex_target* target = dtex_buf_fetch_target(buf);
	dtex_target_bind_texture(target, tex->id);
	dtex_target_bind(target);

	dtex_gl_clear_color(0, 0, 0, 0);

	dtex_target_unbind();
	dtex_target_unbind_texture(target);
	dtex_buf_return_target(buf, target);
}

void 
dtex_texture_pool_init() {
	memset(&POOL, 0, sizeof(POOL));
	for (int i = 0; i < MAX_TEXTURE; ++i) {
		POOL.textures[i].type = DTEX_TT_INVALID;
		POOL.textures[i].uid = i + QUAD_TEXID_IN_PKG_MAX;
	}
}

struct dtex_texture* 
dtex_texture_fetch(int uid) {
	int idx = uid - QUAD_TEXID_IN_PKG_MAX;
	if (idx < 0 || idx >= POOL.count) {
		return NULL;
	} else {
		return &POOL.textures[idx];
	}
}

unsigned int 
dtex_texture_get_gl_id(int uid) {
	struct dtex_texture* tex = dtex_texture_fetch(uid);
	if (tex) {
		return tex->id;
	} else {
		return 0;
	}
}
