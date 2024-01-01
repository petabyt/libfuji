#ifndef BACKEND_H
#define BACKEND_H

struct AndroidBackend {
	jobject main; // Backend class
	jobject conn; // Conn class
	JNIEnv *env; // Java env, updated on every JNI call

	// Imported functions
	jmethodID jni_print;
	jmethodID cmd_read;
	jmethodID cmd_write;
	jmethodID cmd_close;

	jobject cmd_buffer;
	jobject progress_bar;

	jobject tester; // Tester class
	jmethodID tester_log;
	jmethodID tester_fail;

	// Global one connection runtime
	struct PtpRuntime r;

	void *log_buf;
	size_t log_size;
	size_t log_pos;
};

extern struct AndroidBackend backend;

// Test suite verbose logging
void tester_log(char *fmt, ...);
void tester_fail(char *fmt, ...);

// printf to UI
void jni_print(char *fmt, ...);

// printf to kernel
void android_err(char *fmt, ...);

// Verbose print to log file
void jni_verbose_log(char *str);

void reset_connection();

#endif
