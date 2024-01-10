#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <android/log.h>
#include <stdlib.h>
#include <stdarg.h>
#include <jni.h>

#include "ui.h"
#include "ui_android.h"

/*
TODO: activity intent to itself + key = native screens? Sounds like a stupid idea
*/

static jobject get_view_by_name_id(const char *id) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, uilib.class, "getView", "(Ljava/lang/String;)Landroid/view/View;");
	jobject view = (*env)->CallObjectMethod(env, uilib.class, method,
		(*env)->NewStringUTF(env, id)
	);

	return view;
}

void uiSwitchScreen(uiControl *content, const char *title) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetStaticMethodID(env, uilib.class, "switchScreen", "(Landroid/view/View;Ljava/lang/String;)V");
	(*env)->CallStaticVoidMethod(env, uilib.class, method,
		((struct uiAndroidControl *)content)->o, (*env)->NewStringUTF(env, title)
	);
}

static void ctx_set_content_view(jobject view) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, uilib.ctx), "setContentView", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, uilib.ctx, method,
		view
	);
}

void uiAndroidSetContent(uiControl *c) {
	ctx_set_content_view(((uiAndroidControl *)c)->o);
}

static void view_set_view_enabled(jobject view, int b) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setEnabled", "(Z)V");
	(*env)->CallVoidMethod(env, view, method, b);

	if ((*env)->ExceptionCheck(env)) {
		// ???
		return;
	}
}

void uiMultilineEntrySetReadOnly(uiMultilineEntry *e, int readonly) {
	view_set_view_enabled(((uiAndroidControl *)e)->o, readonly);
}

char *uiMultilineEntryText(uiMultilineEntry *e) {
	// GetText returns 'Editable'/CharSequence , toString
}

static void view_set_visibility(jobject view, int v) {
	// 0 = visible
	// 4 = invisible
	// 8 = gone
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setVisibility", "(I)V");
	(*env)->CallVoidMethod(env, view, method, v);
}

static void view_set_dimensions(jobject view, int w, int h) {
	JNIEnv *env = uilib.env;

	jmethodID method = (*env)->GetMethodID(env,
		(*env)->GetObjectClass(env, view),
		"getLayoutParams", "()Landroid/view/ViewGroup$LayoutParams;");
	jobject obj = (*env)->CallObjectMethod(env, view, method);

	jclass *class = (*env)->GetObjectClass(env, obj);
	jfieldID width_f = (*env)->GetFieldID(env, class, "width", "I");
	jfieldID height_f = (*env)->GetFieldID(env, class, "height", "I");

	if (w != 0)
		(*env)->SetIntField(env, obj, width_f, w);

	if (h != 0)
		(*env)->SetIntField(env, obj, height_f, h);
}

static void view_set_layout(jobject view, int x, int y) {
	JNIEnv *env = uilib.env;

	jclass class = (*env)->FindClass(env, "android/widget/LinearLayout$LayoutParams");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(II)V");
	jobject obj = (*env)->NewObject(env, class, constructor, x, y);

	jmethodID method = (*env)->GetMethodID(env,
		(*env)->GetObjectClass(env, view),
		"setLayoutParams", "(Landroid/view/ViewGroup$LayoutParams;)V");
	(*env)->CallVoidMethod(env, view, method, obj);
}

static void view_set_padding_px(jobject obj, int padding) {
	int p = 10 * padding;
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, obj), "setPadding", "(IIII)V");
	(*env)->CallVoidMethod(env, obj, method, p, p, p, p);
	
}

void uiBoxSetPadded(uiBox *b, int padded) {
	view_set_padding_px(b->c.o, padded);
}

void uiFormSetPadded(uiForm *f, int padded) {
	view_set_padding_px(f->c.o, padded);
}

static inline struct uiAndroidControl *new_view_control(int signature) {
	struct uiAndroidControl *b = calloc(1, sizeof(struct uiAndroidControl));
	b->c.Signature = signature;
	b->c.Disable = uiControlDisable;
	b->c.Enable = uiControlEnable;
	return b;
}

static uiBox *new_uibox(int type) {
	struct uiAndroidControl *b = new_view_control(uiBoxSignature);
	jobject box = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.layout_m,
		type
	);
	b->o = box;

	return (uiBox *)b;
}

static void view_set_view_text(jobject view, const char *text) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, view), "setText", "(Ljava/lang/CharSequence;)V");
	(*env)->CallVoidMethod(env, view, method, (*uilib.env)->NewStringUTF(uilib.env, text));
}

static struct uiAndroidControl *view_new_separator() {
	struct uiAndroidControl *c = new_view_control(uiSeparatorSignature);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "android/widget/Space");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uilib.ctx);

	c->request_height = 50;

	c->o = obj;
	return c;
}

uiSeparator *uiNewHorizontalSeparator(void) {
	return (uiSeparator *)view_new_separator();
}

uiSeparator *uiNewVerticalSeparator(void) {
	return (uiSeparator *)view_new_separator();
}

void uiControlEnable(uiControl *c) {
	jobject obj = ((struct uiAndroidControl *)c)->o;
	view_set_view_enabled(obj, 1);
}

void uiControlDisable(uiControl *c) {
	jobject obj = ((struct uiAndroidControl *)c)->o;
	view_set_view_enabled(obj, 0);
}

uiButton *uiNewButton(const char *text) {
	struct uiAndroidControl *b = new_view_control(uiButtonSignature);
	jobject button = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.button_m,
		(*uilib.env)->NewStringUTF(uilib.env, text)
	);
	b->o = button;
	return (uiButton *)b;
}

uiLabel *uiNewLabel(const char *text) {
	struct uiAndroidControl *l = new_view_control(uiLabelSignature);
	jobject label = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.label_m,
		(*uilib.env)->NewStringUTF(uilib.env, text)
	);
	l->o = label;
	return (uiLabel *)l;
}

uiBox *uiNewVerticalBox() {
	return new_uibox(1);
}

uiBox *uiNewHorizontalBox() {
	return new_uibox(0);
}

uiTab *uiNewTab() {
	struct uiAndroidControl *t = new_view_control(uiTabSignature);

	jobject tab = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.tab_layout_m
	);
	t->o = tab;

	return (uiTab *)t;
}

uiMultilineEntry *uiNewMultilineEntry() {
	struct uiAndroidControl *c = new_view_control(uiMultilineEntrySignature);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "android/widget/EditText");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uilib.ctx);

	c->o = obj;
	return (uiMultilineEntry *)c;
}

uiEntry *uiNewEntry() {
	struct uiAndroidControl *c = new_view_control(uiEntrySignature);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "android/widget/EditText");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uilib.ctx);

	view_set_layout(obj, ANDROID_LAYOUT_MATCH_PARENT, ANDROID_LAYOUT_WRAP_CONTENT);

	c->o = obj;
	return (uiEntry *)c;
}

uiProgressBar *uiNewProgressBar() {
	struct uiAndroidControl *c = new_view_control(uiProgressBarSignature);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "android/widget/ProgressBar");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Landroid/content/Context;Landroid/util/AttributeSet;I)V");
	jobject obj = (*env)->NewObject(env, class, constructor, uilib.ctx, NULL, ANDROID_progressBarStyleHorizontal);
	c->o = obj;
	return (uiProgressBar *)c;
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar) {
	struct uiAndroidControl *c = new_view_control(uiWindowSignature);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "libui/LibUI$Popup");
	jmethodID constructor = (*env)->GetMethodID(env, class, "<init>", "(Ljava/lang/String;I)V");
	jobject obj = (*env)->NewObject(env, class, constructor, (*uilib.env)->NewStringUTF(uilib.env, title), 0);
	c->o = obj;

	return (uiWindow *)c;
}

uiGroup *uiNewGroup(const char *title) {
	struct uiAndroidControl *f = new_view_control(uiGroupSignature);

	jobject form = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.form_m,
		(*uilib.env)->NewStringUTF(uilib.env, title)
	);
	f->o = form;

	return (uiGroup *)f;
}

void uiGroupSetMargined(uiGroup *g, int margined) {}

void uiGroupSetChild(uiGroup *g, uiControl *c) {
	uiBoxAppend((uiBox *)g, c, 0);
}

uiForm *uiNewForm() {
	return (uiForm *)uiNewVerticalBox();	
}

void uiFormAppend(uiForm *f, const char *label, uiControl *c, int stretchy) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.form_add_m,
		f->c.o, (*uilib.env)->NewStringUTF(uilib.env, label), ((uiAndroidControl *)c)->o
	);
}

void uiButtonSetText(uiButton *b, const char *text) {
	view_set_view_text(b->c.o, text);
}

void uiLabelSetText(uiLabel *l, const char *text) {
	view_set_view_text(l->c.o, text);	
}


void uiEntrySetText(uiEntry *e, const char *text) {
	view_set_view_text(e->c.o, text);	
}

void uiWindowSetChild(uiWindow *w, uiControl *child) {
	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "libui/LibUI$Popup");
	jmethodID m_set_child = (*env)->GetMethodID(env, class, "setChild", "(Landroid/view/View;)V");
	(*env)->CallVoidMethod(env, w->c.o, m_set_child, ((struct uiAndroidControl *)child)->o);
}

void uiProgressBarSetValue(uiProgressBar *p, int n) {
	JNIEnv *env = uilib.env;
	jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, p->c.o), "setProgress", "(I)V");
	(*env)->CallVoidMethod(env, p->c.o, method, n);
}

void uiBoxAppend(uiBox *b, uiControl *child, int stretchy) {
	struct uiAndroidControl *ctl = (struct uiAndroidControl *)child;
	ctl->o = (*uilib.env)->NewGlobalRef(uilib.env, ctl->o);

	JNIEnv *env = uilib.env;
	jclass class = (*env)->FindClass(env, "android/view/ViewGroup");
	jmethodID add_view = (*env)->GetMethodID(env, class, "addView", "(Landroid/view/View;)V");

	(*env)->CallVoidMethod(env, b->c.o, add_view,
		((struct uiAndroidControl *)child)->o
	);


	// Controls can optionally request to be set a certain width (only can be set after appending)
	if (ctl->request_width) view_set_dimensions(ctl->o, ctl->request_width, 0);
	if (ctl->request_height) view_set_dimensions(ctl->o, 0, ctl->request_height);
}

void uiButtonOnClicked(uiButton *b, void (*f)(uiButton *sender, void *senderData), void *data) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.set_click_m,
		b->c.o, (uintptr_t)f, (uintptr_t)b, (uintptr_t)data
	);
}

void uiQueueMain(void (*f)(void *data), void *data) {
	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.add_runnable_m,
		(uintptr_t)f, data, 0
	);
}

void uiTabAppend(uiTab *t, const char *name, uiControl *c) {
	((uiAndroidControl *)c)->o = (*uilib.env)->NewGlobalRef(uilib.env, ((uiAndroidControl *)c)->o);

	(*uilib.env)->CallStaticVoidMethod(
		uilib.env, uilib.class, uilib.add_tab_m,
		t->c.o, (*uilib.env)->NewStringUTF(uilib.env, name), ((uiAndroidControl *)c)->o
	);
}

void uiToast(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[256];

    vsnprintf(buffer, sizeof(buffer), format, args);
    (*uilib.env)->CallStaticVoidMethod(
        uilib.env, uilib.class, uilib.toast_m,
        (*uilib.env)->NewStringUTF(uilib.env, buffer)
    );

    va_end(args);
}

const char *uiGet(const char *id) {
	jstring *ret = (*uilib.env)->CallStaticObjectMethod(
		uilib.env, uilib.class, uilib.get_string_m,
		(*uilib.env)->NewStringUTF(uilib.env, id)
	);

	const char *c_string = (*uilib.env)->GetStringUTFChars(uilib.env, ret, 0);

	return c_string;
}

int uiAndroidInit(JNIEnv *env, jobject context) {
	uilib.pid = getpid();
	uilib.env = env;
	uilib.ctx = context;

	jclass class = (*env)->FindClass(env, "libui/LibUI");
	uilib.class = (*env)->NewGlobalRef(env, class);

	// TODO: Handle exception

	jfieldID ctx_f = (*env)->GetStaticFieldID(env, class, "ctx", "Landroid/content/Context;");
	(*env)->SetStaticObjectField(env, class, ctx_f, context);

	jfieldID action_bar_f = (*env)->GetStaticFieldID(env, class, "actionBar", "Landroidx/appcompat/app/ActionBar;");

	uilib.button_m = (*env)->GetStaticMethodID(env, class, "button", "(Ljava/lang/String;)Landroid/view/View;");
	uilib.label_m = (*env)->GetStaticMethodID(env, class, "label", "(Ljava/lang/String;)Landroid/view/View;");
	uilib.form_m = (*env)->GetStaticMethodID(env, class, "form", "(Ljava/lang/String;)Landroid/view/View;");
	uilib.layout_m = (*env)->GetStaticMethodID(env, class, "linearLayout", "(I)Landroid/view/ViewGroup;");
	uilib.tab_layout_m = (*env)->GetStaticMethodID(env, class, "tabLayout", "()Landroid/view/View;");

	uilib.add_tab_m = (*env)->GetStaticMethodID(env, class, "addTab", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");
	uilib.form_add_m = (*env)->GetStaticMethodID(env, class, "formAppend", "(Landroid/view/View;Ljava/lang/String;Landroid/view/View;)V");
	uilib.toast_m = (*env)->GetStaticMethodID(env, class, "toast", "(Ljava/lang/String;)V");
	uilib.set_click_m = (*env)->GetStaticMethodID(env, class, "setClickListener", "(Landroid/view/View;JJJ)V");
	uilib.get_string_m = (*env)->GetStaticMethodID(env, class, "getString", "(Ljava/lang/String;)Ljava/lang/String;");

	uilib.add_runnable_m = (*env)->GetStaticMethodID(env, class, "runRunnable", "(JJJ)V");
	
	return 0;
}

uiBox *uiAndroidBox(JNIEnv *env, jobject context, jobject parent) {
	struct uiBox *box = malloc(sizeof(struct uiBox));
	uiAndroidInit(env, context);
	box->c.o = parent;
	return box;
}

#define LIBUI(ret, name) JNIEXPORT ret JNICALL Java_libui_LibUI_##name

LIBUI(void, callFunction)(JNIEnv *env, jobject thiz, uintptr_t ptr, uintptr_t arg1, uintptr_t arg2) {

	// TODO: jlong == long long breaks ptr hack, need to store pointer data in struct -> jbytearray
	
	void (*ptr_f)(uintptr_t, uintptr_t) = (void *)ptr;
	ptr_f((uintptr_t)arg1, (uintptr_t)arg2);
}
