// JNI PTP/IP interface for camlib and fuji.c
// Copyright 2023 (C) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include <dlfcn.h>
#include <ui.h>
#include "app.h"
#include "backend.h"
#include "fuji.h"
#include "fujiptp.h"

volatile int download_cancel = 1;

JNI_FUNC(int, cCancelDownload)(JNIEnv *env, jobject thiz) {
	if (!download_cancel) {
		download_cancel = 1;
		return 1;
	}
	return 0;
}

JNI_FUNC(void, cReportError)(JNIEnv *env, jobject thiz, jint code, jstring reason) {
	set_jni_env(env);

	const char *c_reason = (*env)->GetStringUTFChars(env, reason, 0);

	ptp_report_error(&backend.r, c_reason, (int)code);

	if (backend.r.connection_type == PTP_USB) {
		ptp_device_close(&backend.r);
	} else {
		ptpip_close(&backend.r);
	}

	(*env)->ReleaseStringUTFChars(env, reason, c_reason);
	(*env)->DeleteLocalRef(env, reason);
}

// TODO: remove
JNI_FUNC(void, cClearKillSwitch)(JNIEnv *env, jobject thiz) {
	backend.r.io_kill_switch = 0;
}
// TODO: Remove?
JNI_FUNC(jboolean, cGetKillSwitch)(JNIEnv *env, jobject thiz) {
	return backend.r.io_kill_switch != 0;
}

JNI_FUNC(jint, cPtpFujiPing)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	if (backend.r.connection_type == PTP_USB) {
		// TODO: Poll int endpoint?
		return 0;
	}

	return fuji_get_events(&backend.r);
}

JNI_FUNC(jint, cFujiDownloadFile)(JNIEnv *env, jobject thiz, jint handle, jstring path) {
	set_jni_env(env);

	const char *c_path = (*env)->GetStringUTFChars(env, path, 0);

	plat_dbg("Downloading to %s", c_path);
	FILE *f = fopen(c_path, "wb");
	if (f == NULL) return PTP_RUNTIME_ERR;

	int rc = ptp_download_object(&backend.r, handle, f, FUJI_MAX_PARTIAL_OBJECT);
	fclose(f);
	if (rc) {
		app_print("Failed to save %s: %s", c_path, ptp_perror(rc));
		return rc;
	}

	(*env)->ReleaseStringUTFChars(env, path, c_path);
	(*env)->DeleteLocalRef(env, path);

	return 0;
}

// PTP_IP_USB: Must be called after cFujiGetUncompressedObjectInfo
JNI_FUNC(jint, cFujiGetFile)(JNIEnv *env, jobject thiz, jint handle, jbyteArray array, jint file_size) {
	set_jni_env(env);

	download_cancel = 0;

	// Makes sure to set the compression prop back to 0 after finished
	// (extra data won't go through for some reason)
	int read = 0;
	while (1) {
		if (download_cancel) {
			if (backend.r.connection_type == PTP_IP_USB) fuji_disable_compression(&backend.r);
			return PTP_CANCELED;
		}

		ptp_mutex_keep_locked(&backend.r);
		int cur = file_size - read; if (cur > FUJI_MAX_PARTIAL_OBJECT) cur = FUJI_MAX_PARTIAL_OBJECT;
		int rc = ptp_get_partial_object(&backend.r, handle, read, cur);
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
			ptp_mutex_unlock(&backend.r);
			return rc;
		}

		(*env)->SetByteArrayRegion(
			env, array,
			read, payload_size,
			(const jbyte *)(ptp_get_payload(&backend.r))
		);

		// Check for possible buffer overflow
		if ((*env)->ExceptionCheck(env)) {
			plat_dbg("SetByteArrayRegion exception");
			(*env)->ExceptionClear(env);
			return PTP_OUT_OF_MEM;
		}

		read += ptp_get_payload_length(&backend.r);

		ptp_mutex_unlock(&backend.r);

		if (read >= file_size) {
			plat_dbg("Downloaded %d bytes", read);
			if (backend.r.connection_type == PTP_IP_USB) fuji_disable_compression(&backend.r);
			return 0;
		}
	}
}

// PTP_IP_USB: Must be called *before* a call to cFujiGetFile
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

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));

	jstring ret = (*env)->NewStringUTF(env, buffer);
	return ret;
}

JNI_FUNC(jbyteArray, cFujiGetThumb)(JNIEnv *env, jobject thiz, jint handle) {
	set_jni_env(env);
	struct PtpRuntime *r = ptp_get();

	{
		ptp_mutex_keep_locked(r);
		int length, offset;
		int rc = ptp_dirty_rotten_thumb_hack(r, handle, &offset, &length);
		if (rc) {
		ptp_mutex_unlock(r);
		return (*env)->NewByteArray(env, 0);
		}

		jbyteArray ret = (*env)->NewByteArray(env, length);
		(*env)->SetByteArrayRegion(env, ret, 0, length, (const jbyte *) ptp_get_payload(r) + offset);
		ptp_mutex_unlock(r);

		return ret;
	}

	ptp_mutex_keep_locked(r);
	int rc = ptp_get_thumbnail(r, (int)handle);
	if (rc == PTP_CHECK_CODE || ptp_get_payload_length(r) < 100) {
	__android_log_write(ANDROID_LOG_ERROR, "camlib", "Thumbnail get failed");
	ptp_mutex_unlock(r);
	return (*env)->NewByteArray(env, 0);
	} else if (rc) {
	ptp_mutex_unlock(r);
	return NULL;
	}

	jbyteArray ret = (*env)->NewByteArray(env, ptp_get_payload_length(r));
	(*env)->SetByteArrayRegion(env, ret, 0, ptp_get_payload_length(r), (const jbyte *)(ptp_get_payload(r)));
	ptp_mutex_unlock(r);

	return ret;
}

JNI_FUNC(jint, cFujiSetup)(JNIEnv *env, jobject thiz, jstring ip) {
	set_jni_env(env);

	if (backend.r.connection_type == PTP_USB) return fujiusb_setup(&backend.r);

	const char *c_ip = (*env)->GetStringUTFChars(env, ip, 0);

	int rc = fuji_setup(&backend.r, c_ip);

	(*env)->ReleaseStringUTFChars(env, ip, c_ip);
	(*env)->DeleteLocalRef(env, ip);

	return rc;
}

JNI_FUNC(jint, cFujiConfigImageGallery)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	if (backend.r.connection_type == PTP_USB) return 0;
	int rc = fuji_config_image_viewer(&backend.r);
	return rc;
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

		int rc = fuji_test_suite(&backend.r, c_ip);

		(*env)->ReleaseStringUTFChars(env, ip, c_ip);
		(*env)->DeleteLocalRef(env, ip);

		return rc;
	}
}

JNI_FUNC(jint, cTryConnectWiFi)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	const char *c_ip = fuji_get_camera_ip();

	int rc = ptpip_connect(&backend.r, c_ip, FUJI_CMD_IP_PORT);

	if (rc == 0) {
		fuji_reset_ptp(ptp_get()); // ???
		strcpy(fuji_get(ptp_get())->ip_address, c_ip);
	}

	return rc;
}

JNI_FUNC(jint, cConnectNative)(JNIEnv *env, jobject thiz, jstring ip, jint port) {
	set_jni_env(env);
	const char *c_ip = (*env)->GetStringUTFChars(env, ip, 0);

	int rc = ptpip_connect(&backend.r, c_ip, (int)port);

	if (rc == 0) {
		fuji_reset_ptp(ptp_get()); // ???
		strcpy(fuji_get(ptp_get())->ip_address, c_ip);
	}

	(*env)->ReleaseStringUTFChars(env, ip, c_ip);

	return rc;
}

struct FujiDiscoverArg {
	JNIEnv *env;
	jobject ctx;
};

int fuji_discover_ask_connect(void *arg, struct DiscoverInfo *info) {
	struct FujiDiscoverArg *farg = (struct FujiDiscoverArg *)arg;
	JNIEnv *env = farg->env;
	jobject ctx = farg->ctx;
	// Ask if we want to connect?
	// onReceiveCameraInfo
	jmethodID register_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "dev/danielc/fujiapp/MainActivity"), "onReceiveCameraInfo",
											   "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	(*env)->CallVoidMethod(env, ctx, register_m,
						   (*env)->NewStringUTF(env, info->camera_model),
						   (*env)->NewStringUTF(env, info->camera_name),
						   (*env)->NewStringUTF(env, info->camera_ip)
	);
	return 1;
}

int fuji_discovery_check_cancel(void *arg) {
	return 0;
}

volatile int already_discovering = 0;
JNI_FUNC(jint, cStartDiscovery)(JNIEnv *env, jobject thiz, jobject ctx) {
	if (already_discovering) return 0;
	already_discovering = 1;
	set_jni_env(env);
	struct DiscoverInfo info;
	struct FujiDiscoverArg arg = {
		.env = env,
		.ctx = ctx,
	};
	int rc = fuji_discover_thread(&info, "Fudge", &arg);
	if (rc == FUJI_D_REGISTERED) {
		jmethodID register_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "dev/danielc/fujiapp/MainActivity"), "onCameraRegistered",
			"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
		(*env)->CallVoidMethod(env, ctx, register_m,
			(*env)->NewStringUTF(env, info.camera_model),
			(*env)->NewStringUTF(env, info.camera_name),
			(*env)->NewStringUTF(env, info.camera_ip)
		);
	} else if (rc == FUJI_D_GO_PTP) {
		jmethodID register_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "dev/danielc/fujiapp/MainActivity"), "onCameraWantsToConnect",
												   "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
		(*env)->CallVoidMethod(env, ctx, register_m,
			(*env)->NewStringUTF(env, info.camera_model),
			(*env)->NewStringUTF(env, info.camera_name),
			(*env)->NewStringUTF(env, info.camera_ip),
			info.camera_port
		);
	} else if (rc < 0) {
		app_print("AutoSave Err: %d", rc);
	}
	already_discovering = 0;
	return rc;
}

static jlong get_handle() {
	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "camlib/WiFiComm");
	jmethodID get_handle_m = (*env)->GetStaticMethodID(env, class, "getNetworkHandle", "()J");
	jlong handle = (*env)->CallStaticLongMethod(env, class, get_handle_m);
	return handle;
}

int app_bind_socket_wifi(int fd) {
	typedef int (*_android_setsocknetwork_td)(jlong handle, int fd);

	// https://developer.android.com/ndk/reference/group/networking#android_setsocknetwork
	void *lib = dlopen("libandroid.so", RTLD_NOW);
	_android_setsocknetwork_td _android_setsocknetwork = (_android_setsocknetwork_td)dlsym(lib, "android_setsocknetwork");

	if (_android_setsocknetwork == NULL) {
		return -1;
	}

	jobject jni_get_application_ctx(JNIEnv *env);
	jobject jni_get_pref(JNIEnv *env, jobject ctx, char *key);
	//plat_dbg("WiFi: %d\n", jni_get_pref(get_jni_env(), jni_get_application_ctx(get_jni_env()), "foo"));

	jlong handle = get_handle();
	if (handle < 0) {
		return handle;
	}

	int rc = _android_setsocknetwork(handle, fd);

	dlclose(lib);

	return rc;
}
