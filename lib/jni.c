// JNI PTP/IP interface for camlib and fuji.c
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>

#include "myjni.h"
#include "backend.h"
#include "fuji.h"
#include "fujiptp.h"

JNI_FUNC(jint, cPtpFujiInit)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	// Take this as an opportunity to reset all the structures for a new connection
	reset_connection();

	int rc = ptpip_fuji_init(&backend.r, "fujiapp");

	struct PtpFujiInitResp resp;
	ptp_fuji_get_init_info(&backend.r, &resp);

	fuji_known.info = fuji_get_model_info(resp.cam_name);

	return rc;
}

JNI_FUNC(jstring, cPtpRun)(JNIEnv *env, jobject thiz, jstring string) {
	backend.env = env;
	const char *req = (*env)->GetStringUTFChars(env, string, 0);

	char *buffer = malloc(PTP_BIND_DEFAULT_SIZE);

	int r = bind_run(&backend.r, (char *)req, buffer, PTP_BIND_DEFAULT_SIZE);

	if (r == -1) {
		return (*env)->NewStringUTF(env, "{\"error\": -1}");
	}

	jstring ret = (*env)->NewStringUTF(env, buffer);
	free(buffer);
	return ret;
}

JNI_FUNC(jint, cPtpFujiWaitUnlocked)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return fuji_wait_for_access(&backend.r);
}

JNI_FUNC(jint, cPtpFujiPing)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return fuji_get_events(&backend.r);
}

JNI_FUNC(jbyteArray, cPtpGetThumb)(JNIEnv *env, jobject thiz, jint handle) {
	backend.env = env;
	void *data = NULL;
	int length = 0;
	int rc = ptp_get_thumbnail_smart_cache(&backend.r, handle, &data, &length);
	if (rc == PTP_CHECK_CODE || length == 0 || data == NULL) {
		// If an error code is returned - allow it to fall
		// through and return a zero-length array
		length = 0;
		data = ptp_get_payload(&backend.r); // is this necessary? hmm
	} else if (rc) {
		return NULL;
	}

	jbyteArray ret = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, ret, 0, length, (const jbyte *)(data));
	return ret;
}

JNI_FUNC(jint, cPtpGetPropValue)(JNIEnv *env, jobject thiz, jint code) {
	backend.env = env;

	int rc = ptp_get_prop_value(&backend.r, code);
	if (rc < 0) {
		return rc;
	}

	return ptp_parse_prop_value(&backend.r);
}

JNI_FUNC(jbyteArray, cFujiGetFile)(JNIEnv *env, jobject thiz, jint handle) {
	backend.env = env;

	// Set the compression prop (allows full images to go through, otherwise puts
	// extra data in ObjectInfo and cuts off image downloads)
	int rc = ptp_set_prop_value(&backend.r, PTP_PC_FUJI_NoCompression, 1);
	if (rc) {
		return NULL;
	}

	int max = backend.r.data_length;

	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(&backend.r, (int)handle, &oi);
	if (rc) {
		return NULL;
	}

	android_err("Compressed Size: %d", oi.compressed_size);

	jbyteArray ret = (*env)->NewByteArray(env, oi.compressed_size);

	// Makes sure to set the compression prop back to 0 after finished
	// (extra data won't go through for some reason)
	int read = 0;
	while (1) {
		rc = ptp_get_partial_object(&backend.r, handle, read, max);
		if (rc == PTP_CHECK_CODE) {
			ptp_set_prop_value(&backend.r, PTP_PC_FUJI_NoCompression, 0);
			return NULL;
		} else if (rc) {
			return NULL;
		}

		if (ptp_get_payload_length(&backend.r) == 0) {
			ptp_set_prop_value(&backend.r, PTP_PC_FUJI_NoCompression, 0);
			return NULL;
		} else if (rc) {
			return NULL;
		}

		(*env)->SetByteArrayRegion(env, ret, read, ptp_get_payload_length(&backend.r), (const jbyte *)(ptp_get_payload(&backend.r)));

		read += ptp_get_payload_length(&backend.r);

		if (read >= oi.compressed_size) {
			ptp_set_prop_value(&backend.r, PTP_PC_FUJI_NoCompression, 0);
			return ret;
		}
	}
}

// NOTE: cFujiConfigInitMode *must* be called before cFujiConfigVersion, or anything else.
// If not, it will break up the connection and destroy packets for any file operation.
JNI_FUNC(jint, cFujiConfigInitMode)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	int rc = fuji_config_init_mode(&backend.r);

	return rc;
}

JNI_FUNC(jint, cFujiConfigVersion)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	int rc = fuji_config_version(&backend.r);
	if (rc) return rc;

	rc = fuji_config_device_info_routine(&backend.r);

	rc = fuji_config_image_viewer(&backend.r);
	if (rc) return rc;

	return 0;
}

JNI_FUNC(jboolean, cIsUntestedMode)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	if (fuji_known.info == NULL) {
		return 1;
	}

	if (fuji_known.image_explore_version != 2) {
		return 1;
	}

	return 0;
}

JNI_FUNC(jboolean, cIsMultipleMode)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	return fuji_known.camera_state == FUJI_MULTIPLE_TRANSFER;
}

JNI_FUNC(jint, cTestStuff)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	int rc = ptpip_fuji_get_events(&backend.r);

	return rc;
}

// TODO: finish
JNI_FUNC(jintArray, cGetObjectHandles)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	// By this point num_objects should be known - by get_events and init_mode
	if (fuji_known.num_objects == 0 || fuji_known.num_objects == -1) {
		return NULL;
	}

	// Object #0 seems to always be DCIM or invalid (can't get thumbnail) - is this standard?
	int *list = malloc(sizeof(int) * fuji_known.num_objects);
	for (int i = 0; i < fuji_known.num_objects; i++) {
		list[i] = i + 1;
	}

	jintArray result = (*env)->NewIntArray(env, fuji_known.num_objects);
	(*env)->SetIntArrayRegion(env, result, 0, fuji_known.num_objects, list);
	free(list);

	return result;
}
