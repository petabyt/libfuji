// JNI PTP/IP interface for camlib and fuji.c
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include <ui.h>
#include "app.h"
#include "backend.h"
#include "fuji.h"
#include "fujiptp.h"

JNI_FUNC(void, cReportError)(JNIEnv *env, jobject thiz, jint code, jstring reason) {
	set_jni_env(env);

	const char *c_reason = (*env)->GetStringUTFChars(env, reason, 0);

	ptp_report_error(&backend.r, (char *)c_reason, (int)code);

	if (backend.r.connection_type == PTP_USB) {
		ptp_device_close(&backend.r);
	} else {
		ptpip_close(&backend.r);
	}

	(*env)->ReleaseStringUTFChars(env, reason, c_reason);
	(*env)->DeleteLocalRef(env, reason);
}

JNI_FUNC(void, cClearKillSwitch)(JNIEnv *env, jobject thiz) {
	backend.r.io_kill_switch = 0;
}

JNI_FUNC(jboolean, cGetKillSwitch)(JNIEnv *env, jobject thiz) {
	return backend.r.io_kill_switch != 0;
}

JNI_FUNC(jint, cPtpFujiPing)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	if (backend.r.connection_type == PTP_USB) {
		return 0;
	}
	int rc = fuji_get_events(&backend.r);
	return rc;
}

JNI_FUNC(jint, cFujiDownloadFile)(JNIEnv *env, jobject thiz, jint handle, jstring path) {
	set_jni_env(env);

	const char *c_path = (*env)->GetStringUTFChars(env, path, 0);

	FILE *f = fopen(c_path, "wb");
	if (f == NULL) return PTP_RUNTIME_ERR;

	int rc = ptp_download_object(&backend.r, handle, f, 0x1000000);
	fclose(f);
	if (rc) {
		app_print("Failed to save %s: %s", c_path, ptp_perror(rc));
		return rc;
	}

	(*env)->ReleaseStringUTFChars(env, path, c_path);
	(*env)->DeleteLocalRef(env, path);

	return 0;
}

// Must be called after cFujiGetUncompressedObjectInfo
JNI_FUNC(jint, cFujiGetFile)(JNIEnv *env, jobject thiz, jint handle, jbyteArray array, jint file_size) {
	set_jni_env(env);

	// This can be any number really, but best to keep under 20mb or so
	// We try and get it as huge as possible to speed things up
	int max = backend.r.data_length;

	// Makes sure to set the compression prop back to 0 after finished
	// (extra data won't go through for some reason)
	int read = 0;
	while (1) {
		ptp_mutex_keep_locked(&backend.r);

		int rc = ptp_get_partial_object(&backend.r, handle, read, max);
		if (rc == PTP_CHECK_CODE) {
			if (backend.r.connection_type == PTP_IP_USB) fuji_disable_compression(&backend.r);
			ptp_mutex_unlock(&backend.r);
			return rc;
		} else if (rc) {
			plat_dbg("Download fail %d", rc);
			ptp_mutex_unlock(&backend.r);
			return rc;
		}

		size_t payload_size = ptp_get_payload_length(&backend.r);

		if (payload_size == 0) {
			//fuji_disable_compression(&backend.r);
			ptp_mutex_unlock(&backend.r);
			return rc;
		}

		if (read + payload_size > file_size) {
			plat_dbg("ptp_get_object_info has lied about it's compressed size out of shame");
		}

		(*env)->SetByteArrayRegion(
			env, array,
			read, payload_size,
			(const jbyte *)(ptp_get_payload(&backend.r))
		);

		ptp_mutex_unlock(&backend.r);

		// Check for possible buffer overflow
		if ((*env)->ExceptionCheck(env)) {
			plat_dbg("SetByteArrayRegion exception");
			(*env)->ExceptionClear(env);

			//fuji_disable_compression(&backend.r);

			return rc;
		}

		read += ptp_get_payload_length(&backend.r);

		if (read >= file_size) {
			if (backend.r.connection_type == PTP_IP_USB) fuji_disable_compression(&backend.r);
			plat_dbg("Downloaded %d bytes", read);
			return 0;
		}
	}
}

// Must be called *before* a call to cFujiGetFile
JNI_FUNC(jstring, cFujiGetUncompressedObjectInfo)(JNIEnv *env, jobject thiz, jint handle) {
	set_jni_env(env);

	int rc;
	if (backend.r.connection_type == PTP_IP_USB) {
		rc = fuji_enable_compression(&backend.r);
		if (rc) {
			return NULL;
		}
	}

	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(&backend.r, (int)handle, &oi);
	if (rc) {
		return NULL;
	}

	// Compression must be disabled in cFujiGetFile or somewhere else

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));

	jstring ret = (*env)->NewStringUTF(env, buffer);
	return ret;
}

JNI_FUNC(jint, cFujiSetup)(JNIEnv *env, jobject thiz, jstring ip) {
	set_jni_env(env);

	if (backend.r.connection_type == PTP_USB) {
		return fujiusb_setup(&backend.r);
	} else {
		const char *c_ip = (*env)->GetStringUTFChars(env, ip, 0);

		int rc = fuji_setup(&backend.r, (char *)c_ip);

		(*env)->ReleaseStringUTFChars(env, ip, c_ip);
		(*env)->DeleteLocalRef(env, ip);

		return rc;
	}
}

JNI_FUNC(jint, cFujiConfigImageGallery)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);

	if (backend.r.connection_type == PTP_USB) return 0;

	int rc = fuji_config_image_viewer(&backend.r);
	if (rc) return rc;

	return 0;
}

jintArray ptpusb_get_object_handles(JNIEnv *env, struct PtpRuntime *r) {
	struct PtpArray *arr;
	int rc = ptp_get_storage_ids(r, &arr);
	if (rc) return NULL;

	if (arr->length == 0) {
		jintArray result = (*env)->NewIntArray(env, 0);
		return result;
	}

	int id = arr->data[0];

	free(arr);

	// Get all objects in root
	rc = ptp_get_object_handles(r, id, 0, 0, &arr);
	if (rc) return NULL;

	jintArray result = (*env)->NewIntArray(env, arr->length);
	(*env)->SetIntArrayRegion(env, result, 0, arr->length, (const int *)arr->data);

	free(arr);

	return result;
}

// Return array of valid objects on main storage device
JNI_FUNC(jintArray, cGetObjectHandles)(JNIEnv *env, jobject thiz) {
	if (backend.r.connection_type == PTP_USB) {
		return ptpusb_get_object_handles(env, &backend.r);
	} else {
		// By this point num_objects should be known - by gain_access
		if (fuji_known.num_objects == 0 || fuji_known.num_objects == -1) {
			return NULL;
		}

		// (Object handles 0x0 is invalid, as per spec)
		int *list = malloc(sizeof(int) * fuji_known.num_objects);
		for (int i = 0; i < fuji_known.num_objects; i++) {
			list[i] = i + 1;
		}

		jintArray result = (*env)->NewIntArray(env, fuji_known.num_objects);
		(*env)->SetIntArrayRegion(env, result, 0, fuji_known.num_objects, list);
		free(list);

		return result;
	}
}

JNI_FUNC(jint, cFujiTestSuite)(JNIEnv *env, jobject thiz, jstring ip) {
	set_jni_env(env);

	if (backend.r.connection_type == PTP_USB) {
		return fuji_test_suite(&backend.r, NULL);
	} else {
		const char *c_ip = (*env)->GetStringUTFChars(env, ip, 0);

		int rc = fuji_test_suite(&backend.r, (char *)c_ip);

		(*env)->ReleaseStringUTFChars(env, ip, c_ip);
		(*env)->DeleteLocalRef(env, ip);

		return rc;
	}
}
