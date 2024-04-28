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

#ifdef ANDROID
#include "backend.h"
int fuji_download_multiple(struct PtpRuntime *r);

JNI_FUNC(jint, cFujiDownloadMultiple)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	return fuji_download_multiple(&backend.r);
}
#endif

int fuji_download_classic(struct PtpRuntime *r) {
	while (1) {
		// This determines whether the connection is terminated or not
		struct PtpObjectInfo oi;
		int rc = ptp_get_object_info(r, 1, &oi);
		if (rc) return rc;

		app_print("Downloading %s...", oi.filename);

		char path[128];
		sprintf(path, "/storage/emulated/0/Pictures/fudge/%s", oi.filename);
		FILE *f = fopen(path, "wb");
		if (f == NULL) return PTP_RUNTIME_ERR;

		// Not sure if 0x100000 is required or not, but we'll do what Fuji is doing.
		rc = ptp_download_object(r, 1, f, 0x100000);
		fclose(f);
		if (rc) {
			app_print("Failed to save %s: %s", oi.filename, ptp_perror(rc));
			return rc;
		}

		// Fuji's fujisystem will swap out object ID 1 with the next image. If there
		// are no more images, the camera shuts down the connection and turns off.

		// In other words, the camera is in superposition - it's on and off at the same time.
		// We don't know until we observe it:
		rc = fuji_get_events(r);
		if (rc) return 0;
	}
}

int fuji_download_multiple(struct PtpRuntime *r) {
	if (fuji_download_classic(r)) {
		app_print("Error importing images");
		return PTP_IO_ERR;
	} else {
		app_print("Done downloading images.");
	}
	return 0;
}
