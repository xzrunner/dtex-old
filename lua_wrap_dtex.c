#include "dtex_facade.h"
#include "dtex_typedef.h"
#include "dtex_package.h"
#include "dtex_shader.h"
#include "dtex_gl.h"

#include "dtex_log.h"

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
	dtex_shader_load();
	return 0;
}

static int
lrelease(lua_State* L) {
	dtex_shader_unload();
	dtexf_release();
	return 0;
}

/************************************************************************/
/* normal                                                               */
/************************************************************************/

static int
lload_package(lua_State* L) {
	dtex_debug("+++++++++++++ lload_package 0");
	const char* name = luaL_checkstring(L, 1);
	const char* path = luaL_checkstring(L, 2);
	const char* stype = luaL_checkstring(L, 3);
	int lod = luaL_optinteger(L, 4, 0);
	int load_c3 = luaL_optinteger(L, 5, 0);
	int load_c2 = luaL_optinteger(L, 6, 0);

	int itype = FILE_INVALID;
	if (strcmp(stype, "epe") == 0) {
		itype = FILE_EPE;
	} else {
		luaL_error(L, "unknown file type %s", stype);
	}

	dtex_debug("+++++++++++++ lload_package 1");
	struct dtex_package* pkg = dtexf_load_pkg(name, path, itype, 1, lod, load_c3, load_c2);
	dtex_debug("+++++++++++++ lload_package 2");
	lua_pushlightuserdata(L, pkg);

	return 1;
}

static int
lunload_package(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	dtexf_unload_pkg(pkg);
	return 0;
}

static int
lpreload_all_textures(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	struct dtex_package* pkg = lua_touserdata(L, 2);
	float scale = luaL_optnumber(L, 3, 1);

	int sz = dtexf_preload_all_textures(path, pkg, scale);
	lua_pushinteger(L, sz);

	return 1;
}

static int
lpreload_texture(lua_State* L) {
	dtex_debug("+++++++++++++ lpreload_texture 0");

	struct dtex_package* pkg = lua_touserdata(L, 1);
	int idx = lua_tointeger(L, 2);
	float scale = luaL_optnumber(L, 3, 1);

	dtex_debug("+++++++++++++ lpreload_texture 1");
	dtexf_preload_texture(pkg, idx, scale);
	dtex_debug("+++++++++++++ lpreload_texture 2");

	return 0;
}

static int
lload_texture(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	int idx = (int)lua_tointeger(L, 2);

	dtexf_load_texture(pkg, idx, true);

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

/************************************************************************/
/* c3                                                                   */
/************************************************************************/

static int
lc3_load(lua_State* L) {
	dtex_debug("+++++++++++++ lc3_load 0");
	struct dtex_package* pkg = lua_touserdata(L, 1);
	float scale = luaL_optnumber(L, 2, 1);
	dtexf_c3_load(pkg, scale);
	dtex_debug("+++++++++++++ lc3_load 1");
	return 0;
}

static int
lc3_load_end(lua_State* L) {
	dtex_debug("+++++++++++++ lc3_load_end 0");

	bool async = lua_toboolean(L, 1);
	dtexf_c3_load_end(async);

	dtex_debug("+++++++++++++ lc3_load_end 1");

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

		{ "preload_all_textures", lpreload_all_textures },
		{ "preload_texture", lpreload_texture },
		{ "load_texture", lload_texture },

		{ "query", lquery },

		// C3
		{ "c3_load", lc3_load },
		{ "c3_load_end", lc3_load_end },

		{ NULL, NULL },		
	};
	luaL_newlib(L, l);
	return 1;
}