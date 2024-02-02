#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <jni.h>
#include <android/log.h>
#error "no"
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

void *libu_get_assets_file(JNIEnv *env, jobject ctx, char *filename, int *length) {
	jclass class = (*env)->FindClass(env, "libui/LibU");

	jmethodID method = (*env)->GetStaticMethodID(env, class, "readFileFromAssets", "(Landroid/content/Context;Ljava/lang/String;)[B");

	jstring jfile = (*env)->NewStringUTF(env, filename);

	jbyteArray array = (*env)->CallStaticObjectMethod(env, class, method,
		ctx, jfile
	);

	jbyte *bytes = (*env)->GetByteArrayElements(env, array, 0);

	(*length) = (*env)->GetArrayLength(env, array);

	void *new = malloc(*length);
	memcpy(new, bytes, *length);

	(*env)->DeleteLocalRef(env, jfile);
	(*env)->ReleaseByteArrayElements(env, array, bytes, 0);

	return new;
}

void *libu_get_txt_file(JNIEnv *env, jobject ctx, char *filename) {
	int length = 0;
	char *bytes = libu_get_assets_file(env, ctx, filename, &length);
	length += 1;
	bytes = realloc(bytes, length);
	bytes[length] = '\0';
	return bytes;
}

// Added in POSIX 2008, not C standard
char *stpcpy(char *dst, const char *src) {
	const size_t len = strlen(src);
	return (char *)memcpy (dst, src, len + 1) + len;
}
