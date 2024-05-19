
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

#define RETURN_SELF lua_pushvalue(L, 1); return 1;

#define CREATE_META(n) \
	luaL_newmetatable(L, "libui." #n);  \
	luaL_setfuncs(L, meta_ ## n, 0);

#define CREATE_OBJECT(t, c) \
	struct wrap *w = lua_newuserdata(L, sizeof(struct wrap)); \
	w->control = uiControl(c); \
	lua_newtable(L); \
	luaL_getmetatable(L, "libui." #t); \
	lua_setfield(L, -2, "__index"); \
	lua_pushcfunction(L, l_gc); \
	lua_setfield(L, -2, "__gc"); \
	lua_setmetatable(L, -2);

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
	//lua_pushinteger(L, 1234);
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
	//luaL_checktype(L, -1, LUA_TUSERDATA);

	lua_pushstring(L, "data");
	lua_gettable(L, -4);
	//luaL_checktype(L, -1, LUA_TUSERDATA);

	/* Call function */

	lua_call(L, 2, 0);

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


/*
 * Area
 */

int l_NewArea(lua_State *L)
{
	static struct uiAreaHandler ah;
	CREATE_OBJECT(Area, uiNewArea(&ah));
	return 1;
}

int l_AreaSetSize(lua_State *L)
{
	uiAreaSetSize(CAST_ARG(1, Area), luaL_checknumber(L, 2), luaL_checknumber(L, 3));
	RETURN_SELF;
}

static struct luaL_Reg meta_Area[] = {
	{ "SetSize",              l_AreaSetSize },
	{ NULL }
};


/*
 * Box
 */

int l_NewVerticalBox(lua_State *L)
{
	CREATE_OBJECT(Box, uiNewVerticalBox());
	return 1;
}

int l_NewHorizontalBox(lua_State *L)
{
	CREATE_OBJECT(Box, uiNewHorizontalBox());
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
	RETURN_SELF;
}

int l_BoxPadded(lua_State *L)
{
	lua_pushnumber(L, uiBoxPadded(CAST_ARG(1, Box)));
	return 1;
}

int l_BoxSetPadded(lua_State *L)
{
	uiBoxSetPadded(CAST_ARG(1, Box), luaL_checknumber(L, 2));
	RETURN_SELF;
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
	CREATE_OBJECT(Button, uiNewButton(
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
	RETURN_SELF;
}

int l_ButtonOnClicked(lua_State *L)
{
	uiButtonOnClicked(CAST_ARG(1, Button), on_button_clicked, L);
	create_callback_data(L, 1);
	RETURN_SELF;
}

static struct luaL_Reg meta_Button[] = {
	{ "SetText",              l_ButtonSetText },
	{ "OnClicked",            l_ButtonOnClicked },
	{ NULL }
};


/*
 * Checkbox
 */

int l_NewCheckbox(lua_State *L)
{
	CREATE_OBJECT(Checkbox, uiNewCheckbox(
		luaL_checkstring(L, 1)
	));
	return 1;
}

static void on_checkbox_toggled(uiCheckbox *c, void *data)
{
	callback(data, c);
}

int l_CheckboxSetText(lua_State *L)
{
	uiCheckboxSetText(CAST_ARG(1, Checkbox), luaL_checkstring(L, 2));
	RETURN_SELF;
}

int l_CheckboxOnToggled(lua_State *L)
{
        uiCheckboxOnToggled(CAST_ARG(1, Checkbox), on_checkbox_toggled, L);
        create_callback_data(L, 1);
        RETURN_SELF;
}

static struct luaL_Reg meta_Checkbox[] = {
	{ "SetText",              l_CheckboxSetText },
	{ "OnToggled",            l_CheckboxOnToggled },
	{ NULL }
};


/*
 * Combobox
 */

int l_NewCombobox(lua_State *L)
{
	CREATE_OBJECT(Combobox, uiNewCombobox());
	return 1;
}

int l_NewEditableCombobox(lua_State *L)
{
	CREATE_OBJECT(Combobox, uiNewEditableCombobox());
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
	RETURN_SELF;
}

int l_ComboboxOnToggled(lua_State *L)
{
        uiComboboxOnSelected(CAST_ARG(1, Combobox), on_combobox_selected, L);
        create_callback_data(L, 1);
        RETURN_SELF;
}

static struct luaL_Reg meta_Combobox[] = {
	{ "Append",               l_ComboboxAppend },
	{ "OnToggled",            l_ComboboxOnToggled },
	{ NULL }
};


/*
 * Control
 */

int l_ControlShow(lua_State *L)
{
	uiControlShow(CAST_ARG(1, Control));
	RETURN_SELF;
}


int l_ControlDestroy(lua_State *L)
{
	printf("destroy not implemented, garbage collection needs to be implemented\n");
	uiControlDestroy(CAST_ARG(1, Control));
	return 0;
}



/*
 * Date/Timepicker
 */

int l_NewDateTimePicker(lua_State *L)
{
	CREATE_OBJECT(DateTimePicker, uiNewDateTimePicker());
	return 1;
}

int l_NewDatePicker(lua_State *L)
{
	CREATE_OBJECT(DateTimePicker, uiNewDatePicker());
	return 1;
}

int l_NewTimePicker(lua_State *L)
{
	CREATE_OBJECT(DateTimePicker, uiNewTimePicker());
	return 1;
}

static struct luaL_Reg meta_DateTimePicker[] = {
	{ NULL }
};




/*
 * Group
 */

int l_NewGroup(lua_State *L)
{
	CREATE_OBJECT(Group, uiNewGroup(
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
	RETURN_SELF;
}

int l_GroupSetChild(lua_State *L)
{
	uiGroupSetChild(CAST_ARG(1, Group), CAST_ARG(2, Control));
	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_pushboolean(L, 1);
	lua_settable(L, -3);
	RETURN_SELF;
}

int l_GroupMargined(lua_State *L)
{
	lua_pushnumber(L, uiGroupMargined(CAST_ARG(1, Group)));
	return 1;
}

int l_GroupSetMargined(lua_State *L)
{
	uiGroupSetMargined(CAST_ARG(1, Group), luaL_checknumber(L, 2));
	RETURN_SELF;
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
	CREATE_OBJECT(Label, uiNewLabel(
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
	RETURN_SELF;
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
	CREATE_OBJECT(ProgressBar, uiNewProgressBar());
	return 1;
}

int l_ProgressBarSetValue(lua_State *L)
{
	double value = luaL_checknumber(L, 2);
	uiProgressBarSetValue(CAST_ARG(1, ProgressBar), value);
	RETURN_SELF;
}

static struct luaL_Reg meta_ProgressBar[] = {
	{ "SetValue",             l_ProgressBarSetValue },
	{ NULL }
};


/*
 * RadioButtons
 */

int l_NewRadioButtons(lua_State *L)
{
	CREATE_OBJECT(RadioButtons, uiNewRadioButtons());
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
	RETURN_SELF;
}


static struct luaL_Reg meta_RadioButtons[] = {
	{ "Append",               l_RadioButtonsAppend },
	{ NULL }
};




/*
 * Separator
 */

int l_NewHorizontalSeparator(lua_State *L)
{
	CREATE_OBJECT(Separator, uiNewHorizontalSeparator());
	return 1;
}

static struct luaL_Reg meta_Separator[] = {
	{ NULL }
};


/*
 * Slider
 */

int l_NewSlider(lua_State *L)
{
	CREATE_OBJECT(Slider, uiNewSlider(
		luaL_checknumber(L, 1),
		luaL_checknumber(L, 2)
	));
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
	RETURN_SELF;
}

static void on_slider_changed(uiSlider *b, void *data)
{
	callback(data, b);
}

int l_SliderOnChanged(lua_State *L)
{
	uiSliderOnChanged(CAST_ARG(1, Slider), on_slider_changed, L);
	create_callback_data(L, 1);
	RETURN_SELF;
}

static struct luaL_Reg meta_Slider[] = {
	{ "Value",                l_SliderValue },
	{ "SetValue",             l_SliderSetValue },
	{ "OnChanged",            l_SliderOnChanged },
	{ NULL }
};


/*
 * Spinbox
 */

int l_NewSpinbox(lua_State *L)
{
	CREATE_OBJECT(Spinbox, uiNewSpinbox(
		luaL_checknumber(L, 1),
		luaL_checknumber(L, 2)
	));
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
	RETURN_SELF;
}

static void on_spinbox_changed(uiSpinbox *b, void *data)
{
	callback(data, b);
}

int l_SpinboxOnChanged(lua_State *L)
{
	uiSpinboxOnChanged(CAST_ARG(1, Spinbox), on_spinbox_changed, L);
	create_callback_data(L, 1);
	RETURN_SELF;
}

static struct luaL_Reg meta_Spinbox[] = {
	{ "Value",                l_SpinboxValue },
	{ "SetValue",             l_SpinboxSetValue },
	{ "OnChanged",            l_SpinboxOnChanged },
	{ NULL }
};


/*
 * Tab
 */

int l_NewTab(lua_State *L)
{
	CREATE_OBJECT(Tab, uiNewTab());
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
	RETURN_SELF;
}

static struct luaL_Reg meta_Tab[] = {
	{ "Append",               l_TabAppend },
	{ NULL }
};





/*
 * Window
 */

int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

static int onShouldQuit(void *data)
{
	uiWindow *mainwin = uiWindow(data);
	uiControlDestroy(uiControl(mainwin));
	return 0;
}

int l_NewWindow(lua_State *L)
{
	CREATE_OBJECT(Window, uiNewWindow(
		luaL_checkstring(L, 1),
		luaL_checknumber(L, 2),
		luaL_checknumber(L, 3),
		lua_toboolean(L, 4)
	));

	uiWindowOnClosing((uiWindow *)w->control, onShouldQuit, w->control);
	uiWindowSetMargined((uiWindow *)w->control, 1);
	
	return 1;
}

int l_WindowSetChild(lua_State *L)
{
	uiWindowSetChild(CAST_ARG(1, Window), CAST_ARG(2, Control));
	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_pushboolean(L, 1);
	lua_settable(L, -3);
	RETURN_SELF;
}

static int on_window_closing(uiWindow *w, void *data)
{
	callback(data, w);
	return 0;
}

int l_WindowOnClosing(lua_State *L)
{
	uiWindowOnClosing(CAST_ARG(1, Window), on_window_closing, L);
	create_callback_data(L, 1);
	RETURN_SELF;
}

int l_WindowSetMargined(lua_State *L)
{
	uiWindowSetMargined(CAST_ARG(1, Window), luaL_checknumber(L, 2));
	RETURN_SELF;
}

int l_MsgBox(lua_State *L)
{
	uiMsgBox(CAST_ARG(1, Window), luaL_checkstring(L, 2), luaL_checkstring(L, 3));
	RETURN_SELF;
}

static struct luaL_Reg meta_Window[] = {
	{ "OnClosing",           l_WindowOnClosing },
	{ "SetChild",            l_WindowSetChild },
	{ "SetMargined",         l_WindowSetMargined },
	{ "Show",                l_ControlShow },
	{ "Destroy",             l_ControlDestroy },
	{ "MsgBox",              l_MsgBox},
	{ NULL }
};



/*
 * Various top level
 */

int l_Init(lua_State *L)
{
	uiInitOptions o;

	memset(&o, 0, sizeof (uiInitOptions));

	const char *err = uiInit(&o);

	lua_pushstring(L, err);
	return 1;
}

int l_Uninit(lua_State *L)
{
	uiUninit();
	return 0;
}

int l_Main(lua_State *L)
{
	uiMain();
	return 0;
}

int l_MainStep(lua_State *L)
{
	int r = uiMainStep(lua_toboolean(L, 1));
	lua_pushnumber(L, r);
	return 1;
}

int l_Quit(lua_State *L)
{
	uiQuit();
	return 0;
}

static struct luaL_Reg lui_table[] = {

	{ "Init",                   l_Init },
	{ "Uninit",                 l_Uninit },
	{ "Main",                   l_Main },
	{ "MainStep",               l_MainStep },
	{ "Quit",                   l_Quit },

	{ "NewArea",                l_NewArea },
	{ "NewButton",              l_NewButton },
	{ "NewCheckbox",            l_NewCheckbox },
	{ "NewCombobox",            l_NewCombobox },
	{ "NewDateTimePicker",      l_NewDateTimePicker },
	{ "NewDatePicker",          l_NewDatePicker },
	{ "NewTimePicker",          l_NewTimePicker },
	{ "NewEditableCombobox",    l_NewEditableCombobox },
	{ "NewGroup",               l_NewGroup },
	{ "NewHorizontalBox",       l_NewHorizontalBox },
	{ "NewHorizontalSeparator", l_NewHorizontalSeparator },
	{ "NewLabel",               l_NewLabel },
	{ "NewProgressBar",         l_NewProgressBar },
	{ "NewRadioButtons",        l_NewRadioButtons },
	{ "NewSlider",              l_NewSlider },
	{ "NewSpinbox",             l_NewSpinbox },
	{ "NewTab",                 l_NewTab },
	{ "NewVerticalBox",         l_NewVerticalBox },
	{ "NewWindow",              l_NewWindow },


	{ NULL }
};


int luaopen_libuilua(lua_State *L)
{


	CREATE_META(Area)
	CREATE_META(Box)
	CREATE_META(Button)
	CREATE_META(Checkbox)
	CREATE_META(Combobox)
	CREATE_META(DateTimePicker)
	CREATE_META(Group)
	CREATE_META(Label)
	CREATE_META(ProgressBar)
	CREATE_META(RadioButtons)
	CREATE_META(Separator)
	CREATE_META(Slider)
	CREATE_META(Spinbox)
	CREATE_META(Tab)
	CREATE_META(Window)
	luaL_newlib(L, lui_table);
	return 1;
}

/*
 * End
 */
