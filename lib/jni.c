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

	int rc = ptpip_fuji_init_req(&backend.r, "fudge");
	if (rc) return rc;

	struct PtpFujiInitResp resp;
	ptp_fuji_get_init_info(&backend.r, &resp);

	fuji_known.info = fuji_get_model_info(resp.cam_name);

	return rc;
}

JNI_FUNC(jint, cPtpFujiWaitUnlocked)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return fuji_wait_for_access(&backend.r);
}

JNI_FUNC(jint, cPtpFujiPing)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return fuji_get_events(&backend.r);
}

// Must be called after cFujiGetUncompressedObjectInfo
JNI_FUNC(jbyteArray, cFujiGetFile)(JNIEnv *env, jobject thiz, jint handle) {
	backend.env = env;

	int max = backend.r.data_length;

	struct PtpObjectInfo oi;
	int rc = ptp_get_object_info(&backend.r, (int)handle, &oi);
	if (rc) {
		return NULL;
	}

	jbyteArray array = (*env)->NewByteArray(env, oi.compressed_size);

	// Makes sure to set the compression prop back to 0 after finished
	// (extra data won't go through for some reason)
	int read = 0;
	while (1) {
		rc = ptp_get_partial_object(&backend.r, handle, read, max);
		if (rc == PTP_CHECK_CODE) {
			fuji_disable_compression(&backend.r);
			return NULL;
		} else if (rc) {
			return NULL;
		}

		if (ptp_get_payload_length(&backend.r) == 0) {
			fuji_disable_compression(&backend.r);
			return NULL;
		}

		if (read + ptp_get_payload_length(&backend.r) > oi.compressed_size) {
			android_err("ptp_get_object_info has lied about it's compressed size out of shame");
		}

		(*env)->SetByteArrayRegion(
			env, array,
			read, ptp_get_payload_length(&backend.r),
			(const jbyte *)(ptp_get_payload(&backend.r))
		);

		if ((*env)->ExceptionCheck(env)) {
			android_err("SetByteArrayRegion exception");
			(*env)->ExceptionClear(env);

			fuji_disable_compression(&backend.r);
			return NULL;
		}

		read += ptp_get_payload_length(&backend.r);

		if (read >= oi.compressed_size) {
			fuji_disable_compression(&backend.r);
			android_err("Downloaded %d bytes", read);
			return array;
		}
	}
}

// Must be called *before* a call to cFujiGetFile
JNI_FUNC(jstring, cFujiGetUncompressedObjectInfo)(JNIEnv *env, jobject thiz, jint handle) {
	backend.env = env;

	int rc = fuji_enable_compression(&backend.r);
	if (rc) {
		return NULL;
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

// NOTE: cFujiConfigInitMode *must* be called before cFujiConfigVersion, or anything else.
// If not, it will break up the connection and destroy packets for any file operation.
JNI_FUNC(jint, cFujiConfigInitMode)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	int rc = fuji_config_init_mode(&backend.r);

	return rc;
}

// Misnomer, should be configImageViewer
JNI_FUNC(jint, cFujiConfigVersion)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	int rc = fuji_config_version(&backend.r);
	if (rc) return rc;

	rc = fuji_config_device_info_routine(&backend.r);

	return 0;
}

JNI_FUNC(jint, cFujiConfigImageGallery)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	int rc = fuji_config_image_viewer(&backend.r);
	if (rc) return rc;

	return 0;
}

JNI_FUNC(jboolean, cIsUntestedMode)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	if (fuji_known.info == NULL) {
		return 1;
	}

	if (fuji_known.get_object_version != 2) {
		return 1;
	}

	return 0;
}

JNI_FUNC(jboolean, cCameraWantsRemote)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	// Determine if camera supports remote mode -> then it will probably require it
	return fuji_known.remote_version != -1;
}

JNI_FUNC(jboolean, cIsMultipleMode)(JNIEnv *env, jobject thiz) {
	backend.env = env;

	return fuji_known.camera_state == FUJI_MULTIPLE_TRANSFER;
}

#if 0
JNI_FUNC(jint, cTestStuff)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return 0;
}

// TODO: Idea: when activity changes, notify C so we don't run tester_log in gallery?
JNI_FUNC(void, cNotifyScreenStart)(JNIEnv *env, jobject thiz, jstring string) {
	backend.env = env;
	const char *name = (*env)->GetStringUTFChars(env, string, 0);
}
#endif

JNI_FUNC(jintArray, cGetObjectHandles)(JNIEnv *env, jobject thiz) {
	backend.env = env;

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

JNI_FUNC(jint, cFujiTestSuiteSetup)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return fuji_test_setup(&backend.r);
}

JNI_FUNC(jint, cFujiTestStartRemoteSockets)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	int rc = fuji_remote_mode_open_sockets(&backend.r);
	if (rc) {
		tester_fail("Failed to open sockets");
	} else {
		tester_log("Opened sockets in remote mode");
	}

	return rc;
}

JNI_FUNC(jint, cFujiEndRemoteMode)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	int rc = fuji_remote_mode_end(&backend.r);
	if (rc) {
		tester_fail("Failed to end remote mode");
	} else {
		tester_log("Ended remote mode after setting up sockets");
	}
	return rc;
}

JNI_FUNC(jint, cFujiTestSetupImageGallery)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	int rc = fuji_config_image_viewer(&backend.r);
	if (rc) {
		tester_fail("Failed to config image viewer");
		return rc;
	} else {
		tester_log("Configured image viewer");
	}

	rc = fuji_test_filesystem(&backend.r);
	if (rc) return rc;

	return 0;
}
