// Scripts UI layouts
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "myjni.h"
#include "ui.h"
#include "ui_android.h"

static void on_click(uiButton *b, void *dat) {
	uiToast("Hello, World");
}

void fudge_scripts_screen() {
	uiBox *box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);

	uiButton *b = uiNewButton("Hello, World");
	uiButtonOnClicked(b, on_click, NULL);
	uiBoxAppend(box, uiControl(b), 0);

	uiBoxAppend(box, uiControl(uiNewLabel("With scripts, you can make your camera do anything.")), 0);

	uiSwitchScreen(box, "Scripts");
}

#ifdef ANDROID
JNI_FUNC(jint, cFujiScriptsScreen)(JNIEnv *env, jobject thiz, jobject ctx) {
	uiAndroidInit(env, ctx);

	fudge_scripts_screen();

	return 0;
}
#endif
