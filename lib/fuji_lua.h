#ifndef CAM_LUA_H
#define CAM_LUA_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

LUALIB_API int luaopen_ptp(lua_State *L);

lua_State *cam_new_task(int *id);

int lua_script_run_loop(int id);

int cam_run_lua_script(const char *buffer);

int cam_run_lua_script_async(const char *buffer);

const char *cam_lua_get_error(void);

#define MAX_LUA_CONCURRENT 5

// Must be provided
//extern int cam_lua_setup(lua_State *L);

LUALIB_API int luaopen_fuji(lua_State *L);

extern struct PtpRuntime *luaptp_get_runtime(lua_State *L);

// lua-cjson
int luaopen_cjson(lua_State *l);
void lua_json_decode(lua_State *l, const char *json_text, int json_len);

#endif
