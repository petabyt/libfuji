// Progress bar / IO monitoring backend
// Copyright 2024 by Daniel C (https://github.com/petabyt/camlib)
#include <camlib.h>
#include <time.h>
#include <dlfcn.h>
#include "app.h"
#include "backend.h"

static int last_p = 0;

void app_increment_progress_bar(int read) {
	if (backend.progress_bar == NULL) {
		return;
	}

	backend.download_progress += read;

	int n = (((double)backend.download_progress) / (double)backend.download_size * 100.0);
	if (last_p != n) {
		if (n > 100) return;

		JNIEnv *env = get_jni_env();

		jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, backend.progress_bar), "setProgress", "(I)V");
		(*env)->CallVoidMethod(env, backend.progress_bar, method, n);
	}
	last_p = n;
}

JNI_FUNC(jboolean, cSetProgressBarObj)(JNIEnv *env, jobject thiz, jobject pg, jint size) {
	static clock_t tm;
	if (pg == NULL) {
		plat_dbg("Time taken to download: %f", (double)(clock() - tm) / CLOCKS_PER_SEC);
		(*env)->DeleteGlobalRef(env, backend.progress_bar);
		backend.progress_bar = NULL;
		return 0;
	}
	tm = clock();
	last_p = 0;
	backend.download_size = size;
	backend.download_progress = 0;
	backend.progress_bar = (*env)->NewGlobalRef(env, pg);
	return 0;
}
