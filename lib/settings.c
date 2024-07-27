// Progress bar / IO monitoring backend
// Copyright 2024 by Daniel C (https://github.com/petabyt/camlib)
#include <camlib.h>
#include <time.h>
#include <string.h>
#include <dlfcn.h>
#include <android.h>
#include "app.h"
#include "backend.h"

#define SETTINGS_FUNC(ret, name) JNIEXPORT ret JNICALL Java_dev_danielc_fujiapp_SettingsActivity_##name

int app_do_connect_without_wifi() {
	JNIEnv *env = get_jni_env();
	return jni_get_pref_int(env, "connect_without_wifi", 0);
}

const char *app_get_client_name() {
	JNIEnv *env = get_jni_env();
	return jni_get_pref_str(env, "client_name", "Fudge");
}

const char *app_get_camera_ip() {
	JNIEnv *env = get_jni_env();
	return jni_get_pref_str(env, "ip_address", "192.168.0.1");
}

const char *app_get_wpa2_password() {
	JNIEnv *env = get_jni_env();
	return jni_get_pref_str(env, "wpa_password", "");
}

SETTINGS_FUNC(jobject, getWPA2Password)(JNIEnv *env, jobject thiz, jobject ctx) {
	set_jni_env_ctx(env, ctx);
	return (*env)->NewStringUTF(env, app_get_wpa2_password());
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
	// view_get_checked();
	jni_set_pref_int(env, setting_id, 1);
}

SETTINGS_FUNC(void, handleSettingsButtons)(JNIEnv *env, jobject thiz, jobject ctx) {
	set_jni_env_ctx(env, ctx);

	jobject view = view_get_by_id(env, ctx, "connect_without_wifi");
	view_add_native_checked_listener(env, view, (void *) on_selected_handler, 2, "connect_without_wifi", "connect_without_wifi");
	int x = app_do_connect_without_wifi();
	view_set_checked(env, view, x);

	view = view_get_by_id(env, ctx, "ip_address_text");
	view_add_native_input_listener(env, view, (void *) input_handler, 2, "ip_address_text", "ip_address");
	view_set_text(env, view, app_get_camera_ip());

	view = view_get_by_id(env, ctx, "wpa_password");
	view_add_native_input_listener(env, view, (void *) input_handler, 2, "wpa_password", "wpa_password");
	view_set_text(env, view, app_get_wpa2_password());

	view = view_get_by_id(env, ctx, "client_name");
	view_add_native_input_listener(env, view, (void *) input_handler, 2, "client_name", "client_name");
	view_set_text(env, view, app_get_wpa2_password());
}