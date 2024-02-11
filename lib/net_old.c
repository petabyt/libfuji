#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <camlib.h>
#include <time.h>
#include "app.h"
#include "fuji.h"
#include "backend.h"

#define CMD_BUFFER_SIZE 512

int ptpip_cmd_write_jni(struct PtpRuntime *r, void *to, int length) {
	if (r->io_kill_switch) return -1;
	if (length <= 0) {
		plat_dbg("Length is less than 1");
		return -1;
	}

	JNIEnv *env = get_jni_env();

	int written = 0;
	while (written != length) {
		int max_len = length - written;
		if (max_len > CMD_BUFFER_SIZE) {
			max_len = CMD_BUFFER_SIZE;
		}

		(*env)->SetByteArrayRegion(env, backend.wifi_cmd_buffer, 0, max_len, (const jbyte *)(to) + written);

		int ret = (*env)->CallIntMethod(env, backend.cmd_socket, backend.wifi_cmd_write, max_len);
		if (ret < 0) {
			plat_dbg("wifi_cmd_write failed: %d", ret);
			return -1;
		}

		written += ret;
	}

	return written;
}

int ptpip_cmd_read_jni(struct PtpRuntime *r, void *to, int length) {
	if (r->io_kill_switch) return -1;
	if (length <= 0) {
		plat_dbg("Length is less than 1");
		return -1;
	}

	JNIEnv *env = get_jni_env();

	int read = 0;
	while (read != length) {
		int max_len = length - read;
		if (max_len > CMD_BUFFER_SIZE) {
			max_len = CMD_BUFFER_SIZE;
		}

		int ret = (*env)->CallIntMethod(env, backend.cmd_socket, backend.wifi_cmd_read, max_len);
		if (ret < 0) {
			plat_dbg("failed to receive packet, rc=%d (length=%d)", ret, length);
			return -1;
		}

		jbyte *bytes = (*env)->GetByteArrayElements(env, backend.wifi_cmd_buffer, 0);
		memcpy(to + read, bytes, ret);

		(*env)->ReleaseByteArrayElements(env, backend.wifi_cmd_buffer, bytes, 0);

		read += ret;

		if (backend.progress_bar != NULL) {
			increment_progress_bar(env, ret);
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
