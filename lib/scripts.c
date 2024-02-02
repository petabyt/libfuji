// Scripts UI layouts
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ui.h>
#include <uifw.h>
#include "myjni.h"

#include <camlib.h>
#include <camlua.h>

int luaopen_libuilua(lua_State *L);

int cam_lua_setup(lua_State *L) {
	luaL_requiref(L, "ui", luaopen_libuilua, 1);
}

static void on_click(uiButton *b, void *dat) {
	char *file = libu_get_txt_file(uiAndroidGetEnv(), uiAndroidGetCtx(), "script.lua");

	if (cam_run_lua_script_async(file) < 0) {
		uiToast(cam_lua_get_error());
	}

	free(file);
}

void *libu_get_assets_file(JNIEnv *env, jobject ctx, char *filename, int *length);

void fudge_scripts_screen() {
	uiBox *box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);

	uiButton *b = uiNewButton("Hello, World");
	uiButtonOnClicked(b, on_click, NULL);
	uiBoxAppend(box, uiControl(b), 0);

	uiBoxAppend(box, uiControl(uiNewLabel("Lua plugins coming soon")), 0);

	uiBoxAppend(box, uiControl(uiNewLabel("Planned features:\n"
						"- Scripts can have a custom UI (just like this UI)\n"
						"- Automate things and workflows\n"
						"- Bleeding edge experimental features!\n"
						"- Buffer overflows!")), 0);

	uiSwitchScreen(box, "Scripts");
}

JNI_FUNC(jint, cFujiScriptsScreen)(JNIEnv *env, jobject thiz, jobject ctx) {
	uiAndroidInit(env, ctx);

	fudge_scripts_screen();

	return 0;
}