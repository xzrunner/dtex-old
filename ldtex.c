#include "dtex_facade.h"

#include "sprite.h"

#include "lua.h"
#include "lauxlib.h"

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

static int
lload_pkg(lua_State* L) {
// 	const char* name = luaL_checkstring(L, 1);
// 	const char* path = luaL_checkstring(L, 2);
//	dtexf_load_pkg(name, path);
	return 0;
}

static int
lc3_load_pkg(lua_State* L) {
// 	const char* name = luaL_checkstring(L, 1);
// 	const char* path = luaL_checkstring(L, 2);
// 	float scale = luaL_optnumber(L, 3, 1);
// 
// 	struct ej_package* pkg = dtexf_c3_load_pkg(name, path, scale);
// 	lua_pushlightuserdata(L, pkg);
	return 1;
}

static int
lc3_load_pkg_finish(lua_State* L) {
	dtexf_c3_load_end();
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
lualib_dtex(lua_State* L) {
	luaL_Reg l[] = {
		{ "create", lcreate },
		{ "release", lrelease },

		// loading
		{ "load_pkg", lload_pkg },

		// C3
		{ "c3_load_pkg", lc3_load_pkg },
		{ "c3_load_pkg_finish", lc3_load_pkg_finish },

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