#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <camlib.h>

#include "myjni.h"
#include "fuji.h"
#include "backend.h"

#define CMD_BUFFER_SIZE 512

typedef int (*_android_setsocknetwork_td)(jlong handle, int fd);

int ndk_network_init() {
	void *lib = dlopen("libandroid.so", RTLD_NOW);
	_android_setsocknetwork_td _android_setsocknetwork = dlsym(lib, "android_setsocknetwork");

	// todo
}

JNI_FUNC(jboolean, cSetProgressBar)(JNIEnv *env, jobject thiz, jobject pg) {
	if (pg == NULL) {
		(*env)->DeleteGlobalRef(env, backend.progress_bar);
		backend.progress_bar = NULL;
		return 0;
	}
	backend.progress_bar = (*env)->NewGlobalRef(env, pg);
	return 0;
}

JNI_FUNC(void, cClearKillSwitch)(JNIEnv *env, jobject thiz) {
	reset_connection();
	backend.r.io_kill_switch = 0;
}

int ptpip_cmd_close(struct PtpRuntime *r) {
	JNIEnv *env = get_jni_env();
	(*env)->CallVoidMethod(env, backend.conn, backend.cmd_close);
	return 0;
}

JNI_FUNC(void, cReportError)(JNIEnv *env, jobject thiz, jint code, jstring reason) {
	set_jni_env(env);

	const char *c_reason = (*env)->GetStringUTFChars(env, reason, 0);

	ptp_report_error(&backend.r, (char *)c_reason, (int)code);

	ptpip_cmd_close(&backend.r);
}

void ptp_report_error(struct PtpRuntime *r, char *reason, int code) {
	android_err("Kill switch %d\n", r->io_kill_switch);
	if (r->io_kill_switch) return;
	r->io_kill_switch = 1;

	if (reason == NULL) {
		if (code == PTP_IO_ERR) {
			app_print("Disconnected: IO Error");
		} else {
			app_print("Disconnected: Runtime error");
		}
	} else {
		app_print("Disconnected: %s", reason);
	}
}

int ptpip_cmd_write(struct PtpRuntime *r, void *to, int length) {
	if (r->io_kill_switch) return -1;
	if (length <= 0) {
		android_err("Length is less than 1");
		return -1;
	}

	JNIEnv *env = get_jni_env();

	int written = 0;
	while (written != length) {
		int max_len = length - written;
		if (max_len > CMD_BUFFER_SIZE) {
			max_len = CMD_BUFFER_SIZE;
		}

		(*env)->SetByteArrayRegion(env, backend.cmd_buffer, 0, max_len, (const jbyte *)(to) + written);

		int ret = (*env)->CallIntMethod(env, backend.conn, backend.cmd_write, max_len);
		if (ret < 0) {
			android_err("cmd_write failed: %d", ret);
			return -1;
		}

		written += ret;
	}

	return written;
}

int ptpip_cmd_read(struct PtpRuntime *r, void *to, int length) {
	if (r->io_kill_switch) return -1;
	if (length <= 0) {
		android_err("Length is less than 1");
		return -1;
	}

	if (length > 50000000) {
		android_err("Camera is trying to send too much data - breaking connection.");
		return -1;
	}

	JNIEnv *env = get_jni_env();

	int read = 0;
	while (read != length) {
		int max_len = length - read;
		if (max_len > CMD_BUFFER_SIZE) {
			max_len = CMD_BUFFER_SIZE;
		}

		int ret = (*env)->CallIntMethod(env, backend.conn, backend.cmd_read, max_len);
		if (ret < 0) {
			android_err("failed to receive packet, rc=%d (length=%d)", ret, length);
			return -1;
		}

		jbyte *bytes = (*env)->GetByteArrayElements(env, backend.cmd_buffer, 0);
		memcpy(to + read, bytes, ret);

		(*env)->ReleaseByteArrayElements(env, backend.cmd_buffer, bytes, 0);

		read += ret;

		if (backend.progress_bar != NULL) {
			static int last_p = 0;
			int n = (((double)read) / (double)length * 100.0);
			if (last_p != n) {
				jmethodID method = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, backend.progress_bar), "setProgress", "(I)V");
				(*env)->CallVoidMethod(env, backend.progress_bar, method, n);
			}
			last_p = n;
		}
	}

	return read;
}

int ptpip_event_send(struct PtpRuntime *r, void *data, int size) {
	return -1;
}

int ptpip_event_read(struct PtpRuntime *r, void *data, int size) {
	return -1;
}
