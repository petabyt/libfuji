/** \file */
#ifndef CAM_LUA_H
#define CAM_LUA_H

#include <lua.h>

/// @brief Adds `ptp.` commands to a Lua state
LUALIB_API int luaopen_ptp(lua_State *L);

/// @brief Create a new global PTP/Lua task
/// @param id Returned value of task ID
lua_State *cam_new_task(int *id);

/// @brief Calls the eventLoop function once from task ID
/// @returns Non-zero for Lua error
int lua_script_run_loop(int id);

/// @brief Quickly run a Lua script, no task creation involved
/// @returns Non-zero for error
int cam_run_lua_script(const char *buffer);

/// @brief Creates a global PTP/Lua task from ASCII string buffer and runs init code
/// @returns task ID
int cam_run_lua_script_async(const char *buffer);

/// @brief Returns string of last error
const char *cam_lua_get_error(void);

#define MAX_LUA_CONCURRENT 5

/// @brief Adds a `fuji.` namespace for Fuji-specific functionality
LUALIB_API int luaopen_fuji(lua_State *L);

/// @returns The PTP runtime structure that corresponds to the Lua state
extern struct PtpRuntime *luaptp_get_runtime(lua_State *L);

// lua-cjson
int luaopen_cjson(lua_State *l);
void lua_json_decode(lua_State *l, const char *json_text, int json_len);

#endif
