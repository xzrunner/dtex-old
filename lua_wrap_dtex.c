#include "dtex_facade.h"
#include "dtex_typedef.h"
#include "dtex_package.h"
#include "dtex_shader.h"

#include <string.h>

#include <lua.h>
#include <lauxlib.h>

/************************************************************************/
/* overall                                                        */
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
lpreload_pkg(lua_State* L) {
	const char* name = luaL_checkstring(L, 1);
	const char* path = luaL_checkstring(L, 2);
	const char* stype = luaL_checkstring(L, 3);
	float scale = luaL_optnumber(L, 4, 1);

	int itype = FILE_INVALID;
	if (strcmp(stype, "ept") == 0) {
		itype = FILE_EPT;
	} else if (strcmp(stype, "epe") == 0) {
		itype = FILE_EPE;
	} else {
		luaL_error(L, "unknown file type %s", stype);
	}
	
 	struct dtex_package* pkg = dtexf_preload_pkg(name, path, itype, scale);
 	lua_pushlightuserdata(L, pkg);

	return 1;
}

static int
lload_texture(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	int idx = (int)lua_tointeger(L, 2);
	float scale = luaL_optnumber(L, 3, 1);

	dtexf_load_texture(pkg, idx, scale);
	return 0;
}

static int
lquery(lua_State* L) {
	const char* pkg_name = luaL_checkstring(L, 1);
	const char* spr_name = luaL_checkstring(L, 2);

	struct dtex_package* pkg = dtexf_query_pkg(pkg_name);
	if (!pkg) {
		int zz = 0;
	}
	lua_pushlightuserdata(L, pkg->ej_pkg);

	int spr_id = dtex_get_spr_id(pkg, spr_name);
	lua_pushinteger(L, spr_id);

	return 2;
}

/************************************************************************/
/* c3                                                                   */
/************************************************************************/

static int
lc3_load(lua_State* L) {
	struct dtex_package* pkg = lua_touserdata(L, 1);
	float scale = luaL_optnumber(L, 2, 1);
	dtexf_c3_load(pkg, scale);
	return 0;
}

static int
lc3_load_end(lua_State* L) {
	dtex_shader_texture(0);
	dtex_shader_program(PROGRAM_NULL);

	dtexf_c3_load_end(false);

	ej_shader_texture(0, 0);
	ej_shader_program(PROGRAM_PICTURE, NULL);

	return 0;
}

/************************************************************************/
/* debug                                                                */
/************************************************************************/

static int
ldebug_draw(lua_State* L) {
	dtexf_debug_draw();
	return 0;
}

//static int
//lc2_load_begin(lua_State* L) {
//	dtexf_c2_load_begin();
//	return 0;
//}
//
//static int
//lc2_load_spr(lua_State* L) {
//	struct ej_package* pkg = lua_touserdata(L,1);
//	const char* name = luaL_checkstring(L,2);
//	dtexf_c2_load_sprite(pkg, name);
//	return 0;
//}
//
//static int
//lc2_load_end(lua_State* L) {
//	dtexf_c2_load_end();
//	return 0;
//}
//
//static int
//lc1_cache_spr(lua_State* L) {
//	struct ej_sprite* spr = lua_touserdata(L, 1);
//	dtexf_c1_load_anim(spr->pack, spr->ani, spr->action);
//	return 0;
//}
//
//static int
//lasync_load_spr(lua_State* L) {
//	const char* pkg_name = luaL_checkstring(L, 1);
//	const char* spr_name = luaL_checkstring(L, 2);
//	const char* path = luaL_checkstring(L, 3);
//	dtexf_async_load_spr(pkg_name, spr_name, path);
//	return 0;
//}
//
//static int
//lupdate(lua_State* L) {
//	dtexf_update();
//	return 0;
//}
//
//static int
//ltest_pvr(lua_State* L) {
//	const char* path = luaL_checkstring(L, 1);
//	dtexf_test_pvr(path);
//	return 0;
//}

int
luaopen_dtex_c(lua_State* L) {
	luaL_Reg l[] = {
		// overall
		{ "create", lcreate },
		{ "release", lrelease },

		// normal
		{ "preload_pkg", lpreload_pkg },
		{ "load_texture", lload_texture },
		{ "query", lquery },

		// C3
		{ "c3_load", lc3_load },
		{ "c3_load_end", lc3_load_end },

		// debug
		{ "debug_draw", ldebug_draw },

// 		// C2
// 		{ "c2_load_begin", lc2_load_begin },
// 		{ "c2_load_spr", lc2_load_spr },
// 		{ "c2_load_end", lc2_load_end },
// 
// 		// C1
// 		{ "c1_cache_spr", lc1_cache_spr },
// 
// 		// async loading
// 		{ "async_load_spr", lasync_load_spr },
// 		{ "update", lupdate },
// 
// 		// test
// 		{ "test_pvr", ltest_pvr },

		{ NULL, NULL },		
	};
	luaL_newlib(L, l);
	return 1;
}