
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <camlib.h>
#include <fujiptp.h>
#include "fuji_lua.h"

static int mylua_set_property(lua_State *L) {
	struct PtpRuntime *r = luaptp_get_runtime(L);

	const char *name = luaL_checkstring(L, 1);
	int value = lua_tointeger(L, 1);

	int rc = ptp_set_generic_property(r, name, value);

	lua_pushinteger(L, rc);

	return 1;
}

static int mylua_device_info(lua_State *L) {
	struct PtpRuntime *r = luaptp_get_runtime(L);

	struct PtpDeviceInfo	 di;
	int rc = ptp_get_device_info(r, &di);
	if (rc) {
		lua_pushinteger(L, rc);
		return 1;
	}

	char buffer[4096];
	ptp_device_info_json(&di, buffer, sizeof(buffer));

	lua_json_decode(L, buffer, strlen(buffer));

	return 1;
}

static int mylua_take_picture(lua_State *L) {
	struct PtpRuntime *r = luaptp_get_runtime(L);

	if (r->di == NULL) return 1;

	int rc = ptp_pre_take_picture(r);
	if (rc) goto err;

	rc = ptp_take_picture(r);
	if (rc) goto err;

	err:;
	lua_pushinteger(L, rc);

	return 1;
}

static int mylua_send_operation(lua_State *L) {
	struct PtpRuntime *r = luaptp_get_runtime(L);

	int opcode = lua_tointeger(L, 1);
	int len = lua_gettop(L);

	if (!lua_istable(L, 2)) {
		return luaL_error(L, "arg2 expected array");
	}

	struct PtpCommand cmd;
	cmd.code = opcode;

	// Read parameters
	int param_length = 0;
	if (len >= 2) {
		param_length = luaL_len(L, 2);
		for (int i = 1; i <= param_length; ++i) {
			lua_rawgeti(L, 2, i);
			cmd.params[i - 1] = luaL_checkinteger(L, -1);
			lua_pop(L, 1);
		}
		cmd.param_length = param_length;
	}

	// Read payload if provided
	int data_length = 0;
	uint8_t *data_array = NULL;
	if (len >= 3) {
		data_length = luaL_len(L, 3);
		data_array = malloc(data_length * sizeof(int));
		for (int i = 1; i <= data_length; ++i) {
			lua_rawgeti(L, 3, i);
			data_array[i - 1] = (uint8_t)luaL_checkinteger(L, -1);
			lua_pop(L, 1);
		}
	}

	int rc = 0;
	if (data_array == NULL) {
		rc = ptp_generic_send(r, &cmd);
	} else {
		rc = ptp_generic_send_data(r, &cmd, data_array, data_length);
	}

	lua_newtable(L);

	lua_pushstring(L, "error");
	lua_pushinteger(L, rc);
	lua_settable(L, -3);

	lua_pushstring(L, "code");
	lua_pushinteger(L, ptp_get_return_code(r));
	lua_settable(L, -3);

	lua_pushstring(L, "payload");
	lua_newtable(L);
	for (int i = 0; i < ptp_get_payload_length(r); ++i) {
		lua_pushinteger(L, i + 1);
		lua_pushinteger(L, (int)(r->data[i]));
		lua_settable(L, -3);
	}

	lua_settable(L, -3);

	lua_pushstring(L, "id");
	lua_pushinteger(L, ptp_get_last_transaction_id(r));
	lua_settable(L, -3);

	return 1;
}

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
