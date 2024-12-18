#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <camlib.h>
#include "fuji_lua.h"

static char error_buffer[512] = {0};

static struct CamLuaTasks {
	int tasks;
	struct lua_State *L[MAX_LUA_CONCURRENT];
	int state[MAX_LUA_CONCURRENT];
}lua_tasks = {0};

const char *cam_lua_get_error(void) {
	return error_buffer;
}

int lua_script_run_loop(int id) {
	if (id >= MAX_LUA_CONCURRENT || id < 0) {
		snprintf(error_buffer, sizeof(error_buffer), "Script: out of bounds ID: %d\n", id);
		return 1;
	}

	struct lua_State *L = lua_tasks.L[(int)id];

	if (L == NULL) return -1;

	lua_getglobal(L, "eventLoop");

	if (lua_isfunction(L, -1)) {
		if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
			snprintf(error_buffer, sizeof(error_buffer), "Failed to run Lua: %s", lua_tostring(L, -1));
			return -1;
		}
	} else {
		snprintf(error_buffer, sizeof(error_buffer), "eventLoop is not a function: %s", lua_tostring(L, -1));
		return -1;
	}

	return 0;
}

static int get_task_id(lua_State *L) {
	for (int i = 0; i < lua_tasks.tasks; i++) {
		if (L == lua_tasks.L[i]) return i;
	}

	return -1;
}

static int lua_script_print(lua_State *L) {
	const char *str = luaL_checkstring(L, 1);
	ptp_verbose_log("%s\n", str);
	return 1;
}

static int lua_script_toast(lua_State *L) {
	const char *str = luaL_checkstring(L, 1);
	ptp_verbose_log("%s\n", str);
	return 1;
}

static int lua_script_set_status(lua_State *L) {
	const char *str = luaL_checkstring(L, 1);
	//ui_send_text("status", (char *)str);
	return 1;
}

static int mylua_itoa(lua_State* L) {
	int number = (int)luaL_checkinteger(L, 1);
	char buffer[20];
	snprintf(buffer, sizeof(buffer), "%d", number);
	lua_pushstring(L, buffer);
	return 1;
}

int lua_mark_dead(lua_State *L) {
	int id = get_task_id(L);
	if (id == -1) abort();
	lua_tasks.state[id] = 1;
	return 1;
}

lua_State *cam_lua_state(void) {
	lua_State *L = luaL_newstate();
	luaopen_base(L);
	luaL_requiref(L, "json", luaopen_cjson, 1);
	luaL_requiref(L, "ptp", luaopen_ptp, 1);
	luaL_requiref(L, "fuji", luaopen_fuji, 1);
	lua_register(L, "print", lua_script_print);
	lua_register(L, "exit", lua_mark_dead);
	lua_register(L, "setStatusText", lua_script_set_status);
	return L;
}

int cam_run_lua_script(const char *buffer) {
	lua_State *L = cam_lua_state();
	if (L == NULL) {
		snprintf(error_buffer, sizeof(error_buffer), "Reached max concurrent Lua tasks");
		return -1;
	}

	if (luaL_loadbuffer(L, buffer, strlen(buffer), "script") != LUA_OK) {
		snprintf(error_buffer, sizeof(error_buffer), "Failed to run Lua: %s", lua_tostring(L, -1));
		lua_close(L);
		return -1;
	}

	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		snprintf(error_buffer, sizeof(error_buffer), "Failed to run Lua: %s", lua_tostring(L, -1));
		lua_close(L);
		return -1;
	}

	lua_close(L);
	return 0;
}

lua_State *cam_new_task(int *id) {
	(*id) = -1;
	if (lua_tasks.tasks >= MAX_LUA_CONCURRENT) {
		for (int i = 0; i < lua_tasks.tasks; i++) {
			if (lua_tasks.state[i] == 1) {
				(*id) = i;
				lua_close(lua_tasks.L[i]);
				lua_tasks.L[i] = 0;
				break;
			}
		}
		if ((*id) == -1) {
			return NULL;
		}
	} else {
		(*id) = lua_tasks.tasks;
		lua_tasks.tasks++;
	}

	lua_State *L = cam_lua_state();

	lua_tasks.L[(*id)] = L;
	lua_tasks.state[(*id)] = 0;

	return L;
}

int cam_run_lua_script_async(const char *buffer) {
	int id = 0;
	lua_State *L = cam_new_task(&id);
	if (L == NULL) {
		snprintf(error_buffer, sizeof(error_buffer), "Reached max concurrent Lua tasks");
		return -1;
	}

	if (luaL_loadbuffer(L, buffer, strlen(buffer), "script") != LUA_OK) {
		snprintf(error_buffer, sizeof(error_buffer), "Failed to run Lua: %s", lua_tostring(L, -1));
		lua_close(L);
		return -1;
	}

	if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
		snprintf(error_buffer, sizeof(error_buffer), "Failed to run Lua: %s", lua_tostring(L, -1));
		lua_close(L);
		return -1;
	}

	return id;
}
