// Main backend for JNI/socket communication
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <android/log.h>
#include <jni.h>

#include <camlib.h>

#include "jni.h"
#include "backend.h"

struct AndroidBackend backend;

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
    // For good measure
    memset(&backend, 0, sizeof(backend));

    backend.env = env;
    jclass thizClass = (*env)->GetObjectClass(env, thiz);
    jclass pacClass = (*env)->GetObjectClass(env, pac);
    backend.pac = (*env)->NewGlobalRef(env, pacClass);

    jclass connClass = (*env)->GetObjectClass(env, conn);
    backend.conn = (*env)->NewGlobalRef(env, connClass);

    backend.main = (*env)->NewGlobalRef(env, thiz);

    backend.jni_print = (*backend.env)->GetStaticMethodID(backend.env, pacClass, "jni_print", "(Ljava/lang/String;)V");
    backend.cmd_write = (*backend.env)->GetStaticMethodID(backend.env, connClass, "write", "([B)I");
    backend.cmd_read = (*backend.env)->GetStaticMethodID(backend.env, connClass, "read", "([BI)I");
    //backend.progress = (*backend.env)->GetStaticFieldID(backend.env, pacClass, "transferProgress", "I");

    ptp_generic_init(&backend.r);
    backend.r.connection_type = PTP_IP;
}

int ptpip_cmd_write(struct PtpRuntime *r, void *to, int length) {
    jbyteArray data = (*backend.env)->NewByteArray(backend.env, length);
    (*backend.env)->SetByteArrayRegion(backend.env, data, 0, length, (const jbyte *)(to));

    int ret = (*backend.env)->CallStaticIntMethod(backend.env, backend.conn, backend.cmd_write, data);

    if (ret < 0) {
        // TODO: debug
        return ret;
    }

    return ret;
}

int ptpip_cmd_read(struct PtpRuntime *r, void *to, int length) {
    if (length <= 0) {
        return PTP_IO_ERR;
    }

    if (length > 50000000) {
        jni_print("Camera is trying to send too much data - breaking connection.\n");
        return -1;
    }

    jbyteArray data = (*backend.env)->NewByteArray(backend.env, length);

    int ret = (*backend.env)->CallStaticIntMethod(backend.env, backend.conn, backend.cmd_read, data, length);

    if (ret < 0) {
        android_err("failed to recieve packet, %d", ret);
        return ret;
    }

    jbyte *bytes = (*backend.env)->GetByteArrayElements(backend.env, data, 0);
    memcpy(to, bytes, ret);

    (*backend.env)->DeleteLocalRef(backend.env, data);
    return ret;
}

int ptpip_event_send(struct PtpRuntime *r, void *data, int size) {
    return -1;
}

int ptpip_event_read(struct PtpRuntime *r, void *data, int size) {
    return -1;
}
