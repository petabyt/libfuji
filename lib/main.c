// Init routines
// Copyright 2023 (c) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <camlib.h>

#include "myjni.h"
#include "fuji.h"
#include "backend.h"

struct AndroidBackend backend;

// This will be put in a __emutls_t.* variable
// It's up to the compiler to decide how to implement it
__thread JNIEnv *backend_env = NULL;

void set_jni_env(JNIEnv *env) {
	backend_env = env;
}

JNIEnv *get_jni_env() {
	// Compiles to GCC/clang __emutls_get_address
	return backend_env;
}

struct PtpRuntime *ptp_get() {
	return &backend.r;
}

struct PtpRuntime *luaptp_get_runtime() {
	return ptp_get();
}

void reset_connection() {
	memset(&fuji_known, 0, sizeof(struct FujiDeviceKnowledge));
	ptp_generic_reset(&backend.r);
	backend.r.connection_type = PTP_IP_USB;
}

void ptp_verbose_log(char *fmt, ...) {
	if (backend.log_buf == NULL) return;

	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	//__android_log_write(ANDROID_LOG_ERROR, "ptp-verbose", buffer);

	if (strlen(buffer) + backend.log_pos + 1 > backend.log_size) {
		backend.log_buf = realloc(backend.log_buf, strlen(buffer) + backend.log_pos + 1);
	}

	strcpy(backend.log_buf + backend.log_pos, buffer);
	backend.log_pos += strlen(buffer);
}

void ptp_panic(char *fmt, ...) {
	// TODO: abort()
}

void app_print(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	JNIEnv *env = get_jni_env();
	(*env)->CallStaticVoidMethod(env, backend.main, backend.jni_print, (*env)->NewStringUTF(env, buffer));
}

void android_err(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	__android_log_write(ANDROID_LOG_ERROR, "fujiapp-dbg", buffer);
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
	(*env)->CallVoidMethod(env, backend.tester, backend.tester_log, (*env)->NewStringUTF(env, buffer));
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

JNI_FUNC(void, cInit)(JNIEnv *env, jobject thiz, jobject pac, jobject conn) {
	// On init, all members in backend are garunteed to be NULL
	memset(&backend, 0, sizeof(backend));

	set_jni_env(env);

	backend.main = (*env)->NewGlobalRef(env, thiz);
	backend.conn = (*env)->NewGlobalRef(env, conn);

	backend.jni_print = (*env)->GetStaticMethodID(env, backend.main, "print", "(Ljava/lang/String;)V");

	jfieldID soc_f = (*env)->GetStaticFieldID(env, backend.main, "cmdSocket", "Lcamlib/SimpleSocket;");
	jobject soc_o = (*env)->GetStaticObjectField(env, backend.main, soc_f);
	jclass soc_class = (*env)->GetObjectClass(env, soc_o);
	backend.conn = (*env)->NewGlobalRef(env, soc_o);

	backend.cmd_write = (*env)->GetMethodID(env, soc_class, "write", "(I)I");
	backend.cmd_read = (*env)->GetMethodID(env, soc_class, "read", "(I)I");
	backend.cmd_close = (*env)->GetMethodID(env, soc_class, "close", "()V");

	// Get socket IO buffer
	jmethodID get_buffer_m = (*env)->GetMethodID(env, soc_class, "getBuffer", "()Ljava/lang/Object;");
	jobject buffer = (*env)->CallObjectMethod(env, soc_o, get_buffer_m);
	backend.cmd_buffer = (*env)->NewGlobalRef(env, buffer);

	ptp_init(&backend.r);
	reset_connection();
}

// Init commands 
JNI_FUNC(void, cTesterInit)(JNIEnv *env, jobject thiz, jobject tester) {
	set_jni_env(env);

	//jclass thizClass = (*env)->GetObjectClass(env, thiz);
	jclass testerClass = (*env)->GetObjectClass(env, tester);
	backend.tester = (*env)->NewGlobalRef(env, tester);

	backend.tester_log = (*env)->GetMethodID(env, testerClass, "log", "(Ljava/lang/String;)V");
	backend.tester_fail = (*env)->GetMethodID(env, testerClass, "fail", "(Ljava/lang/String;)V");
}

JNI_FUNC(jint, cRouteLogs)(JNIEnv *env, jobject thiz, jstring path) {
	set_jni_env(env);
	const char *req = (*env)->GetStringUTFChars(env, path, 0);
	if (req == NULL) return 1;

	backend.log_buf = malloc(1000);
	backend.log_size = 1000;
	backend.log_pos = 0;

	strcpy(backend.log_buf, "Fujiapp log file - Send this to devs!\n");

	ptp_verbose_log("ABI: %s\n", ABI);
	ptp_verbose_log("Compile date: %s\n", __DATE__);
	ptp_verbose_log("https://github.com/petabyt/fujiapp\n");
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
