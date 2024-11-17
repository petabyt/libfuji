#define MAX_LUA_CONCURRENT 5

// Must be provided
//extern int cam_lua_setup(lua_State *L);

LUALIB_API int luaopen_fuji(lua_State *L);

extern struct PtpRuntime *luaptp_get_runtime(lua_State *L);

// lua-cjson
int luaopen_cjson(lua_State *l);
void lua_json_decode(lua_State *l, const char *json_text, int json_len);
