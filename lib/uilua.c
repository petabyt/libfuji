
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "ui.h"


/*
 * Boilerplate macros
 */

struct wrap {
	uiControl *control;
};

#define CAST_ARG(n, type) ui ## type(((struct wrap *)lua_touserdata(L, n))->control)

#define CREATE_META(n) \
	luaL_newmetatable(L, "libui." #n);  \
	luaL_setfuncs(L, meta_ ## n, 0);

#define CREATE_OBJECT(t, c) create_object(L, t, uiControl(c));

static void create_callback_data(lua_State *L, int n)
{
	/* Push registery key: userdata pointer to control */

	lua_pushlightuserdata(L, CAST_ARG(1, Control));

	/* Create table with callback data */

	lua_newtable(L);

	lua_pushstring(L, "foo");
	lua_pushinteger(L, 123);

	lua_settable(L, -3);

	lua_pushvalue(L, 1);
	lua_setfield(L, -2, "udata");

	lua_pushvalue(L, 2);
	lua_setfield(L, -2, "fn");

	lua_pushvalue(L, 3);
	lua_setfield(L, -2, "data");

	/* Store in registry */

	lua_settable(L, LUA_REGISTRYINDEX);

}

static void callback(lua_State *L, void *control)
{
	/* Find table with callback data in registry */

	lua_pushlightuserdata(L, control);
	lua_gettable(L, LUA_REGISTRYINDEX);

	/* Get function, control userdata and callback data */

	luaL_checktype(L, 1, LUA_TTABLE);

	lua_pushstring(L, "fn");
	lua_gettable(L, -2);
	luaL_checktype(L, -1, LUA_TFUNCTION);

	lua_pushstring(L, "udata");
	lua_gettable(L, -3);

	lua_pushstring(L, "data");
	lua_gettable(L, -4);

	/* Call function */

	if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
		char error_buffer[128];
		snprintf(error_buffer, sizeof(error_buffer), "%s", lua_tostring(L, -1));
		uiToast(error_buffer);

		// Ruin the entire script, mark it dead
		extern int lua_mark_dead(lua_State *L);
		lua_mark_dead(L);
	}

	/* Cleanup stack */

	lua_pop(L, 1);
}


int l_gc(lua_State *L)
{
	return 0;

	struct wrap *w = lua_touserdata(L, 1);
	uint32_t s = w->control->TypeSignature;
	printf("gc %p %c%c%c%c\n", w->control, s >> 24, s >> 16, s >> 8, s >> 0);

	uiControl *control = CAST_ARG(1, Control);
	uiControl *parent = uiControlParent(control);

	if(parent) {
		if(parent->TypeSignature == 0x57696E64) {
			//uiWindowSetChild(uiWindow(parent), NULL);
		}
		if(parent->TypeSignature == 0x47727062) {
			//uiGroupSetChild(uiWindow(parent), NULL);
		}
	}
	//uiControlDestroy(control);

	return 0;
}

static void create_meta(lua_State *L, const char *name, struct luaL_Reg *reg) {
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "libui.%s", name);
	luaL_newmetatable(L, buffer);
	luaL_setfuncs(L, reg, 0);
}

static void create_object(lua_State *L, const char *t, uiControl *c) {
	struct wrap *w = lua_newuserdata(L, sizeof(struct wrap));
	w->control = uiControl(c);
	lua_newtable(L);
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "libui.%s", t);
	luaL_getmetatable(L, buffer);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, l_gc);
	lua_setfield(L, -2, "__gc");
	lua_setmetatable(L, -2);
}

/*
 * Box
 */

int l_NewVerticalBox(lua_State *L)
{
	create_object(L, "Box", uiControl(uiNewVerticalBox()));
	return 1;
}

int l_NewHorizontalBox(lua_State *L)
{
	CREATE_OBJECT("Box", uiNewHorizontalBox());
	return 1;
}

int l_BoxAppend(lua_State *L)
{
	int n = lua_gettop(L);
	int stretchy = 0;

	if(lua_isnumber(L, n) || lua_isboolean(L, n)) {
		stretchy = lua_toboolean(L, n);
	}

	int i;

	for(i=2; i<=n; i++) {
		if(lua_isuserdata(L, i)) {
			uiBoxAppend(CAST_ARG(1, Box), CAST_ARG(i, Control), stretchy);
			lua_getmetatable(L, 1);
			lua_pushvalue(L, i);
			luaL_ref(L, -2);
		}
	}
	lua_pushvalue(L, 1);
	return 1;;
}

int l_BoxPadded(lua_State *L)
{
	lua_pushnumber(L, uiBoxPadded(CAST_ARG(1, Box)));
	return 1;
}

int l_BoxSetPadded(lua_State *L)
{
	uiBoxSetPadded(CAST_ARG(1, Box), luaL_checknumber(L, 2));
	lua_pushvalue(L, 1);
	return 1;;
}


static struct luaL_Reg meta_Box[] = {
		{ "Append",               l_BoxAppend },
		{ "Padded",               l_BoxPadded },
		{ "SetPadded",            l_BoxSetPadded },
		{ NULL }
};


/*
 * Button
 */

int l_NewButton(lua_State *L)
{
	CREATE_OBJECT("Button", uiNewButton(
			luaL_checkstring(L, 1)
	));
	return 1;
}

static void on_button_clicked(uiButton *b, void *data)
{
	callback(data, b);
}

int l_ButtonSetText(lua_State *L)
{
	uiButtonSetText(CAST_ARG(1, Button), luaL_checkstring(L, 2));
	lua_pushvalue(L, 1);
	return 1;;
}

int l_ButtonOnClicked(lua_State *L)
{
	uiButtonOnClicked(CAST_ARG(1, Button), on_button_clicked, L);
	create_callback_data(L, 1);
	lua_pushvalue(L, 1);
	return 1;;
}

static struct luaL_Reg meta_Button[] = {
		{ "SetText",              l_ButtonSetText },
		{ "OnClicked",            l_ButtonOnClicked },
		{ NULL }
};

/*
 * Control
 */

int l_ControlShow(lua_State *L)
{
	uiControlShow(CAST_ARG(1, Control));
	lua_pushvalue(L, 1);
	return 1;;
}


int l_ControlDestroy(lua_State *L)
{
	printf("destroy not implemented, garbage collection needs to be implemented\n");
	uiControlDestroy(CAST_ARG(1, Control));
	return 0;
}

/*
 * Group
 */

int l_NewGroup(lua_State *L)
{
	CREATE_OBJECT("Group", uiNewGroup(
			luaL_checkstring(L, 1)
	));
	return 1;
}

int l_GroupTitle(lua_State *L)
{
	lua_pushstring(L, uiGroupTitle(CAST_ARG(1, Group)));
	return 1;
}

int l_GroupSetTitle(lua_State *L)
{
	const char *title = luaL_checkstring(L, 2);
	uiGroupSetTitle(CAST_ARG(1, Group), title);
	lua_pushvalue(L, 1);
	return 1;;
}

int l_GroupSetChild(lua_State *L)
{
	uiGroupSetChild(CAST_ARG(1, Group), CAST_ARG(2, Control));
	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_pushboolean(L, 1);
	lua_settable(L, -3);
	lua_pushvalue(L, 1);
	return 1;;
}

int l_GroupMargined(lua_State *L)
{
	lua_pushnumber(L, uiGroupMargined(CAST_ARG(1, Group)));
	return 1;
}

int l_GroupSetMargined(lua_State *L)
{
	uiGroupSetMargined(CAST_ARG(1, Group), luaL_checknumber(L, 2));
	lua_pushvalue(L, 1);
	return 1;;
}

static struct luaL_Reg meta_Group[] = {
		{ "Title",                l_GroupTitle },
		{ "SetTitle",             l_GroupSetTitle },
		{ "SetChild",             l_GroupSetChild },
		{ "Margined",             l_GroupMargined },
		{ "SetMargined",          l_GroupSetMargined },
		{ NULL }
};


/*
 * Label
 */

int l_NewLabel(lua_State *L)
{
	CREATE_OBJECT("Label", uiNewLabel(
			luaL_checkstring(L, 1)
	));
	return 1;
}

int l_LabelText(lua_State *L)
{
	lua_pushstring(L, uiLabelText(CAST_ARG(1, Label)));
	return 1;
}

int l_LabelSetText(lua_State *L)
{
	uiLabelSetText(CAST_ARG(1, Label), luaL_checkstring(L, 2));
	lua_pushvalue(L, 1);
	return 1;;
}


static struct luaL_Reg meta_Label[] = {
		{ "Text",                 l_LabelText },
		{ "SetText",              l_LabelSetText },
		{ NULL }
};




/*
 * ProgressBar
 */

int l_NewProgressBar(lua_State *L)
{
	CREATE_OBJECT("ProgressBar", uiNewProgressBar());
	return 1;
}

int l_ProgressBarSetValue(lua_State *L)
{
	double value = luaL_checknumber(L, 2);
	uiProgressBarSetValue(CAST_ARG(1, ProgressBar), value);
	lua_pushvalue(L, 1);
	return 1;;
}

static struct luaL_Reg meta_ProgressBar[] = {
		{ "SetValue",             l_ProgressBarSetValue },
		{ NULL }
};

/*
 * Separator
 */

int l_NewHorizontalSeparator(lua_State *L)
{
	CREATE_OBJECT("Separator", uiNewHorizontalSeparator());
	return 1;
}

static struct luaL_Reg meta_Separator[] = {
		{ NULL }
};

/*
 * Tab
 */

int l_NewTab(lua_State *L)
{
	CREATE_OBJECT("Tab", uiNewTab());
	return 1;
}

int l_TabAppend(lua_State *L)
{
	int n = lua_gettop(L);
	int i;
	for(i=2; i<=n; i+=2) {
		uiTabAppend(CAST_ARG(1, Tab), luaL_checkstring(L, i+0), CAST_ARG(i+1, Control));
		lua_getmetatable(L, 1);
		lua_pushvalue(L, i+1);
		luaL_ref(L, -2);
	}
	lua_pushvalue(L, 1);
	return 1;;
}

static struct luaL_Reg meta_Tab[] = {
		{ "Append",               l_TabAppend },
		{ NULL }
};

/*
 * Window
 */

int l_NewWindow(lua_State *L)
{
	CREATE_OBJECT("Window", uiNewWindow(
			luaL_checkstring(L, 1),
			luaL_checknumber(L, 2),
			luaL_checknumber(L, 3),
			lua_toboolean(L, 4)
	));

	return 1;
}

int l_WindowSetChild(lua_State *L)
{
	uiWindowSetChild(CAST_ARG(1, Window), CAST_ARG(2, Control));
	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_pushboolean(L, 1);
	lua_settable(L, -3);
	lua_pushvalue(L, 1);
	return 1;;
}

int l_MsgBox(lua_State *L)
{
	uiMsgBox(CAST_ARG(1, Window), luaL_checkstring(L, 2), luaL_checkstring(L, 3));
	lua_pushvalue(L, 1);
	return 1;;
}

static struct luaL_Reg meta_Window[] = {
		{ "SetChild",            l_WindowSetChild },
		{ "Show",                l_ControlShow },
		{ "Destroy",             l_ControlDestroy },
		{ "MsgBox",              l_MsgBox},
		{ NULL }
};

/*
 * Various top level
 */

static int l_Toast(lua_State *L)
{
	uiToast(luaL_checkstring(L, 1));
	lua_pushvalue(L, 1);
	return 1;;
}

static struct luaL_Reg lui_table[] = {
		{ "NewButton",              l_NewButton },
		{ "NewGroup",               l_NewGroup },
		{ "NewHorizontalBox",       l_NewHorizontalBox },
		{ "NewVerticalBox",         l_NewVerticalBox },
		{ "NewHorizontalSeparator", l_NewHorizontalSeparator },
		{ "NewLabel",               l_NewLabel },
		{ "NewProgressBar",         l_NewProgressBar },
		{ "NewTab",                 l_NewTab },
		{ "NewWindow",              l_NewWindow },
		{ "Toast",                  l_Toast},
		{ NULL }
};

int libuilua_from_control(lua_State *L, uiControl *c) {
	CREATE_OBJECT("Button", c);
	return 0;
}

int luaopen_libuilua(lua_State *L)
{
	CREATE_META(Box)
	CREATE_META(Button)
	CREATE_META(Group)
	CREATE_META(Label)
	CREATE_META(ProgressBar)
	CREATE_META(Separator)
	CREATE_META(Tab)
	CREATE_META(Window)
	luaL_newlib(L, lui_table);
	return 1;
}

/*
 * End
 */
