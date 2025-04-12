
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <libpict.h>
#include <fujiptp.h>
#include "fuji_lua.h"

static int mylua_test(lua_State *L) {
	struct PtpRuntime *r = luaptp_get_runtime(L);

	char *t = "{\"fujifilm\": 123}";

	//json_create_config(L);
	lua_json_decode(L, t, strlen(t));

	return 1;
}

static int mylua_connect(lua_State *L) {
	struct PtpRuntime *r = luaptp_get_runtime(L);
	lua_pushinteger(L, 0);
	return 1;
}

static const luaL_Reg fujilib[] = {
	{"test",			mylua_test},
#if 0
	disconnect
	getTransport
	getObjectHandles
#endif
	{NULL, NULL}
};

static void new_const(lua_State *L, char *name, int val) {
	lua_pushstring(L, name);
	lua_pushnumber(L, val);
	lua_settable(L, -3);
}

LUALIB_API int luaopen_fuji(lua_State *L) {
	luaL_newlib(L, fujilib);

	new_const(L, "FUJI_FEATURE_AUTOSAVE", FUJI_FEATURE_AUTOSAVE);
	new_const(L, "FUJI_FEATURE_WIRELESS_TETHER", FUJI_FEATURE_WIRELESS_TETHER);
	new_const(L, "FUJI_FEATURE_WIRELESS_TETHER", FUJI_FEATURE_WIRELESS_TETHER);
	new_const(L, "FUJI_FEATURE_WIRELESS_COMM", FUJI_FEATURE_WIRELESS_COMM);
	new_const(L, "FUJI_FEATURE_USB", FUJI_FEATURE_USB);
	new_const(L, "FUJI_FEATURE_USB_CARD_READER", FUJI_FEATURE_USB_CARD_READER);
	new_const(L, "FUJI_FEATURE_USB_TETHER_SHOOT", FUJI_FEATURE_USB_TETHER_SHOOT);
	new_const(L, "FUJI_FEATURE_RAW_CONV", FUJI_FEATURE_RAW_CONV);

	return 1;
}
