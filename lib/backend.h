#ifndef BACKEND_H
#define BACKEND_H

#include <jni.h>
#include <android/log.h>

#ifndef JNI_FUNC
#define JNI_FUNC(ret, name) JNIEXPORT ret JNICALL Java_dev_danielc_fujiapp_Backend_##name
#endif

#if defined(__arm__)
#if defined(__ARM_ARCH_7A__)
#if defined(__ARM_NEON__)
#if defined(__ARM_PCS_VFP)
#define ABI "armeabi-v7a/NEON (hard-float)"
#else
#define ABI "armeabi-v7a/NEON"
#endif
#else
#if defined(__ARM_PCS_VFP)
	#define ABI "armeabi-v7a (hard-float)"
	#else
	#define ABI "armeabi-v7a"
	#endif
#endif
#else
#define ABI "armeabi"
#endif
#elif defined(__i386__)
#define ABI "x86"
#elif defined(__x86_64__)
#define ABI "x86_64"
	#elif defined(__mips64)  /* mips64el-* toolchain defines __mips__ too */
	#define ABI "mips64"
	#elif defined(__mips__)
	#define ABI "mips"
	#elif defined(__aarch64__)
	#define ABI "arm64-v8a"
	#else
	#define ABI "unknown"
#endif

struct AndroidBackend {
	jobject main; // Backend global obj
	jmethodID jni_print;
	jmethodID send_text_m;

	jobject progress_bar;
	int download_progress;
	int download_size;

	jobject tester;
	jmethodID tester_log;
	jmethodID tester_fail;

	// Global one connection runtime
	struct PtpRuntime r;

	void *log_buf;
	size_t log_size;
	size_t log_pos;
};

extern struct AndroidBackend backend;

// Thread safe JNIEnv storage
void set_jni_env(JNIEnv *env);
JNIEnv *get_jni_env();

// Verbose print to log file
void jni_verbose_log(char *str);

int jni_setup_usb(JNIEnv *env, jobject obj);

static inline void app_increment_progress_bar(int read) {
	// Measures progress on all threads
	static int last_p = 0;

	backend.download_progress += read;

	int n = (((double)backend.download_progress) / (double)backend.download_size * 100.0);
	if (last_p != n) {
		if (n > 100) return;

		JNIEnv *env = get_jni_env();

		jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, backend.progress_bar), "setProgress", "(I)V");
		(*env)->CallVoidMethod(env, backend.progress_bar, method, n);
	}
	last_p = n;
}

#endif
