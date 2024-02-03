#ifndef BACKEND_H
#define BACKEND_H

struct AndroidBackend {
	jobject main; // Backend class
	jobject conn; // Conn class

	// Imported functions
	jmethodID jni_print;
	jmethodID cmd_read;
	jmethodID cmd_write;
	jmethodID cmd_close;

	jobject cmd_buffer;
	jobject progress_bar;

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

#include "app.h"

// printf to kernel
void android_err(char *fmt, ...);

// Verbose print to log file
void jni_verbose_log(char *str);

void reset_connection();

// lib.c
int libu_write_file(JNIEnv *env, char *path, void *data, size_t length);

void set_jni_env(JNIEnv *env);
JNIEnv *get_jni_env();

#endif
