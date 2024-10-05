
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

static void create_meta(lua_State *L, const char *name, struct luaL_Reg *reg) {
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "libui.%s", name);
	luaL_newmetatable(L, buffer);
	luaL_setfuncs(L, reg, 0);
}

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
	create_object(L, "Box", uiControl(uiNewHorizontalBox()));
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
	create_object(L, "Button", uiControl(uiNewButton((luaL_checklstring(L, (1), ((void *) 0))))));
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
	create_object(L, "Group", uiControl(uiNewGroup((luaL_checklstring(L, (1), ((void *) 0))))));
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
	create_object(L, "Label", uiControl(uiNewLabel((luaL_checklstring(L, (1), ((void *) 0))))));
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
	create_object(L, "ProgressBar", uiControl(uiNewProgressBar()));
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
	create_object(L, "Separator", uiControl(uiNewHorizontalSeparator()));
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
	create_object(L, "Tab", uiControl(uiNewTab()));
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
	create_object(L, "Window", uiControl(
			uiNewWindow((luaL_checklstring(L, (1), ((void *) 0))), luaL_checknumber(L, 2),
						luaL_checknumber(L, 3), lua_toboolean(L, 4))));

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

#ifdef UNFINISHED
/*
 * Spinbox
 */

int l_NewSpinbox(lua_State *L)
{
	create_object(L, "Spinbox",
				  uiControl(uiNewSpinbox(luaL_checknumber(L, 1), luaL_checknumber(L, 2))));
	return 1;
}

int l_SpinboxValue(lua_State *L)
{
	lua_pushnumber(L, uiSpinboxValue(CAST_ARG(1, Spinbox)));
	return 1;
}

int l_SpinboxSetValue(lua_State *L)
{
	double value = luaL_checknumber(L, 2);
	uiSpinboxSetValue(CAST_ARG(1, Spinbox), value);
	lua_pushvalue(L, 1);
	return 1;
}

static void on_spinbox_changed(uiSpinbox *b, void *data)
{
	callback(data, b);
}

int l_SpinboxOnChanged(lua_State *L)
{
	uiSpinboxOnChanged(CAST_ARG(1, Spinbox), on_spinbox_changed, L);
	create_callback_data(L, 1);
	lua_pushvalue(L, 1);
	return 1;
}

static struct luaL_Reg meta_Spinbox[] = {
	{ "Value",                l_SpinboxValue },
	{ "SetValue",             l_SpinboxSetValue },
	{ "OnChanged",            l_SpinboxOnChanged },
	{ NULL }
};

/*
 * Slider
 */

int l_NewSlider(lua_State *L)
{
	create_object(L, "Slider",
				  uiControl(uiNewSlider(luaL_checknumber(L, 1), luaL_checknumber(L, 2))));
	return 1;
}

int l_SliderValue(lua_State *L)
{
	lua_pushnumber(L, uiSliderValue(CAST_ARG(1, Slider)));
	return 1;
}

int l_SliderSetValue(lua_State *L)
{
	double value = luaL_checknumber(L, 2);
	uiSliderSetValue(CAST_ARG(1, Slider), value);
	lua_pushvalue(L, 1);
	return 1;
}

static void on_slider_changed(uiSlider *b, void *data)
{
	callback(data, b);
}

int l_SliderOnChanged(lua_State *L)
{
	uiSliderOnChanged(CAST_ARG(1, Slider), on_slider_changed, L);
	create_callback_data(L, 1);
	lua_pushvalue(L, 1);
	return 1;
}

static struct luaL_Reg meta_Slider[] = {
	{ "Value",                l_SliderValue },
	{ "SetValue",             l_SliderSetValue },
	{ "OnChanged",            l_SliderOnChanged },
	{ NULL }
};

/*
 * RadioButtons
 */

int l_NewRadioButtons(lua_State *L)
{
	create_object(L, "RadioButtons", uiControl(uiNewRadioButtons()));
	return 1;
}


int l_RadioButtonsAppend(lua_State *L)
{
	int n = lua_gettop(L);
	int i;
	for(i=2; i<=n; i++) {
		const char *text = luaL_checkstring(L, i);
		uiRadioButtonsAppend(CAST_ARG(1, RadioButtons), text);
	}
	lua_pushvalue(L, 1);
	return 1;
}


static struct luaL_Reg meta_RadioButtons[] = {
	{ "Append",               l_RadioButtonsAppend },
	{ NULL }
};

/*
 * Date/Timepicker
 */

int l_NewDateTimePicker(lua_State *L)
{
	create_object(L, "DateTimePicker", uiControl(uiNewDateTimePicker()));
	return 1;
}

int l_NewDatePicker(lua_State *L)
{
	create_object(L, "DateTimePicker", uiControl(uiNewDatePicker()));
	return 1;
}

int l_NewTimePicker(lua_State *L)
{
	create_object(L, "DateTimePicker", uiControl(uiNewTimePicker()));
	return 1;
}

static struct luaL_Reg meta_DateTimePicker[] = {
	{ NULL }
};

/*
 * Combobox
 */

int l_NewCombobox(lua_State *L)
{
	create_object(L, "Combobox", uiControl(uiNewCombobox()));
	return 1;
}

int l_NewEditableCombobox(lua_State *L)
{
	create_object(L, "Combobox", uiControl(uiNewEditableCombobox()));
	return 1;
}

static void on_combobox_selected(uiCombobox *c, void *data)
{
	callback(data, c);
}

int l_ComboboxAppend(lua_State *L)
{
	int n = lua_gettop(L);
	int i;
	for(i=2; i<=n; i++) {
		const char *text = luaL_checkstring(L, i);
		uiComboboxAppend(CAST_ARG(1, Combobox), text);
	}
	lua_pushvalue(L, 1);
	return 1;
}

int l_ComboboxOnToggled(lua_State *L)
{
        uiComboboxOnSelected(CAST_ARG(1, Combobox), on_combobox_selected, L);
        create_callback_data(L, 1);
	lua_pushvalue(L, 1);
	return 1;
}

static struct luaL_Reg meta_Combobox[] = {
	{ "Append",               l_ComboboxAppend },
	{ "OnToggled",            l_ComboboxOnToggled },
	{ NULL }
};


/*
 * Checkbox
 */

int l_NewCheckbox(lua_State *L)
{
	create_object(L, "Checkbox",
				  uiControl(uiNewCheckbox((luaL_checklstring(L, (1), ((void *) 0))))));
	return 1;
}

static void on_checkbox_toggled(uiCheckbox *c, void *data)
{
	callback(data, c);
}

int l_CheckboxSetText(lua_State *L)
{
	uiCheckboxSetText(CAST_ARG(1, Checkbox), luaL_checkstring(L, 2));
	lua_pushvalue(L, 1);
	return 1;
}

int l_CheckboxOnToggled(lua_State *L)
{
        uiCheckboxOnToggled(CAST_ARG(1, Checkbox), on_checkbox_toggled, L);
        create_callback_data(L, 1);
	lua_pushvalue(L, 1);
	return 1;
}

static struct luaL_Reg meta_Checkbox[] = {
	{ "SetText",              l_CheckboxSetText },
	{ "OnToggled",            l_CheckboxOnToggled },
	{ NULL }
};

/*
 * Area
 */

int l_NewArea(lua_State *L)
{
	static struct uiAreaHandler ah;
	create_object(L, "Area", uiControl(uiNewArea(&ah)));
	return 1;
}

int l_AreaSetSize(lua_State *L)
{
	uiAreaSetSize(CAST_ARG(1, Area), luaL_checknumber(L, 2), luaL_checknumber(L, 3));
	lua_pushvalue(L, 1);
	return 1;
}

static struct luaL_Reg meta_Area[] = {
	{ "SetSize",              l_AreaSetSize },
	{ NULL }
};

#endif

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

#ifdef UNFINISHED
	{ "NewCheckbox",            l_NewCheckbox },
	{ "NewArea",                l_NewArea },
	{ "NewCombobox",            l_NewCombobox },
	{ "NewDateTimePicker",      l_NewDateTimePicker },
	{ "NewDatePicker",          l_NewDatePicker },
	{ "NewTimePicker",          l_NewTimePicker },
	{ "NewEditableCombobox",    l_NewEditableCombobox },
	{ "NewRadioButtons",        l_NewRadioButtons },
	{ "NewSlider",              l_NewSlider },
	{ "NewSpinbox",             l_NewSpinbox },
	{ "NewTab",                 l_NewTab },
#endif


		{ NULL }
};

int libuilua_from_control(lua_State *L, uiControl *c) {
	create_object(L, "Button", uiControl(c));
	return 0;
}

int luaopen_libuilua(lua_State *L)
{
	create_meta(L, "Box", meta_Box);
	create_meta(L, "Button", meta_Button);
	create_meta(L, "Group", meta_Group);
	create_meta(L, "Label", meta_Label);
	create_meta(L, "ProgressBar", meta_ProgressBar);
	create_meta(L, "Separator", meta_Separator);
	create_meta(L, "Tab", meta_Tab);
	create_meta(L, "Window", meta_Window);
	luaL_newlib(L, lui_table);
	return 1;
}

/*
 * End
 */
