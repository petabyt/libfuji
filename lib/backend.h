#ifndef BACKEND_H
#define BACKEND_H

struct AndroidBackend {
    jobject pac; // Backend class
    jobject conn; // Conn class
    JNIEnv *env; // Java env, updated on every JNI call
    jobject main; // global thiz ref

	// Imported functions
    jmethodID jni_print;
    jmethodID cmd_read;
    jmethodID cmd_write;
    //jfieldID progress;

	// Global one connection runtime
    struct PtpRuntime r;

    // Version of Fuji's protocol (has different quirks)
    int function_version;
};

extern struct AndroidBackend backend;

// printf to UI
void jni_print(char *fmt, ...);

// printf to kernel
void android_err(char *fmt, ...);

#endif
