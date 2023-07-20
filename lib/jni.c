// JNI PTP/IP interface for camlib and fuji.c
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <jni.h>
#include <camlib.h>

#include "jni.h"
#include "backend.h"
#include "fuji.h"

JNI_FUNC(jint, cPtpFujiInit)(JNIEnv *env, jobject thiz) {
    backend.env = env;
    int rc = ptpip_fuji_init(&backend.r, "fujiapp");

    struct PtpFujiInitResp resp;
    ptp_fuji_get_init_info(&backend.r, &resp);

    jni_print("Connecting to %s\n", resp.cam_name);

    return rc;
}

JNI_FUNC(jstring, cPtpRun)(JNIEnv *env, jobject thiz, jstring string) {
    backend.env = env;
    const char *req = (*env)->GetStringUTFChars(env, string, 0);

    char *buffer = malloc(PTP_BIND_DEFAULT_SIZE);

    int r = bind_run(&backend.r, (char *)req, buffer, PTP_BIND_DEFAULT_SIZE);

    if (r == -1) {
        return (*env)->NewStringUTF(env, "{\"error\": -1}");
    }

    jstring ret = (*env)->NewStringUTF(env, buffer);
    free(buffer);
    return ret;
}

JNI_FUNC(jint, cPtpFujiWaitUnlocked)(JNIEnv *env, jobject thiz) {
    backend.env = env;
    return ptpip_fuji_wait_unlocked(&backend.r);
}

JNI_FUNC(jint, cPtpFujiPing)(JNIEnv *env, jobject thiz) {
    backend.env = env;
    return ptpip_fuji_get_events(&backend.r);
}

JNI_FUNC(jbyteArray, cPtpGetThumb)(JNIEnv *env, jobject thiz, jint handle) {
    backend.env = env;
    int rc = ptp_get_thumbnail(&backend.r, (int)handle);
    if (rc) {
        return NULL;
    }

    jbyteArray ret = (*env)->NewByteArray(env, ptp_get_payload_length(&backend.r));
    (*env)->SetByteArrayRegion(env, ret, 0, ptp_get_payload_length(&backend.r), (const jbyte *)(ptp_get_payload(&backend.r)));
    return ret;
}

JNI_FUNC(jint, cPtpGetPropValue)(JNIEnv *env, jobject thiz, jint code) {
    backend.env = env;

    int rc = ptp_get_prop_value(&backend.r, code);
    if (rc < 0) {
        return rc;
    }

    return ptp_parse_prop_value(&backend.r);
}

JNI_FUNC(jbyteArray, cFujiGetFile)(JNIEnv *env, jobject thiz, jint handle) {
    backend.env = env;

    // Set the compression prop (allows full images to go through, otherwise puts
    // extra data in ObjectInfo and cuts off image downloads)
    int rc = ptp_set_prop_value(&backend.r, PTP_PC_FUJI_Compression, 1);
    if (rc) {
        return NULL;
    }

    int max = backend.r.data_length;

    struct PtpObjectInfo oi;
    rc = ptp_get_object_info(&backend.r, (int)handle, &oi);
    if (rc) {
        return NULL;
    }

    jbyteArray ret = (*env)->NewByteArray(env, oi.compressed_size);

    // Makes sure to set the compression prop back to 0 after finished
    // (extra data won't go through for some reason)
    int read = 0;
    while (1) {
        rc = ptp_get_partial_object(&backend.r, handle, read, max);
        if (rc) {
            ptp_set_prop_value(&backend.r, PTP_PC_FUJI_Compression, 0);
            return NULL;
        }

        if (ptp_get_payload_length(&backend.r) == 0) {
            ptp_set_prop_value(&backend.r, PTP_PC_FUJI_Compression, 0);
            return NULL;
        }

        (*env)->SetByteArrayRegion(env, ret, read, ptp_get_payload_length(&backend.r), (const jbyte *)(ptp_get_payload(&backend.r)));

        read += ptp_get_payload_length(&backend.r);

        if (read >= oi.compressed_size) {
            ptp_set_prop_value(&backend.r, PTP_PC_FUJI_Compression, 0);
            return ret;
        }
    }
}

// NOTE: cFujiConfigInitMode *must* be called before cFujiConfigVersion, or anything else.
// If not, it will break up the connection and destroy packets for any file operation.
JNI_FUNC(jint, cFujiConfigInitMode)(JNIEnv *env, jobject thiz) {
    backend.env = env;

    int rc = fuji_config_init_mode(&backend.r);

    return rc;
}

JNI_FUNC(jint, cFujiConfigVersion)(JNIEnv *env, jobject thiz) {
    backend.env = env;

    int rc = fuji_config_version(&backend.r);
    if (rc) return rc;

    rc = fuji_config_remote_photo_viewer(&backend.r);
    if (rc) return rc;

    return 0;
}
