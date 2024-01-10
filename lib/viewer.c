// Implementation for Fudge image viewer / downloader
// Copyright 2024 (c) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <camlib.h>

#ifdef ANDROID
#include "myjni.h"

JNI_FUNC(jint, cFujiDownloadMultiple)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return fuji_download_multiple(&backend.r);
}
#endif

int fuji_download_multiple(struct PtpRuntime *r) {
	return 0;
}
