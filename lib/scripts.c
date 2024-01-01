#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "myjni.h"
#include "ui.h"

static void on_click(uiButton *b, void *dat) {
	uiToast("Hello, World");
}

JNI_FUNC(jint, cFujiScriptsScreen)(JNIEnv *env, jobject thiz, jobject ctx) {
	uiAndroidInit(env, ctx);

	uiBox *box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);

	uiButton *b = uiNewButton("Hello, World");
	uiButtonOnClicked(b, on_click, NULL);
	uiBoxAppend(box, b, 0);

	uiBoxAppend(box, uiNewLabel("With scripts, you can make your camera do anything."), 0);

	uiSwitchScreen(box, "Scripts");

	return 0;
}
