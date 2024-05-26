// Native rendering tests

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include <ui.h>
#include "app.h"
#include "backend.h"
#include "fuji.h"

#ifndef JNI_LV
#define JNI_LV(ret, name) JNIEXPORT ret JNICALL Java_dev_danielc_fujiapp_Liveview_##name
#endif

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <inttypes.h>
static ANativeWindow *window = 0;

JNI_LV(void, cUpdateSurface)(JNIEnv *env, jobject thiz, jobject surface) {
	if (surface == 0) abort();
	window = ANativeWindow_fromSurface(env, surface);
	ANativeWindow_acquire(window);

	ANativeWindow_setBuffersGeometry(window, 0, 0,  AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);

	ANativeWindow_Buffer winbuf;
	ARect x;
	if (ANativeWindow_lock(window, &winbuf, &x)) {
		abort();
	}

	plat_dbg("Dimension: %dx%dx%d", winbuf.width, winbuf.height, winbuf.stride);
	plat_dbg("Prop: %X", winbuf.format);

#define RGBA888(x) ((x & 0xFF) << 16) | (x & 0xFF00) | ((x & 0xFF0000) >> 8)

	for (int x = 0; x < 400; x++) {
		for (int y = 0; y < 400; y++) {
			((uint32_t *)winbuf.bits)[(x + 100) + ((y + 500) * winbuf.stride)] = RGBA888(0x0eb4c9);
		}
	}

	jmethodID render_text_m = (*env)->GetStaticMethodID(env, (*env)->FindClass(env, "dev/danielc/fujiapp/Liveview"), "renderText", "(Ljava/lang/String;II)[B");
	jbyteArray text = (*env)->CallStaticObjectMethod(env, (*env)->FindClass(env, "dev/danielc/fujiapp/Liveview"), render_text_m, (*env)->NewStringUTF(env, "Hello, World"), 0, 0xffffff);

	uint32_t *data = (uint32_t *)(*env)->GetByteArrayElements(env, text, 0);

	for (int x = 0; x < 256; x++) {
		for (int y = 0; y < 256; y++) {
			((uint32_t *)winbuf.bits)[(x + 100) + ((y + 500) * winbuf.stride)] = (data[x + (y * 256)]);
		}
	}

	if (ANativeWindow_unlockAndPost(window)) {
		abort();
	}
}
