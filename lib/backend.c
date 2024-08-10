// JNI PTP/IP interface for camlib and fuji.c
// Copyright 2024 (C) Fudge
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

static inline jclass get_backend_class(JNIEnv *env) {
	return (*env)->FindClass(env, "dev/danielc/fujiapp/Backend");
}

static jbyteArray struct_to_bytearr(void *data, int length) {
	JNIEnv *env = get_jni_env();
	jbyteArray arr = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, arr, 0, length, (const jbyte *)data);
	return arr;
}

JNI_FUNC(void, cInit)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	ptp_init(&backend.r);
	fuji_reset_ptp(&backend.r);
}

JNI_FUNC(void, cTesterInit)(JNIEnv *env, jobject thiz, jobject tester) {
	set_jni_env(env);
}

JNI_FUNC(jint, cRouteLogs)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);

	backend.log_buf = malloc(1000);
	backend.log_size = 1000;
	backend.log_pos = 0;

	strcpy(backend.log_buf, "Fudge log file - Send this to devs!\n");

	ptp_verbose_log("ABI: %s\n", ABI);
	ptp_verbose_log("Compile date: %s\n", __DATE__);
	ptp_verbose_log("https://github.com/petabyt/fudge\n");
	return 0;
}

JNI_FUNC(jstring, cEndLogs)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);

	if (backend.log_buf == NULL) return NULL;

	jstring str = (*env)->NewStringUTF(env, backend.log_buf);

	free(backend.log_buf);
	backend.log_buf = NULL;

	return str;
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

/// For frontend to check connection status
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
	struct PtpRuntime *r = ptp_get();
	int rc = 0;

	// Makes sure to set the compression prop back to 0 after finished
	// (extra data won't go through for some reason)
	int read = 0;
	while (1) {
		if (app_check_thread_cancel()) {
			rc = PTP_CANCELED;
			goto end;
		}

		ptp_mutex_keep_locked(&backend.r);
		int cur = file_size - read;
		if (cur > FUJI_MAX_PARTIAL_OBJECT) cur = FUJI_MAX_PARTIAL_OBJECT;
		rc = ptp_get_partial_object(&backend.r, handle, read, cur);
		if (rc == PTP_CHECK_CODE) {
			goto end_unlock;
		} else if (rc) {
			plat_dbg("Download fail %d", rc);
			goto end_unlock;
		}

		size_t payload_size = ptp_get_payload_length(&backend.r);

		if (payload_size == 0) {
			rc = PTP_RUNTIME_ERR;
			goto end_unlock;
		}

		(*env)->SetByteArrayRegion(
			env, array,
			read, payload_size,
			(const jbyte *)(ptp_get_payload(&backend.r))
		);

		// Check for java possible buffer overflow
		if ((*env)->ExceptionCheck(env)) {
			plat_dbg("SetByteArrayRegion exception");
			(*env)->ExceptionClear(env);
			rc = PTP_OUT_OF_MEM;
			goto end_unlock;
		}

		read += ptp_get_payload_length(&backend.r);

		ptp_mutex_unlock(&backend.r);

		if (read >= file_size) {
			plat_dbg("Downloaded %d bytes", read);
			rc = 0;
			goto end;
		}
	}

	end:;
	if (fuji_get(r)->transport == FUJI_FEATURE_WIRELESS_COMM) {
		fuji_disable_compression(r);
	}
	return rc;

	end_unlock:;
	if (fuji_get(r)->transport == FUJI_FEATURE_WIRELESS_COMM) {
		fuji_disable_compression(r);
	}
	ptp_mutex_unlock(r);
	return rc;
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
	if (fuji_get(r)->transport == FUJI_FEATURE_AUTOSAVE) {
		ptp_mutex_keep_locked(r);
		int length, offset;
		int rc = ptp_get_partial_exif(r, handle, &offset, &length);
		if (rc) {
			ptp_mutex_unlock(r);
			return (*env)->NewByteArray(env, 0);
		}

		jbyteArray ret = (*env)->NewByteArray(env, length);
		(*env)->SetByteArrayRegion(env, ret, 0, length, (const jbyte *) ptp_get_payload(r) + offset);
		ptp_mutex_unlock(r);

		return ret;
	} else {
		ptp_mutex_keep_locked(r);
		int rc = ptp_get_thumbnail(r, (int)handle);
		if (rc == PTP_CHECK_CODE || ptp_get_payload_length(r) < 100) {
			plat_dbg("Thumbnail get failed");
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
}

JNI_FUNC(jint, cFujiSetup)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	struct PtpRuntime *r = ptp_get();

	if (r->connection_type == PTP_USB) return fujiusb_setup(r);

	int rc = fuji_setup(r);

	if (!rc && fuji_get(r)->camera_state == FUJI_MULTIPLE_TRANSFER) {
		rc = fuji_download_classic(r);
		if (rc) {
			app_print("Error downloading images");
			return rc;
		}
		app_print("Check your file manager app/gallery.");
		ptp_report_error(r, "Disconnected", 0);
	}

	return rc;
}

JNI_FUNC(jint, cFujiConfigImageGallery)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
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
// TODO: Rename cFujiGetObjectHandles
JNI_FUNC(jintArray, cGetObjectHandles)(JNIEnv *env, jobject thiz) {
	struct PtpRuntime *r = ptp_get();
	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	if (backend.r.connection_type == PTP_USB) {
		return ptpusb_get_object_handles(env, &backend.r);
	} else {
		// By this point num_objects should be known - by gain_access
		if (fuji->num_objects == 0 || fuji->num_objects == -1) {
			return NULL;
		}

		// (Object handles 0x0 is invalid, as per spec)
		int *list = malloc(sizeof(int) * fuji->num_objects);
		for (int i = 0; i < fuji->num_objects; i++) {
			list[i] = i + 1;
		}

		jintArray result = (*env)->NewIntArray(env, fuji->num_objects);
		(*env)->SetIntArrayRegion(env, result, 0, fuji->num_objects, list);
		free(list);

		return result;
	}
}

JNI_FUNC(jint, cFujiTestSuite)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	return fuji_test_suite(&backend.r);
}

JNI_FUNC(jint, cGetTransport)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	return fuji_get(ptp_get())->transport;
}

JNI_FUNC(jint, cTryConnectWiFi)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	const char *c_ip = app_get_camera_ip();

	int rc = ptpip_connect(&backend.r, c_ip, FUJI_CMD_IP_PORT);
	if (rc == 0) {
		fuji_reset_ptp(ptp_get());
		strcpy(fuji_get(ptp_get())->ip_address, c_ip);
		fuji_get(ptp_get())->transport = FUJI_FEATURE_WIRELESS_COMM;
	}

	return rc;
}

JNI_FUNC(jint, cConnectFromDiscovery)(JNIEnv *env, jobject thiz, jbyteArray discovery) {
	set_jni_env(env);

	struct DiscoverInfo *info = (struct DiscoverInfo *)(*env)->GetByteArrayElements(env, discovery, 0);

	int rc = ptpip_connect(&backend.r, info->camera_ip, info->camera_port);
	if (rc == 0) {
		fuji_reset_ptp(ptp_get());
		strcpy(fuji_get(ptp_get())->ip_address, info->camera_ip);
		fuji_get(ptp_get())->transport = info->transport;
	}

	(*env)->ReleaseByteArrayElements(env, discovery, (jbyte *)info, JNI_ABORT);

	return rc;
}

JNI_FUNC(jint, cConnectNative)(JNIEnv *env, jobject thiz, jstring ip, jint port) {
	set_jni_env(env);
	const char *c_ip = (*env)->GetStringUTFChars(env, ip, 0);

	int rc = ptpip_connect(&backend.r, c_ip, (int)port);

	if (rc == 0) {
		//fuji_reset_ptp(ptp_get()); // ???
		strcpy(fuji_get(ptp_get())->ip_address, c_ip);
		// TODO: ->transport ???
	}

	(*env)->ReleaseStringUTFChars(env, ip, c_ip);

	return rc;
}

JNI_FUNC(jint, cFujiImportFiles)(JNIEnv *env, jobject thiz, jintArray handles) {
	set_jni_env(env);

	jsize length = (*env)->GetArrayLength(env, handles);
	jint *handles_n = (*env)->GetIntArrayElements(env, handles, NULL);

	int rc = fuji_import_all(ptp_get(), handles_n, length);

	(*env)->ReleaseIntArrayElements(env, handles, handles_n, 0);

	return rc;
}

int fuji_discover_ask_connect(void *arg, struct DiscoverInfo *info) {
	JNIEnv *env = get_jni_env();
	jobject ctx = get_jni_ctx();
	// Ask if we want to connect?
	// onReceiveCameraInfo
	jmethodID register_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "dev/danielc/fujiapp/MainActivity"), "onReceiveCameraInfo",
											   "(Ljava/lang/String;Ljava/lang/String;[B)V");
	(*env)->CallVoidMethod(env, ctx, register_m,
	   (*env)->NewStringUTF(env, info->camera_model),
	   (*env)->NewStringUTF(env, info->camera_name),
	   struct_to_bytearr(info, sizeof(struct DiscoverInfo))
	);
	return 1;
}

int fuji_discovery_check_cancel(void *arg) {
	if (app_check_thread_cancel()) {
		return 1;
	}
	return 0;
}

void fuji_discovery_update_progress(void *arg, int progress) {
	switch (progress) {
	case 0:
		app_print_id(app_get_string("discovery1")); return;
	case 1:
		app_print("Exchanging a loving greeting..."); return;
	case 2:
		app_print("Waiting for the camera to invite us in..."); return;
	case 3:
		app_print("Waiting for the camera to tell us it's secrets..."); return;
	case 4:
		app_print("Accepting the camera's offer..."); return;
	default:
		app_print("...");
	}
}

volatile static int already_discovering = 0;
JNI_FUNC(jint, cStartDiscovery)(JNIEnv *env, jobject thiz, jobject ctx) {
	struct PtpRuntime *r = ptp_get();
	struct FujiDeviceKnowledge *fuji = fuji_get(r);

	set_jni_env_ctx(env, ctx);
	if (already_discovering) {
		plat_dbg("cStartDiscovery called twice, sanity check failed");
		abort();
	}
	already_discovering = 1;

	struct DiscoverInfo info;
	int rc = fuji_discover_thread(&info, "Fudge", fuji_get(ptp_get()));
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
												   "(Ljava/lang/String;Ljava/lang/String;[B)V");
		(*env)->CallVoidMethod(env, ctx, register_m,
			(*env)->NewStringUTF(env, info.camera_model),
			(*env)->NewStringUTF(env, info.camera_name),
			struct_to_bytearr(&info, sizeof(struct DiscoverInfo))
		);
	} else if (rc < 0) {
		app_print("Discovery thread err: %d", rc);
		already_discovering = 0;
		return rc;
	}

	already_discovering = 0;
	return rc;
}

static jlong get_handle() {
	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "dev/danielc/common/WiFiComm");
	jmethodID get_handle_m = (*env)->GetStaticMethodID(env, class, "getNetworkHandle", "()J");
	jlong handle = (*env)->CallStaticLongMethod(env, class, get_handle_m);
	return handle;
}

int app_bind_socket_wifi(int fd) {
//	if (app_do_connect_without_wifi()) {
//		return 0;
//	}

	typedef int (*_android_setsocknetwork_td)(jlong handle, int fd);

	// https://developer.android.com/ndk/reference/group/networking#android_setsocknetwork
	void *lib = dlopen("libandroid.so", RTLD_NOW);
	_android_setsocknetwork_td _android_setsocknetwork = (_android_setsocknetwork_td)dlsym(lib, "android_setsocknetwork");

	if (_android_setsocknetwork == NULL) {
		ptp_verbose_log("android_setsocknetwork not found");
		return -1;
	}

	jlong handle = get_handle();
	if (handle < 0) {
		return handle;
	}

	int rc = _android_setsocknetwork(handle, fd);

	dlclose(lib);

	return rc;
}
