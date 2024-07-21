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

struct AndroidBackend backend;

__thread struct AndroidLocal local = {0, 0};

void set_jni_env_ctx(JNIEnv *env, jobject ctx) {
	//plat_dbg("Setting env/ctx %d, %d: %d", local.env, local.ctx, gettid());
	local.env = env;
	local.ctx = ctx;
}

struct AndroidLocal push_jni_env_ctx(JNIEnv *env, jobject ctx) {
	struct AndroidLocal l;
	l.env = local.env;
	l.ctx = local.ctx;
	//plat_dbg("env: %u, ctx: %u, tid: %d", local.env, local.ctx, gettid());
	local.env = env;
	local.ctx = ctx;
	return l;
}

void pop_jni_env_ctx(struct AndroidLocal l) {
	set_jni_env_ctx(l.env, l.ctx);
}

JNIEnv *get_jni_env() {
	if (local.env == NULL) {
		plat_dbg("JNIEnv not set for this thread");
		abort();
	}

	return local.env;
}

jobject get_jni_ctx() {
	if (local.ctx == NULL) {
		plat_dbg("ctx not set for this thread");
		abort();
	}

	return local.ctx;
}

struct PtpRuntime *ptp_get() {
	return &backend.r;
}

struct PtpRuntime *luaptp_get_runtime() {
	return ptp_get();
}

void ptp_report_error(struct PtpRuntime *r, const char *reason, int code) {
	plat_dbg("Kill switch: %d tid: %d\n", r->io_kill_switch, gettid());
	if (r->io_kill_switch) return;

	// Safely disconnect if intentional
	if (code == 0) {
		plat_dbg("Closing session");
		ptp_close_session(r);
	}

	r->io_kill_switch = 1;

	if (r->connection_type == PTP_IP_USB) {
		ptpip_close(r);
	} else if (r->connection_type == PTP_USB) {
		ptp_device_close(r);
	}

	fuji_reset_ptp(r);

	if (reason == NULL) {
		if (code == PTP_IO_ERR) {
			app_print("Disconnected: IO Error");
		} else {
			app_print("Disconnected: Runtime error");
		}
	} else {
		app_print("Disconnected: %s", reason);
	}
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

#ifdef VERBOSE
	if (backend.log_buf == NULL) return;
#endif

	char buffer2[512];
	snprintf(buffer2, sizeof(buffer2), "%d\t%s", (int)clock() * 1000 / CLOCKS_PER_SEC, buffer);

	if (strlen(buffer2) + backend.log_pos + 1 > backend.log_size) {
		backend.log_buf = realloc(backend.log_buf, strlen(buffer2) + backend.log_pos + 1);
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

void app_print_id(int resid) {
	JNIEnv *env = get_jni_env();
	jclass class = (*env)->FindClass(env, "dev/danielc/fujiapp/Backend");
	jmethodID id = (*env)->GetStaticMethodID(env, class, "print", "(I)V");
	(*env)->CallStaticVoidMethod(env, backend.main, backend.jni_print, resid);
}

void app_print(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	JNIEnv *env = get_jni_env();
	jstring j_str = (*env)->NewStringUTF(env, buffer);
	(*env)->CallStaticVoidMethod(env, backend.main, backend.jni_print, j_str);
	(*env)->DeleteLocalRef(env, j_str);
}

void plat_dbg(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	__android_log_write(ANDROID_LOG_ERROR, "fudge", buffer);
}

void tester_log(char *fmt, ...) {
	if (backend.tester_log == NULL) return;
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	ptp_verbose_log("%s\n", buffer);

	JNIEnv *env = get_jni_env();
	jstring j_str = (*env)->NewStringUTF(env, buffer);
	(*env)->CallVoidMethod(env, backend.tester, backend.tester_log, j_str);
	(*env)->DeleteLocalRef(env, j_str);
}

void tester_fail(char *fmt, ...) {
	if (backend.tester_fail == NULL) return;
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	ptp_verbose_log("%s\n", buffer);

	JNIEnv *env = get_jni_env();
	(*env)->CallVoidMethod(env, backend.tester, backend.tester_fail, (*env)->NewStringUTF(env, buffer));
}

void ui_send_text(char *key, char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	JNIEnv *env = get_jni_env();
	jstring j_str = (*env)->NewStringUTF(env, buffer);
	jstring j_key = (*env)->NewStringUTF(env, key);
	(*env)->CallStaticVoidMethod(env, backend.main, backend.send_text_m, j_key, j_str);
	(*env)->DeleteLocalRef(env, j_str);
	(*env)->DeleteLocalRef(env, j_key);
}

JNI_FUNC(void, cInit)(JNIEnv *env, jobject thiz) {
	// On init, all members in backend are guaranteed to be NULL
	memset(&backend, 0, sizeof(backend));

	set_jni_env(env);

	backend.main = (*env)->NewGlobalRef(env, thiz);

	backend.jni_print = (*env)->GetStaticMethodID(env, backend.main, "print", "(Ljava/lang/String;)V");
	backend.send_text_m = (*env)->GetStaticMethodID(env, backend.main, "sendTextUpdate", "(Ljava/lang/String;Ljava/lang/String;)V");

	ptp_init(&backend.r);
	fuji_reset_ptp(&backend.r);
}

JNI_FUNC(void, cTesterInit)(JNIEnv *env, jobject thiz, jobject tester) {
	set_jni_env(env);

	jclass testerClass = (*env)->GetObjectClass(env, tester);
	backend.tester = (*env)->NewGlobalRef(env, tester);

	backend.tester_log = (*env)->GetMethodID(env, testerClass, "log", "(Ljava/lang/String;)V");
	backend.tester_fail = (*env)->GetMethodID(env, testerClass, "fail", "(Ljava/lang/String;)V");
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
