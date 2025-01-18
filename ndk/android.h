#ifndef UIFW_ANDROID_H
#define UIFW_ANDROID_H

#include <jni.h>

struct AndroidLocal {
	JNIEnv *env;
	jobject ctx;
};

struct AndroidLocal push_jni_env_ctx(JNIEnv *env, jobject ctx);
void pop_jni_env_ctx(struct AndroidLocal l);
void set_jni_env_ctx(JNIEnv *env, jobject ctx);
JNIEnv *get_jni_env(void);
jobject get_jni_ctx(void);

// https://android.googlesource.com/platform/frameworks/base/+/2ca2c87/core/res/res/values/public.xml
#define ANDROID_progressBarStyleHorizontal 0x01010078

#define ANDROID_LAYOUT_MATCH_PARENT 0xffffffff
#define ANDROID_LAYOUT_WRAP_CONTENT 0xfffffffe

#define ANDROID_simple_spinner_dropdown_item 0x01090009
#define ANDROID_simple_spinner_item 0x01090008

#define ANDROID_MODE_PRIVATE 0x0

#define ANDROID_GRAVITY_CENTER 0x00000011

enum AndroidViewVisibilities {
	ANDROID_VIEW_VISIBLE = 0,
	ANDROID_VIEW_INVISIBLE = 4,
	ANDROID_VIEW_GONE = 9,
};

/// @brief ctx.getPackageName()
jstring jni_get_package_name(JNIEnv *env, jobject context);
/// @brief ctx.getLayoutInflater()
jobject jni_get_layout_inflater(JNIEnv *env, jobject context);
jobject jni_get_resources(JNIEnv *env, jobject context);
jobject jni_get_theme(JNIEnv *env, jobject context);
/// @brief Concatenate 2 JNI strings
jstring jni_concat_strings2(JNIEnv *env, jstring a, jstring b);
/// @brief Concatenate 3 JNI strings
jstring jni_concat_strings3(JNIEnv *env, jstring a, jstring b, jstring c);
jobject jni_get_drawable(JNIEnv *env, jobject ctx, int resid);
jobject jni_get_display_metrics(JNIEnv *env, jobject ctx);
jobject jni_get_main_looper(JNIEnv *env);
jobject jni_get_handler(JNIEnv *env);
jobject jni_activity_get_root_view(JNIEnv *env, jobject ctx);
jobject jni_get_application_ctx(JNIEnv *env);

void *jni_get_assets_file(JNIEnv *env, jobject ctx, const char *filename, int *length);

/// Creates a new PopupWindow of full-screen height (?)
jobject popupwindow_new(JNIEnv *env, jobject ctx, int drawable_id);
/// Sets the content view of the PopupWindow
void popupwindow_set_content(JNIEnv *env, jobject popup, jobject view);
/// Make the PopupWindow visible.
void popupwindow_open(JNIEnv *env, jobject ctx, jobject popup);

/// @memberof pref
jint jni_get_pref_int(JNIEnv *env, const char *key, jint default_val);
/// @note Returns strdup'd string
char *jni_get_pref_str(JNIEnv *env, const char *key, const char *default_val);
void jni_set_pref_str(JNIEnv *env, const char *key, const char *str);
void jni_set_pref_int(JNIEnv *env, const char *key, int x);
/// @returns 1 if preference exists
jboolean jni_check_pref(JNIEnv *env, const char *key);
/// @note Returns strdup'd string
char *jni_get_string(JNIEnv *env, jobject ctx, const char *id);
/// @brief Get string resource ID from R.strings.
int jni_get_string_id(JNIEnv *env, jobject ctx, const char *id);

jobject popup_new(JNIEnv *env, jobject ctx, int drawable_id);

/// View.getContext()
jobject view_get_context(JNIEnv *env, jobject view);
/// View.setChecked()
void view_set_checked(JNIEnv *env, jobject view, jboolean checked);
/// View.isChecked()
jboolean view_get_checked(JNIEnv *env, jobject view);
/// View.setTextSize()
void view_set_text_size(JNIEnv *env, jobject obj, float size);
/// Get resource ID from key/name. For example. key=id name=hello == R.id.hello
jint view_get_res_id(JNIEnv *env, jobject ctx, const char *key, const char *name);
/// (View)findViewById(R.id.<id>)
jobject view_get_by_id(JNIEnv *env, jobject ctx, const char *id);
void view_set_visibility(JNIEnv *env, jobject view, int v);
void view_set_dimensions(JNIEnv *env, jobject view, int w, int h);
void view_set_layout(JNIEnv *env, jobject view, int x, int y);
void view_set_padding_px(JNIEnv *env, jobject obj, int padding);
void view_set_focusable(JNIEnv *env, jobject obj);
char *view_get_text(JNIEnv *env, jobject view);
void view_set_text(JNIEnv *env, jobject view, const char *text);
jobject view_new_linearlayout(JNIEnv *env, jobject ctx, int is_vertical, int x, int y);
void view_set_button_style(JNIEnv *env, jobject ctx, jobject button, jint bg_res);
jobject view_new_button(JNIEnv *env, jobject ctx);
/// Create a ScrollView
jobject view_new_scroll(JNIEnv *env, jobject ctx);
jobject view_new_space(JNIEnv *env, jobject ctx);
jobject view_new_textview(JNIEnv *env, jobject ctx);
/// Get the adapter of a combobox. Will create an ArrayAdapter if it's not set. Returns the adapter.
jobject combobox_get_adapter(JNIEnv *env, jobject ctx, jobject view);
/// Get resource ID of a R.drawable.<name>
jobject get_drawable_id(JNIEnv *env, jobject ctx, const char *name);
/// Inflate a view/viewgroup from name
jobject view_expand(JNIEnv *env, jobject ctx, const char *name);
/// Create an old (deprecated) TabHost
jobject view_new_tabhost(JNIEnv *env, jobject ctx);
/// Add a tab to the TabHost - string title only.
void view_tabhost_add(JNIEnv *env, const char *name, jobject tabhost, jobject child);
/// Append a view to a ViewGroup
void viewgroup_addview(JNIEnv *env, jobject group, jobject child);
void jni_toast(JNIEnv *env, jobject ctx, const char *string);
/// setContentView(R.layout.<name>)
void ctx_set_content_layout(JNIEnv *env, jobject ctx, const char *name);
/// setContentView(view)
void ctx_set_content_view(JNIEnv *env, jobject ctx, jobject view);

// callback.c
// Callback and listener API
void view_add_native_click_listener(JNIEnv *env, jobject view, void *fn, int argc, void *arg1, void *arg2);
void jni_native_runnable(JNIEnv *env, jobject ctx, void *fn, int argc, void *arg1, void *arg2);
void view_add_native_select_listener(JNIEnv *env, jobject view, void *fn, int argc, void *arg1, void *arg2);
void view_add_native_checked_listener(JNIEnv *env, jobject view, void *fn, int argc, void *arg1, void *arg2);
void view_add_native_input_listener(JNIEnv *env, jobject view, void *fn, int argc, void *arg1, void *arg2);

typedef int activity_callback(JNIEnv *env, jobject ctx);
int jni_start_native_activity(JNIEnv *env, jobject from_ctx, activity_callback *oncreate, activity_callback *ondestroy);

// Env/Context setter
// unused void ui_android_set_env_ctx(JNIEnv *env, jobject ctx);

void jni_set_action_bar(JNIEnv *env, jobject ctx, int id);
int jni_action_bar_set_home_icon(JNIEnv *env, jobject ctx, int drawable_id);

#endif
