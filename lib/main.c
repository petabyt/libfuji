// Main backend for JNI/socket communication
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <camlib.h>

#include "myjni.h"
#include "fuji.h"
#include "backend.h"

struct AndroidBackend backend;

void reset_connection() {
    memset(&fuji_known, 0, sizeof(struct FujiDeviceKnowledge));
    ptp_generic_reset(&backend.r);
    backend.r.connection_type = PTP_IP;
}

void jni_verbose_log(char *str) {
    if (backend.log_fp == NULL) return;

    fputs(str, backend.log_fp);
    fputs("\n", backend.log_fp);
    fflush(backend.log_fp);
}

void ptp_verbose_log(char *fmt, ...) {
    if (backend.log_fp == NULL) return;

    va_list args;
    va_start(args, fmt);
    vfprintf(backend.log_fp, fmt, args);
    va_end(args);
    fflush(backend.log_fp);
}

void jni_print(char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // TODO: check use before init
    (*backend.env)->CallStaticVoidMethod(backend.env, backend.pac, backend.jni_print, (*backend.env)->NewStringUTF(backend.env, buffer));
}

void android_err(char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    __android_log_write(ANDROID_LOG_ERROR, "fujiapp-dbg", buffer);
}

JNI_FUNC(void, cInit)(JNIEnv *env, jobject thiz, jobject pac, jobject conn) {
    // Already zero-ed in BSS, but to be *extra* sure...
    memset(&backend, 0, sizeof(backend));

    backend.env = env;
    jclass thizClass = (*env)->GetObjectClass(env, thiz);

    jclass pacClass = (*env)->GetObjectClass(env, pac);

    backend.pac = (*env)->NewGlobalRef(env, pacClass);

    jclass connClass = (*env)->GetObjectClass(env, conn);
    backend.conn = (*env)->NewGlobalRef(env, conn);

    backend.main = (*env)->NewGlobalRef(env, thiz);

    backend.jni_print = (*backend.env)->GetStaticMethodID(backend.env, pacClass, "print", "(Ljava/lang/String;)V");

    backend.cmd_write = (*backend.env)->GetMethodID(backend.env, connClass, "write", "([B)I");
    backend.cmd_read = (*backend.env)->GetMethodID(backend.env, connClass, "read", "([BI)I");
    // TODO: cmd_close

    ptp_generic_init(&backend.r);
    reset_connection();
}

JNI_FUNC(jint, cRouteLogs)(JNIEnv *env, jobject thiz, jstring path) {
    backend.env = env;
    const char *req = (*env)->GetStringUTFChars(env, path, 0);
    if (req == NULL) return 1;

    FILE *f = fopen(req, "w");
    if (f == NULL) return 1;

    backend.log_fp = f;

    ptp_verbose_log("Fujiapp log file - Send this to devs!\n");
    ptp_verbose_log("ABI: %s\n", ABI);
    ptp_verbose_log("Compile date: %s\n", __DATE__);
    ptp_verbose_log("https://github.com/petabyt/fujiapp\n");

    return 0;
}

JNI_FUNC(void, cEndLogs)(JNIEnv *env, jobject thiz) {
    backend.env = env;

    if (backend.log_fp == NULL) return;

    fclose(backend.log_fp);

    backend.log_fp = NULL;
}
