// Implementation for Fudge image viewer / downloader
// Copyright 2024 (c) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <camlib.h>
#include "app.h"

#include "fuji.h"
#include "fujiptp.h"

#include "myjni.h"
#include "backend.h"

int fuji_download_multiple(struct PtpRuntime *r);

JNI_FUNC(jint, cFujiDownloadMultiple)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return fuji_download_multiple(&backend.r);
}

int fuji_download_classic(struct PtpRuntime *r) {
	while (1) {	
		struct PtpObjectInfo oi;
		int rc = ptp_get_object_info(r, 1, &oi);
		if (rc) {
			return NULL;
		}

		int size = 2 * 1000 * 1000;
		uint8_t *buffer = malloc(size);
				
	}
}

int fuji_download_multiple(struct PtpRuntime *r) {
	if (fuji_known.remote_version != -1) {
		return fuji_download_classic(r);
	}
	return 0;
}
