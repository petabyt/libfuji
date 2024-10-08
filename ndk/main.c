// Init routines
// Copyright 2023 (c) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include <android.h>
#include "fuji.h"
#include "app.h"
#include "backend.h"

struct AndroidBackend backend = {0};

__thread struct AndroidLocal local = {0, 0};

void set_jni_env_ctx(JNIEnv *env, jobject ctx) {
	local.env = env;
	local.ctx = ctx;
}

struct AndroidLocal push_jni_env_ctx(JNIEnv *env, jobject ctx) {
	struct AndroidLocal l;
	l.env = local.env;
	l.ctx = local.ctx;
	local.env = env;
	local.ctx = ctx;
	return l;
}

void pop_jni_env_ctx(struct AndroidLocal l) {
	set_jni_env_ctx(l.env, l.ctx);
}

JNIEnv *get_jni_env(void) {
	if (local.env == NULL) {
		plat_dbg("JNIEnv not set for this thread");
		abort();
	}

	return local.env;
}

jobject get_jni_ctx(void) {
	if (local.ctx == NULL) {
		plat_dbg("ctx not set for this thread");
		abort();
	}

	return local.ctx;
}

struct PtpRuntime *ptp_get(void) {
	return &backend.r;
}

struct PtpRuntime *luaptp_get_runtime(void *L) {
	(void)L;
	return ptp_get();
}

void app_get_file_path(char buffer[256], const char *filename) {
	sprintf(buffer, "/storage/emulated/0/Pictures/fudge/%s", filename);
}

#define VERBOSE

void ptp_verbose_log(char *fmt, ...) {
#ifndef VERBOSE
	if (backend.log_buf == NULL) return;
#endif

	char buffer[512] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	__android_log_write(ANDROID_LOG_ERROR, "ptp_verbose_log", buffer);

	// not thread safe
#ifdef VERBOSE
	if (backend.log_buf == NULL) return;
#endif

	char buffer2[512];
	snprintf(buffer2, sizeof(buffer2), "%d\t%s", (int)clock() * 1000 / CLOCKS_PER_SEC, buffer);

	if (strlen(buffer2) + backend.log_pos + 1 > backend.log_size) {
		backend.log_buf = realloc(backend.log_buf, strlen(buffer2) + backend.log_pos + 1);
		if (backend.log_buf == NULL) abort();
	}

	strcpy(((char *)backend.log_buf) + backend.log_pos, buffer2);
	backend.log_pos += strlen(buffer2);
}

void ptp_panic(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	__android_log_write(ANDROID_LOG_ERROR, "ptp_panic", buffer);
	abort();
}

int app_get_string(const char *key) {
	return jni_get_string_id(get_jni_env(), get_jni_ctx(), key);
}

static inline jclass get_frontend_class(JNIEnv *env) {
	return (*env)->FindClass(env, "dev/danielc/fujiapp/Frontend");
}

#define get_frontend_class(env) (*env)->FindClass(env, "dev/danielc/fujiapp/Frontend")

static inline jclass get_tester_class(JNIEnv *env) {
	return (*env)->FindClass(env, "dev/danielc/fujiapp/Tester");
}

static jobject jni_to_json(JNIEnv *env, const char *string) {
	(*env)->PushLocalFrame(env, 10);
	jclass json_class = (*env)->FindClass(env, "org/json/JSONObject");
	jmethodID constructor = (*env)->GetMethodID(env, json_class, "<init>", "(Ljava/lang/String;)V");

	jstring jstring_arg = (*env)->NewStringUTF(env, string);
	if (jstring_arg == NULL) {
		return NULL;
	}

	jobject json_object = (*env)->NewObject(env, json_class, constructor, jstring_arg);

	return (*env)->PopLocalFrame(env, json_object);
}

int app_check_thread_cancel(void) {
	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 10);
	jclass thread_class = (*env)->FindClass(env, "java/lang/Thread");
	jmethodID current_thread_id = (*env)->GetStaticMethodID(env, thread_class, "currentThread", "()Ljava/lang/Thread;");
	jobject current_thread = (*env)->CallStaticObjectMethod(env, thread_class, current_thread_id);
	jmethodID is_interrupted_id = (*env)->GetMethodID(env, thread_class, "isInterrupted", "()Z");
	int sts = (int)(*env)->CallBooleanMethod(env, current_thread, is_interrupted_id);
	(*env)->PopLocalFrame(env, NULL);
	return sts;
}

void app_downloading_file(const struct PtpObjectInfo *oi) {
	plat_dbg("Downloading file %s", oi->filename);
	// Photo importer has started downloading a file, send signal
	char oi_buffer[512];
	ptp_object_info_json(oi, oi_buffer, sizeof(oi_buffer));

	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 5);

	jobject json = jni_to_json(env, oi_buffer);

	jclass frontend_c = get_frontend_class(env);
	jmethodID id = (*env)->GetStaticMethodID(env, frontend_c, "downloadingFile", "(Lorg/json/JSONObject;)V");
	(*env)->CallStaticVoidMethod(env, frontend_c, id, json);

	(*env)->PopLocalFrame(env, NULL);
}

void app_downloaded_file(const struct PtpObjectInfo *oi, const char *path) {
	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 5);
	plat_dbg("Downloaded file %s", path);

	jclass frontend_c = get_frontend_class(env);
	jmethodID id = (*env)->GetStaticMethodID(env, frontend_c, "downloadedFile", "(Ljava/lang/String;)V");
	(*env)->CallStaticVoidMethod(env, frontend_c, id, (*env)->NewStringUTF(env, path));
	(*env)->PopLocalFrame(env, NULL);
}

void app_print_id(int resid) {
	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 5);
	jmethodID id = (*env)->GetStaticMethodID(env, get_frontend_class(env), "print", "(I)V");
	(*env)->CallStaticVoidMethod(env, get_frontend_class(env), id, resid);
	(*env)->PopLocalFrame(env, NULL);
}

void app_print(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 5);
	jmethodID id = (*env)->GetStaticMethodID(env, get_frontend_class(env), "print", "(Ljava/lang/String;)V");
	jstring j_str = (*env)->NewStringUTF(env, buffer);
	(*env)->CallStaticVoidMethod(env, get_frontend_class(env), id, j_str);
	(*env)->PopLocalFrame(env, NULL);
}

void plat_dbg(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	__android_log_write(ANDROID_LOG_DEBUG, "plat_dbg", buffer);
}

void tester_log(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	ptp_verbose_log("%s\n", buffer);

	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 5);
	jmethodID method = (*env)->GetStaticMethodID(env, get_tester_class(env), "log", "(Ljava/lang/String;)V");
	jstring j_str = (*env)->NewStringUTF(env, buffer);
	(*env)->CallStaticVoidMethod(env, get_tester_class(env), method, j_str);
	(*env)->PopLocalFrame(env, NULL);
}

void tester_fail(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	ptp_verbose_log("%s\n", buffer);

	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 5);
	jmethodID method = (*env)->GetStaticMethodID(env, get_tester_class(env), "fail", "(Ljava/lang/String;)V");
	(*env)->CallStaticVoidMethod(env, get_tester_class(env), method, (*env)->NewStringUTF(env, buffer));
	(*env)->PopLocalFrame(env, NULL);
}

void app_send_cam_name(const char *name) {
	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 5);
	jstring j_str = (*env)->NewStringUTF(env, name);
	jclass f = get_frontend_class(env);
	jmethodID id = (*env)->GetStaticMethodID(env, f, "sendCamName", "(Ljava/lang/String;)V");
	(*env)->CallStaticVoidMethod(env, f, id, j_str);
	(*env)->PopLocalFrame(env, NULL);
}

void app_set_progress_bar(int status, int size) {
	backend.do_download = status;
	backend.download_size = size;
	backend.download_progress = 0;
	backend.last_percent = 0;
}

void app_increment_progress_bar(int read) {
	if (backend.do_download == 0) { return; }

	backend.download_progress += read;

	int n = (int)(((double)backend.download_progress) / (double)backend.download_size * 100.0);
	if (backend.last_percent != n) {
		if (n > 100) return;
		JNIEnv *env = get_jni_env();
		(*env)->PushLocalFrame(env, 5);
		jclass f = get_frontend_class(env);
		jmethodID id = (*env)->GetStaticMethodID(env, f, "notifyDownloadProgress", "(I)V");
		(*env)->CallStaticVoidMethod(env, f, id, n);
		(*env)->PopLocalFrame(env, NULL);
	}
	backend.last_percent = n;
}

void app_report_download_speed(long time, size_t size) {
	plat_dbg("Downloaded %lu in %lums", size, time);
	int mbps = (int)((size * 8) / (time));
	JNIEnv *env = get_jni_env();
	(*env)->PushLocalFrame(env, 5);
	jclass f = get_frontend_class(env);
	jmethodID id = (*env)->GetStaticMethodID(env, f, "notifyDownloadSpeed", "(I)V");
	(*env)->CallStaticVoidMethod(env, f, id, mbps);
	(*env)->PopLocalFrame(env, NULL);
}
