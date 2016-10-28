#include "dtex_facade.h"
#include "dtex_typedef.h"
#include "dtex_package.h"
#include "dtex_shader.h"
#include "dtex_gl.h"
#include "dtex_res_path.h"
#include "dtex_c3_strategy.h"
#include "dtex_c2_strategy.h"
#include "dtex_async_loader.h"
#include "dtex_texture_cache.h"
#include "dtex_texture.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

/************************************************************************/
/* overall                                                              */
/************************************************************************/

static int
lcreate(lua_State* L) {
	const char* str = lua_tostring(L, 1);	
	dtexf_create(str);
	return 0;
}

static int
lrelease(lua_State* L) {
	dtexf_release();
	return 0;
}

/************************************************************************/
/* normal                                                               */
/************************************************************************/

static int
lload_package(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	const char* epe_path = luaL_checkstring(L, 2);
	int lod = (int)(luaL_optinteger(L, 3, 0));

	struct dtex_package* pkg = dtexf_load_pkg(name, epe_path, 1, lod);
	lua_pushlightuserdata(L, pkg);

	return 1;
}

static int
lunload_package(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (pkg) {
		dtexf_unload_pkg(pkg);
	}
	return 0;
}

static int
lcreate_res_path(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (!pkg) {
		return 0;
	}

	int img_count = (int)(luaL_checkinteger(L, 2));
	int lod_count = (int)(luaL_checkinteger(L, 3));
	pkg->rp = dtex_res_path_create(img_count, lod_count);

	return 0;
}

static int
lset_res_path(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (!pkg) {
		return 0;
	}
	struct dtex_res_path* rp = pkg->rp;

	const char* path = luaL_checkstring(L, 2);

	const char* type = luaL_checkstring(L, 3);
	if (strcmp(type, "epe") == 0) {
		dtex_set_epe_filepath(rp, path);
	} else if (strcmp(type, "ept") == 0) {
		dtex_set_ept_filepath(rp, path);
	} else if (strcmp(type, "img") == 0) {
		int idx = (int)(luaL_checkinteger(L, 4));
		int lod = (int)(luaL_checkinteger(L, 5));
		dtex_set_img_filepath(rp, path, idx, lod);
	}

	return 0;
}

static int
lcreate_c3_stg(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (!pkg) {
		return 0;
	}

	struct dtex_c3_stg_cfg c3_cfg;
	struct dtex_c3_stg_cfg* c3_cfg_ptr = NULL;
	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "clear_enable");
		c3_cfg.clear_enable = lua_toboolean(L, -1);
		lua_pop(L, 1);
		c3_cfg_ptr = &c3_cfg;
	}

	if (c3_cfg_ptr) {
		pkg->c3_stg = dtex_c3_strategy_create(c3_cfg_ptr);
	}

	return 0;
}

static int
lcreate_c2_stg(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (!pkg) {
		return 0;
	}

	struct dtex_c2_stg_cfg c2_cfg;
	struct dtex_c2_stg_cfg* c2_cfg_ptr = NULL;
	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "clear_enable");
		c2_cfg.clear_enable = lua_toboolean(L, -1);
		lua_getfield(L, 2, "single_max_count");
		c2_cfg.single_max_count = (int)(luaL_checkinteger(L, -1));
		lua_getfield(L, 2, "diff_spr_count");
		c2_cfg.diff_spr_count = (int)(luaL_checkinteger(L, -1));
		lua_getfield(L, 2, "tot_count");
		c2_cfg.tot_count = (int)(luaL_checkinteger(L, -1));
		lua_pop(L, 4);
		c2_cfg_ptr = &c2_cfg;
	}

	if (c2_cfg_ptr) {
		pkg->c2_stg = dtex_c2_strategy_create(pkg->ej_pkg->n, c2_cfg_ptr);
	}

	return 0;
}

static int
lpreload_all_textures(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	struct dtex_package* pkg = lua_touserdata(L, 2);
	if (!pkg) {
		return 0;
	}

	float scale = luaL_optnumber(L, 3, 1);

	int sz = dtexf_preload_all_textures(path, pkg, scale);
	lua_pushinteger(L, sz);

	return 1;
}

static int
lpreload_texture(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (!pkg) {
		return 0;
	}

	int idx = (int)(lua_tointeger(L, 2));
	float scale = luaL_optnumber(L, 3, 1);

	dtexf_preload_texture(pkg, idx, scale);

	return 0;
}

static int
lload_texture(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (!pkg) {
		return 0;
	}

	int idx = (int)lua_tointeger(L, 2);

	dtexf_load_texture(pkg, idx);

	return 0;
}

static int
lload_image(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	struct dtex_texture* tex = dtexf_load_image(path);

	if (tex) {
		lua_pushinteger(L, tex->uid);
		lua_pushinteger(L, tex->width);
		lua_pushinteger(L, tex->height);
		return 3;
	}

	return 0;
}

static int
lquery(lua_State* L) {
	const char* pkg_name = luaL_checkstring(L, 1);
	const char* spr_name = luaL_checkstring(L, 2);

	struct dtex_package* pkg = dtexf_query_pkg(pkg_name);
	if (!pkg) {
		return 0;
	}

	lua_pushlightuserdata(L, pkg);

	int spr_id = dtex_get_spr_id(pkg, spr_name);
	lua_pushinteger(L, spr_id);

	return 2;
}

static int
lclear_state(lua_State* L) {
	dtex_async_loader_clear();
	dtex_texture_cache_clear();
	dtexf_c2_clear();
	return 0;
}

static int
lis_task_finished(lua_State* L) {
	bool finished = dtex_async_loader_empty();
	lua_pushboolean(L, finished);
	return 1;
}

/************************************************************************/
/* c1                                                                   */
/************************************************************************/

static int
lt0_bind(lua_State* L) {
	dtexf_t0_bind();
	return 0;
}

static int
lt0_unbind(lua_State* L) {
	dtexf_t0_unbind();
	return 0;
}

static int
lt0_draw(lua_State* L) {
	float src_w = luaL_optnumber(L, 1, 1),
		  src_h = luaL_optnumber(L, 2, 1);
	float dst_w = luaL_optnumber(L, 3, 1),
		  dst_h = luaL_optnumber(L, 4, 1);
	dtexf_t0_draw(src_w, src_h, dst_w, dst_h);
	return 0;
}

static int
lc1_set_viewport(lua_State* L) {
	dtexf_c1_set_viewport();
	return 0;
}

static int
lc1_draw_between(lua_State* L) {
	bool t0tot1 = lua_toboolean(L, 1);
	float src_w = luaL_optnumber(L, 2, 1),
		  src_h = luaL_optnumber(L, 3, 1);
	float dst_w = luaL_optnumber(L, 4, 1),
		  dst_h = luaL_optnumber(L, 5, 1);
	dtexf_c1_draw_between(t0tot1, src_w, src_h, dst_w, dst_h);
	return 0;
}

/************************************************************************/
/* c3                                                                   */
/************************************************************************/

static int
lc3_load(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (!pkg) {
		return 0;
	}
	float scale = luaL_optnumber(L, 2, 1);
	dtexf_c3_load(pkg, scale);
	return 0;
}

static int
lc3_load_end(lua_State* L) {
	bool async = lua_toboolean(L, 1);
	dtexf_c3_load_end(async);
	return 0;
}

static int
lc3_clear(lua_State* L) {
	dtexf_c3_clear();
	return 0;
}

/************************************************************************/
/* c4                                                                   */
/************************************************************************/

static int
lc4_load(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	if (pkg) {
		dtexf_c4_load(pkg);
	}
	return 0;
}

static int
lc4_load_end(lua_State* L) {
	bool async = lua_toboolean(L, 1);
	dtexf_c4_load_end(async);
	return 0;
}

static int
lc4_clear(lua_State* L) {
	dtexf_c4_clear();
	return 0;
}

static int
lcs1_bind(lua_State* L) {
	dtexf_cs1_bind();
	return 0;
}

static int
lcs1_unbind(lua_State* L) {
	dtexf_cs1_unbind();
	return 0;
}

static int
lcs1_draw(lua_State* L) {
	float src_w = luaL_optnumber(L, 1, 1),
		  src_h = luaL_optnumber(L, 2, 1);
	float dst_w = luaL_optnumber(L, 3, 1),
		  dst_h = luaL_optnumber(L, 4, 1);
	dtexf_cs1_draw(src_w, src_h, dst_w, dst_h, NULL, NULL);
	return 0;
}

static int
lcs2_bind(lua_State* L) {
	dtexf_cs2_bind();
	return 0;
}

static int
lcs2_unbind(lua_State* L) {
	dtexf_cs2_unbind();
	return 0;
}

static int
lcs2_draw(lua_State* L) {
	float src_w = luaL_optnumber(L, 1, 1),
		  src_h = luaL_optnumber(L, 2, 1);
	float dst_w = luaL_optnumber(L, 3, 1),
		  dst_h = luaL_optnumber(L, 4, 1);
	dtexf_cs2_draw(src_w, src_h, dst_w, dst_h, NULL, NULL);
	return 0;
}

static int
lcs_set_viewport(lua_State* L) {
	dtexf_cs_set_viewport();
	return 0;
}

int
luaopen_dtex_c(lua_State* L) {
	luaL_Reg l[] = {
		// overall
		{ "create", lcreate },
		{ "release", lrelease },

		// normal
		{ "load_package", lload_package },
		{ "unload_package", lunload_package },

		{ "create_res_path", lcreate_res_path },
		{ "set_res_path", lset_res_path },

		{ "create_c3_stg", lcreate_c3_stg },
		{ "create_c2_stg", lcreate_c2_stg },

		{ "preload_all_textures", lpreload_all_textures },
		{ "preload_texture", lpreload_texture },
		{ "load_texture", lload_texture },
		{ "load_image", lload_image },		

		{ "query", lquery },

		{ "clear_state", lclear_state },

		{ "is_task_finished", lis_task_finished },

		// C1
		{ "t0_bind", lt0_bind },
		{ "t0_unbind", lt0_unbind },
		{ "t0_draw", lt0_draw },
		{ "c1_set_viewport", lc1_set_viewport },
		{ "c1_draw_between", lc1_draw_between },

		// C3
		{ "c3_load", lc3_load },
		{ "c3_load_end", lc3_load_end },
		{ "c3_clear", lc3_clear },

		// C4
		{ "c4_load", lc4_load },
		{ "c4_load_end", lc4_load_end },
		{ "c4_clear", lc4_clear },

		// CS
		{ "cs1_bind", lcs1_bind },
		{ "cs1_unbind", lcs1_unbind },		
		{ "cs1_draw", lcs1_draw },
		{ "cs2_bind", lcs2_bind },
		{ "cs2_unbind", lcs2_unbind },
		{ "cs2_draw", lcs2_draw },
		{ "cs_set_viewport", lcs_set_viewport },

		{ NULL, NULL },		
	};
	luaL_newlib(L, l);
	return 1;
}