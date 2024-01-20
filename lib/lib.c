#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>

#define LIBU(ret, name) JNIEXPORT ret JNICALL Java_libui_LibU_##name

int libu_write_file(JNIEnv *env, char *path, void *data, size_t length) {
   	jclass class = (*env)->FindClass(env, "libui/LibU");

   	jmethodID method = (*env)->GetStaticMethodID(env, class, "writeFile", "(Ljava/lang/String;[B)V");

	jbyteArray jdata = (*env)->NewByteArray(env, length);
    (*env)->SetByteArrayRegion(
        env, jdata,
        0, length, (const jbyte *)(data)
    );

    jstring jpath = (*env)->NewStringUTF(env, path);

	(*env)->CallStaticVoidMethod(env, class, method,
        jpath, jdata
    );

    (*env)->DeleteLocalRef(env, jdata);
    (*env)->DeleteLocalRef(env, jpath);

    return 0;
}

// Added in POSIX 2008, not C standard
char *stpcpy(char *dst, const char *src) {
    const size_t len = strlen(src);
    return (char *)memcpy (dst, src, len + 1) + len;
}
