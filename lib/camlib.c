// Portable Java bindings to camlib
#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include <camlib.h>

#define PTP_FUNC(ret, name) JNIEXPORT ret JNICALL Java_dev_danielc_common_Camlib_##name

void set_jni_env_ctx(JNIEnv *env, jobject ctx);
JNIEnv *get_jni_env(void);
struct PtpRuntime *ptp_get(void);
jbyteArray jni_struct_to_bytearr(void *data, int length);
jobject jni_string_to_jsonobject(JNIEnv *env, const char *str);

PTP_FUNC(jbyteArray, cPtpGetThumb)(JNIEnv *env, jobject thiz, jint handle) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();

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

PTP_FUNC(jint, cPtpGetPropValue)(JNIEnv *env, jobject thiz, jint code) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();

	int rc = ptp_get_prop_value(r, code);
	if (rc < 0) {
		return rc;
	}

	return ptp_parse_prop_value(r);
}

PTP_FUNC(jint, cPtpOpenSession)(JNIEnv *env, jobject thiz) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();
	return ptp_open_session(r);
}

PTP_FUNC(jint, cPtpCloseSession)(JNIEnv *env, jobject thiz) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();
	return ptp_close_session(r);
}

PTP_FUNC(jstring, cGetObjectInfo)(JNIEnv *env, jobject thiz, jint handle) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();

	struct PtpObjectInfo oi;
	int rc = ptp_get_object_info(r, (int)handle, &oi);
	if (rc) {
		return NULL;
	}

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));

	jstring ret = (*env)->NewStringUTF(env, buffer);
	return ret;
}

static void object_discovered_callback(struct PtpRuntime *r, struct PtpObjectInfo *oi, void *arg) {
	JNIEnv *env = get_jni_env();
	jclass gallery_c = (*env)->FindClass(env, "dev/danielc/fujiapp/Gallery");
	jmethodID id = (*env)->GetStaticMethodID(env, gallery_c, "objectServiceUpdated", "([Lorg/json/JSONObject;)V");
	(*env)->CallStaticVoidMethod(env, gallery_c, id, NULL);
}

PTP_FUNC(void, cPtpObjectServiceStart)(JNIEnv *env, jclass clazz, jintArray handles) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();
	jsize length = (*env)->GetArrayLength(env, handles);
	jint *handles_n = (*env)->GetIntArrayElements(env, handles, NULL);

	r->oc = ptp_create_object_service(handles_n, length, object_discovered_callback, NULL);
}

jobjectArray jni_string_array_to_json_object_array(JNIEnv *env, const char **strArray, int count) {
	set_jni_env_ctx(env, NULL);
	jclass json_object_class = (*env)->FindClass(env, "org/json/JSONObject");
	jmethodID json_object_constructor = (*env)->GetMethodID(env, json_object_class, "<init>", "(Ljava/lang/String;)V");

	jobjectArray json_object_array = (*env)->NewObjectArray(env, count, json_object_class, NULL);

	for (int i = 0; i < count; i++) {
		jstring json_string = (*env)->NewStringUTF(env, strArray[i]);
		jobject json_object = (*env)->NewObject(env, json_object_class, json_object_constructor, json_string);
		(*env)->SetObjectArrayElement(env, json_object_array, i, json_object);

		(*env)->DeleteLocalRef(env, json_string);
		(*env)->DeleteLocalRef(env, json_object);
	}

	return json_object_array;
}

PTP_FUNC(jobject, cPtpObjectServiceGetFilled)(JNIEnv *env, jclass clazz) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();

	int count = ptp_object_service_length(r, r->oc);

	jclass json_object_class = (*env)->FindClass(env, "org/json/JSONObject");
	jobjectArray json_object_array = (*env)->NewObjectArray(env, count, json_object_class, NULL);

	for (int i = 0; i < count; i++) {
		struct PtpObjectInfo *oi = ptp_object_service_get_index(r, r->oc, i);
		if (oi == NULL) abort();

		char buffer[1024];
		ptp_object_info_json(oi, buffer, sizeof(buffer));

		jobject jo = jni_string_to_jsonobject(env, buffer);

		(*env)->SetObjectArrayElement(env, json_object_array, i, jo);
	}

	return json_object_array;
}

PTP_FUNC(jobject, cPtpObjectServiceGet)(JNIEnv *env, jclass clazz, jint handle) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();
	struct PtpObjectInfo *oi = ptp_object_service_get(r, r->oc, (int)handle);

	char buffer[1024];
	ptp_object_info_json(oi, buffer, sizeof(buffer));

	return jni_string_to_jsonobject(env, buffer);
}

PTP_FUNC(jobject, cPtpObjectServiceGetIndex)(JNIEnv *env, jclass clazz, jint index) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();
	struct PtpObjectInfo *oi = ptp_object_service_get_index(r, r->oc, (int)index);

	char buffer[1024];
	ptp_object_info_json(oi, buffer, sizeof(buffer));

	return jni_string_to_jsonobject(env, buffer);
}

PTP_FUNC(jint, cPtpObjectServiceStep)(JNIEnv *env, jclass clazz) {
	set_jni_env_ctx(env, NULL);
	struct PtpRuntime *r = ptp_get();
	return ptp_object_service_step(r, r->oc);
}

PTP_FUNC(void, cPtpObjectServiceAddPriority)(JNIEnv *env, jclass clazz, jint handle) {
	set_jni_env_ctx(env, NULL);
	// TODO: implement cPtpObjectServiceAddPriority()
}