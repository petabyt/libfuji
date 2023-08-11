#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <android/log.h>
#include <jni.h>

#include <camlib.h>

#include "jni.h"
#include "fuji.h"
#include "backend.h"

JNI_FUNC(jboolean, cIsUsingEmulator)(JNIEnv *env, jobject thiz) {
    backend.env = env;
    return 0;
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

    // Sanity checks :)
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
