// Implementation for Fudge image viewer / downloader
// Copyright 2024 (c) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "app.h"
#include <camlib.h>
#include "fuji.h"
#include "fujiptp.h"

#include "myjni.h"
#include "backend.h"

int fuji_download_multiple(struct PtpRuntime *r);

JNI_FUNC(jint, cFujiDownloadMultiple)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	return fuji_download_multiple(&backend.r);
}

static int download(char *name, void *data, size_t length) {
	char path[128];
	sprintf(path, "/storage/emulated/0/DCIM/fuji/%s", name);

	FILE *f = fopen(path, "wb");
	if (f == NULL) return 1;
	fwrite(data, 1, length, f);
	fclose(f);
	return 0;
}

int fuji_download_classic(struct PtpRuntime *r) {
	while (1) {
		// This determines whether the connection is terminated or not
		struct PtpObjectInfo oi;
		int rc = ptp_get_object_info(r, 1, &oi);
		if (rc) {
			return NULL;
		}

		app_print("Downloading %s...", oi.filename);

		// Give a generous 2mb buffer - allow the downloader to extend as needed
		size_t size = 2 * 1000 * 1000;
		uint8_t *buffer = malloc(size);

		int dsize = fuji_slow_download_object(r, 1, &buffer, size);
		if (dsize < 0) return dsize;

		if (download(oi.filename, buffer, dsize)) {
			app_print("Failed to save %s", oi.filename);
		}

		free(buffer);

		// Fuji's fujisystem will swap out object ID 1 with the next image. If there
		// are no more images, the camera shuts down the connection and turns off.

		// In other words, the camera is in superposition - it's on and off at the same time.
		// We don't know until we observe it:
		rc = fuji_get_events(r);
		if (rc) return 0;
	}
}

int fuji_download_multiple(struct PtpRuntime *r) {
	if (fuji_known.remote_version == -1) {
		if (fuji_download_classic(r)) {
			app_print("Error importing images");
		} else {
			app_print("Done downloading images.");
		}
	} else {
		app_print("Unsupported remote mode download");
	}
	return 0;
}
