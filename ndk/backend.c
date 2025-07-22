// JNI PTP/IP interface for libpict and fuji.c
// Copyright 2024 (C) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libpict.h>
#include <dlfcn.h>
#include "app.h"
#include "backend.h"
#include "fuji.h"
#include "fujiptp.h"
#include "object.h"

static struct DiscoverInfo fuji_discover_struct;

jbyteArray jni_struct_to_bytearr(void *data, int length) {
	JNIEnv *env = get_jni_env();
	jbyteArray arr = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, arr, 0, length, (const jbyte *)data);
	return arr;
}

jobject jni_string_to_jsonobject(JNIEnv *env, const char *str) {
	jclass json_object_class;
	jmethodID json_object_constructor;
	jobject json_object;
	jstring json_string;

	json_object_class = (*env)->FindClass(env, "org/json/JSONObject");
	json_object_constructor = (*env)->GetMethodID(env, json_object_class, "<init>", "(Ljava/lang/String;)V");

	json_string = (*env)->NewStringUTF(env, str);
	json_object = (*env)->NewObject(env, json_object_class, json_object_constructor, json_string);

	(*env)->DeleteLocalRef(env, json_string);

	return json_object;
}

JNI_FUNC(void, cInit)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	ptp_init(&backend.r);
	fuji_reset_ptp(&backend.r);
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

	(*env)->ReleaseStringUTFChars(env, reason, c_reason);
	(*env)->DeleteLocalRef(env, reason);
}

/// For frontend to check connection status
JNI_FUNC(jboolean, cGetKillSwitch)(JNIEnv *env, jobject thiz) {
	return backend.r.io_kill_switch != 0;
}

JNI_FUNC(jint, cPtpFujiPing)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	struct PtpRuntime *r = ptp_get();
	if (backend.r.connection_type == PTP_USB) {
		int ptpusb_get_status(struct PtpRuntime *r);
		return ptpusb_get_status(r);
		if (r->io_kill_switch) {
			return PTP_IO_ERR;
		}
		return 0;
	}

	return fuji_get_events(r);
}

JNI_FUNC(jint, cFujiDownloadFile)(JNIEnv *env, jobject thiz, jint handle, jstring path) {
	set_jni_env(env);

	const char *c_path = (*env)->GetStringUTFChars(env, path, 0);

	plat_dbg("Downloading to %s", c_path);
	FILE *f = fopen(c_path, "wb");
	if (f == NULL) return PTP_RUNTIME_ERR;

	app_set_progress_bar(1, 1000000);
	int rc = ptp_download_object(&backend.r, handle, f, FUJI_MAX_PARTIAL_OBJECT);
	app_set_progress_bar(0, 0);
	fclose(f);
	if (rc) {
		app_print("Failed to save %s: %s", c_path, ptp_perror(rc));
		return rc;
	}

	(*env)->ReleaseStringUTFChars(env, path, c_path);
	(*env)->DeleteLocalRef(env, path);

	return 0;
}

static int jbytearray_add(void *arg, void *data, int size, int read) {
	JNIEnv *env = get_jni_env();
	(*env)->SetByteArrayRegion(
		env, (jbyteArray)arg,
		read, size,
		(const jbyte *)data
	);

	// Check for java possible buffer overflow
	if ((*env)->ExceptionCheck(env)) {
		plat_dbg("SetByteArrayRegion exception: %d, %d", size, read);
		(*env)->ExceptionClear(env);
		return -1;
	}

	return 0;
}

// PTP_IP_USB: Must be called after cFujiGetUncompressedObjectInfo
JNI_FUNC(jint, cFujiGetFile)(JNIEnv *env, jobject thiz, jint handle, jbyteArray array, jint file_size) {
	set_jni_env(env);
	struct PtpRuntime *r = ptp_get();

	app_set_progress_bar(1, file_size);
	int rc = fuji_download_file(r, handle, file_size, jbytearray_add, array);
	app_set_progress_bar(0, 0);
	if (rc) return rc;

	return 0;
}

// PTP_IP_USB: Must be called *before* a call to cFujiGetFile
JNI_FUNC(jstring, cFujiBeginDownloadGetObjectInfo)(JNIEnv *env, jobject thiz, jint handle) {
	set_jni_env(env);

	struct PtpObjectInfo oi;
	int rc = fuji_begin_download_get_object_info(ptp_get(), (int) handle, &oi);
	if (rc) return NULL;

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));

	jstring ret = (*env)->NewStringUTF(env, buffer);
	return ret;
}

JNI_FUNC(jbyteArray, cFujiGetThumb)(JNIEnv *env, jobject thiz, jint handle) {
	set_jni_env(env);
	struct PtpRuntime *r = ptp_get();

	int offset, length;
	ptp_mutex_lock(r);

	int rc = fuji_get_thumb(r, handle, &offset, &length);
	if (rc) {
		ptp_mutex_unlock(r);
		return NULL;
	}

	// If error from fuji_get_thumb, array will be zero
	jbyteArray ret = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, ret, 0, length, (const jbyte *) ptp_get_payload(r) + offset);

	ptp_mutex_unlock(r);

	return ret;
}

JNI_FUNC(jint, cFujiSetup)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	struct PtpRuntime *r = ptp_get();
	if (r->io_kill_switch) {
		plat_dbg("BUG: cFujiSetup called with kill switch on");
		return PTP_IO_ERR;
	}

	return fuji_connection_entry(r);
}

JNI_FUNC(jint, cFujiConfigImageGallery)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	int rc = fuji_config_image_viewer(&backend.r);
	return rc;
}

// Is this a good idea?
JNI_FUNC(void, cPtpUnlockAllThreads)(JNIEnv *env, jobject thiz) {
	ptp_mutex_unlock_thread(ptp_get());
}

jintArray ptpusb_get_object_handles(JNIEnv *env, struct PtpRuntime *r) {
	struct PtpArray *arr;
	int rc = ptp_get_storage_ids(r, &arr);
	if (rc) return NULL;

	if (arr->length == 0) {
		jintArray result = (*env)->NewIntArray(env, 0);
		return result;
	}

	int id = (int)arr->data[0];

	free(arr);

	// Get all objects in root
	rc = ptp_get_object_handles(r, id, 0, 0, &arr);
	if (rc) return NULL;

	jintArray result = (*env)->NewIntArray(env, (jsize)arr->length);
	(*env)->SetIntArrayRegion(env, result, 0, (jsize)arr->length, (const int *)arr->data);

	free(arr);

	return result;
}

// Return array of valid objects on main storage device
// TODO: Rename cFujiGetObjectHandles
JNI_FUNC(jintArray, cFujiGetObjectHandles)(JNIEnv *env, jobject thiz) {
	struct PtpRuntime *r = ptp_get();

	struct PtpArray *a;
	int rc = ptp_fuji_get_object_handles(r, &a);
	if (rc) return NULL;

	jintArray result = (*env)->NewIntArray(env, a->length);
	(*env)->SetIntArrayRegion(env, result, 0, a->length, (const jint *)a->data);
	return result;
}

JNI_FUNC(jint, cFujiTestSuite)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	return fuji_test_suite(&backend.r);
}

JNI_FUNC(jint, cGetTransport)(JNIEnv *env, jobject thiz) {
	set_jni_env(env);
	return fuji_get(ptp_get())->transport;
}

JNI_FUNC(jint, cTryConnectWiFi)(JNIEnv *env, jobject thiz, jint extra_tmout) {
	set_jni_env(env);
	struct PtpRuntime *r = ptp_get();
	char *c_ip = app_get_camera_ip();
	fuji_reset_ptp(r);
	strcpy(fuji_get(r)->ip_address, c_ip);
	fuji_get(r)->transport = FUJI_FEATURE_WIRELESS_COMM;
	if (app_get_wifi_network_handle(&fuji_get(r)->net)) {
		free(c_ip);
		return -1;
	}
	int rc = ptpip_connect(&backend.r, c_ip, FUJI_CMD_IP_PORT, (int)extra_tmout);
	free(c_ip);

	return rc;
}

JNI_FUNC(jint, cTryConnectUSB)(JNIEnv *env, jclass thiz, jobject ctx) {
	set_jni_env_ctx(env, ctx);
	struct PtpRuntime *r = ptp_get();
	int rc = fujiusb_try_connect(r, 0);
	return rc;
}

#if 0
JNI_FUNC(jint, cConnectFromDiscovery)(JNIEnv *env, jobject thiz) {
	set_jni_env_ctx(env, NULL);

	struct PtpRuntime *r = ptp_get();

	struct DiscoverInfo *info = &fuji_discover_struct;
	if (info == NULL) ptp_panic("info is null");

	return fuji_connect_from_discoverinfo(r, info);
}
#endif

JNI_FUNC(jint, cFujiImportFiles)(JNIEnv *env, jobject thiz, jintArray handles, int mask) {
	set_jni_env(env);

	jsize length = (*env)->GetArrayLength(env, handles);
	jint *handles_n = (*env)->GetIntArrayElements(env, handles, NULL);

	int rc = fuji_import_objects(ptp_get(), handles_n, length, mask);

	(*env)->ReleaseIntArrayElements(env, handles, handles_n, 0);

	return rc;
}

// For tether, unused for now
int fuji_discover_ask_connect(void *arg, struct DiscoverInfo *info) {
	JNIEnv *env = get_jni_env();
	jobject ctx = get_jni_ctx();
	jmethodID register_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "dev/danielc/fujiapp/MainActivity"), "onReceiveCameraInfo", "(Ljava/lang/String;Ljava/lang/String;[B)Z");
	return (*env)->CallBooleanMethod(env, ctx, register_m,
		(*env)->NewStringUTF(env, info->camera_model),
		(*env)->NewStringUTF(env, info->camera_name),
		jni_struct_to_bytearr(info, sizeof(struct DiscoverInfo))
	);
}

int fuji_discovery_check_cancel(void *arg) {
	(void)arg;
	return app_check_thread_cancel();
}

void fuji_discovery_update_progress(void *arg, enum DiscoverUpdateMessages progress) {
	switch (progress) {
	case FUJI_UM_GOT_FIRST_MESSAGE:
		app_print_id(app_get_string("discovery1")); return;
	case FUJI_UM_CONNECTING_TO_NOTIFY_SERVER:
		app_print("Exchanging a loving greeting..."); return;
	case FUJI_UM_STARTING_INVITE_SERVER:
		app_print("Please start touching your camera..."); return;
	case FUJI_UM_CAMERA_CONNECTED_TO_INVITE_SERVER:
		app_print("Waiting for the camera to tell us her secrets..."); return;
	case FUJI_UM_ALL_DONE:
		app_print("Starting a relationship with the camera..."); return;
	default:
		app_print("...");
	}
}

static int already_discovering = 0;
JNI_FUNC(jint, cStartDiscovery)(JNIEnv *env, jobject thiz, jobject ctx) {
	struct PtpRuntime *r = ptp_get();
	struct FujiDeviceKnowledge *fuji = fuji_get(r);

	set_jni_env_ctx(env, ctx);
	if (already_discovering) {
		plat_dbg("cStartDiscovery called twice, sanity check failed");
		abort();
	}
	already_discovering = 1;

	(*env)->PushLocalFrame(env, 10);

	struct DiscoverInfo *info = &fuji_discover_struct;

	int rc = fuji_discover_thread(info, app_get_client_name(), NULL);
	if (rc > 0) {
		jstring model = (*env)->NewStringUTF(env, info->camera_model);
		jstring name = (*env)->NewStringUTF(env, info->camera_name);
		jstring ip = (*env)->NewStringUTF(env, info->camera_ip);
		if (rc == FUJI_D_REGISTERED) {
			jmethodID register_m = (*env)->GetStaticMethodID(env, get_frontend_class(env), "onCameraRegistered", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
			(*env)->CallStaticVoidMethod(env, get_frontend_class(env), register_m, model, name, ip);
		} else if (rc == FUJI_D_GO_PTP) {
			jmethodID register_m = (*env)->GetStaticMethodID(env, get_frontend_class(env), "onCameraWantsToConnect", "(Ljava/lang/String;Ljava/lang/String;)V");
			(*env)->CallStaticVoidMethod(env, get_frontend_class(env), register_m, model, name);
		}
	} else if (rc < 0) {
		already_discovering = 0;
		(*env)->PopLocalFrame(env, NULL);
		return rc;
	}

	already_discovering = 0;
	(*env)->PopLocalFrame(env, NULL);
	return rc;
}

int app_get_wifi_network_handle(struct NetworkHandle *h) {
	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "dev/danielc/common/WiFiComm");
	jmethodID get_handle_m = (*env)->GetStaticMethodID(env, class, "getNetworkHandle", "()J");
	h->android_fd = (*env)->CallStaticLongMethod(env, class, get_handle_m);
	if (h->android_fd < 0) {
		return -1;
	}
	return 0;
}

int app_get_os_network_handle(struct NetworkHandle *h) {
	// By default, a socket() to bind to whatever interface it finds the IP address on.
	// We want this for PC AutoSave, since we can't get the handle for the hotspot interface.
	// There is a slight chance though, if you're connected to a internet WiFi network and have a
	// hotspot open, it may bind to the wrong interface.
	h->ignore = 1;
	return 0;
}

int app_bind_socket_to_network(int fd, struct NetworkHandle *h) {
	if (h->ignore) return 0;
	typedef int (*_android_setsocknetwork_td)(jlong handle, int fd);

	// https://developer.android.com/ndk/reference/group/networking#android_setsocknetwork
	void *lib = dlopen("libandroid.so", RTLD_NOW);
	_android_setsocknetwork_td _android_setsocknetwork = (_android_setsocknetwork_td)dlsym(lib, "android_setsocknetwork");

	if (_android_setsocknetwork == NULL) {
		plat_dbg("android_setsocknetwork not found");
		return -1;
	}

	jlong handle = h->android_fd;

	int rc = _android_setsocknetwork(handle, fd);
	if (rc == -1) {
		plat_dbg("android_setsocknetwork failed: %d", errno);
		plat_dbg("Handle: %ld", handle);
		return errno;
	}

	return 0;
}
