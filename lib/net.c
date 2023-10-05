#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <camlib.h>

#include "myjni.h"
#include "fuji.h"
#include "backend.h"

JNI_FUNC(jboolean, cIsUsingEmulator)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return 0;
}

int ptpip_cmd_write(struct PtpRuntime *r, void *to, int length) {
	jbyteArray data = (*backend.env)->NewByteArray(backend.env, length);
	(*backend.env)->SetByteArrayRegion(backend.env, data, 0, length, (const jbyte *)(to));

	int ret = (*backend.env)->CallIntMethod(backend.env, backend.conn, backend.cmd_write, data);
	if (ret < 0) {
		android_err("cmd_write failed: %d", ret);
		return -1;
	}

	return ret;
}

int ptpip_cmd_read(struct PtpRuntime *r, void *to, int length) {
	if (length <= 0) {
		android_err("Length is less than 1");
		return -1;
	}

	// We will NOT be reading 50mb in a single packet
	if (length > 50000000) {
		android_err("Camera is trying to send too much data - breaking connection.");
		return -1;
	}

	jbyteArray data = (*backend.env)->NewByteArray(backend.env, length);

	int ret = (*backend.env)->CallIntMethod(backend.env, backend.conn, backend.cmd_read, data, length);
	if (ret < 0) {
		android_err("failed to receive packet, rc=%d (length=%d)", ret, length);
		return -1;
	}

	jbyte *bytes = (*backend.env)->GetByteArrayElements(backend.env, data, 0);
	memcpy(to, bytes, ret);

	(*backend.env)->DeleteLocalRef(backend.env, data);
	return ret;
}

int ptpip_event_send(struct PtpRuntime *r, void *data, int size) {
	return -1;
}

int ptpip_event_read(struct PtpRuntime *r, void *data, int size) {
	return -1;
}
