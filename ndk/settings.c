// Progress bar / IO monitoring backend
// Copyright 2024 by Daniel C (https://github.com/petabyt/camlib)
#include <camlib.h>
#include <time.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "android.h"
#include "app.h"
#include "backend.h"

//#define SETTINGS_FUNC(ret, name) JNIEXPORT ret JNICALL Java_dev_danielc_fujiapp_SettingsActivity_##name

char *app_get_client_name(void) {
	return strdup("Fudge");
}
char *app_get_camera_ip(void) {
	return strdup("192.168.0.1");
}

//int app_do_connect_without_wifi(void) {
//	JNIEnv *env = get_jni_env();
//	return jni_get_pref_int(env, "connect_without_wifi", 0);
//}
#if 0
char *app_get_client_name(void) {
	JNIEnv *env = get_jni_env();
	char *s = jni_get_pref_str(env, "client_name", "Fudge");
	size_t l = strlen(s);
	if (l == 0 || l > 25) {
		return strdup("Fudge");
	}
	return s;
}

char *app_get_camera_ip(void) {
	JNIEnv *env = get_jni_env();
	char *s = jni_get_pref_str(env, "ip_address", "192.168.0.1");
	size_t l = strlen(s);
	if (l == 0 || l > 25) {
		free(s);
		return strdup("192.168.0.1");
	}
	return s;
}

char *app_get_wpa2_password(void) {
	JNIEnv *env = get_jni_env();
	char *s = jni_get_pref_str(env, "wpa_password", "");
	size_t l = strlen(s);
	if (l == 0 || l > 25) {
		free(s);
		return strdup("");
	}
	return s;
}

SETTINGS_FUNC(jobject, getWPA2Password)(JNIEnv *env, jclass thiz, jobject ctx) {
	set_jni_env_ctx(env, ctx);
	char *x = app_get_wpa2_password();
	jstring js = (*env)->NewStringUTF(env, x);
	free(x);
	return js;
}

static void input_handler(const char *view_id, const char *setting_id) {
	JNIEnv *env = get_jni_env();
	jobject ctx = get_jni_ctx();
	jobject x = view_get_by_id(env, ctx, view_id);
	char *str = view_get_text(env, x);
	if (strlen(str) > 64) {
		return;
	}
	jni_set_pref_str(env, setting_id, str);
}

static void on_selected_handler(const char *view_id, const char *setting_id) {
	JNIEnv *env = get_jni_env();
	jobject ctx = get_jni_ctx();
	jobject view = view_get_by_id(env, ctx, view_id);
	int x = (int)view_get_checked(env, view);
	jni_set_pref_int(env, setting_id, x);
}

SETTINGS_FUNC(void, handleSettingsButtons)(JNIEnv *env, jobject thiz, jobject ctx) {
	set_jni_env_ctx(env, ctx);
	int x;
	jobject view;

//	jobject view = view_get_by_id(env, ctx, "connect_without_wifi");
//	view_add_native_checked_listener(env, view, (void *) on_selected_handler, 2, "connect_without_wifi", "connect_without_wifi");
//	int x = app_do_connect_without_wifi();
//	view_set_checked(env, view, x);

	view = view_get_by_id(env, ctx, "ip_address_text");
	view_add_native_input_listener(env, view, (void *)input_handler, 2, "ip_address_text", "ip_address");
	view_set_text(env, view, app_get_camera_ip());

	view = view_get_by_id(env, ctx, "wpa_password");
	view_add_native_input_listener(env, view, (void *)input_handler, 2, "wpa_password", "wpa_password");
	view_set_text(env, view, app_get_wpa2_password());

	view = view_get_by_id(env, ctx, "client_name");
	view_add_native_input_listener(env, view, (void *) input_handler, 2, "client_name", "client_name");
	view_set_text(env, view, app_get_client_name());
}
#endif