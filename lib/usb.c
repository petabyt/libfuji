#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <string.h>
#include <camlib.h>
#include "backend.h"
#include "app.h"
//#define PTP_FUNC(ret, name) JNIEXPORT ret JNICALL Java_camlib_CamlibBackend_##name

// TODO: Use comm_backend?
static struct PrivUSB {
	jobject obj;
	int fd;
}usb_priv;

int jni_setup_usb(JNIEnv *env, jobject obj) {
	jclass class = (*env)->GetObjectClass(env, obj);
	jmethodID get_fd = (*env)->GetMethodID(env, class, "getFileDescriptor", "()I");

	jint fd = (*env)->CallIntMethod(env, obj, get_fd);
	usb_priv.fd = (int)fd;
}

int jni_close_usb(JNIEnv *env) {
	jclass class = (*env)->GetObjectClass(env, usb_priv.obj);
	jmethodID close = (*env)->GetMethodID(env, class, "closeConnection", "()V");
	(*env)->CallVoidMethod(env, usb_priv.obj, close);
}

JNI_FUNC(jint, cUSBConnectNative)(JNIEnv *env, jobject thiz, jobject usb) {
	set_jni_env(env);

	jni_setup_usb(env, usb);

	ptp_reset(&backend.r);
	backend.r.connection_type = PTP_USB;
	backend.r.max_packet_size = 512;

	return 0;
}

int ptp_device_init(struct PtpRuntime *r) {
	// Unimplemented
	return -1;
}

int ptp_cmd_write(struct PtpRuntime *r, void *to, int length) {
	struct usbdevfs_bulktransfer ctrl;
	ctrl.ep = 0x01;
	ctrl.len = length;
	ctrl.data = to;
	ctrl.timeout = PTP_TIMEOUT;
	return ioctl(usb_priv.fd, USBDEVFS_BULK, &ctrl);
}

int ptp_cmd_read(struct PtpRuntime *r, void *to, int length) {
	struct usbdevfs_bulktransfer ctrl;
	ctrl.ep = 0x82;
	ctrl.len = length;
	ctrl.data = to;
	ctrl.timeout = PTP_TIMEOUT;
	int rc = ioctl(usb_priv.fd, USBDEVFS_BULK, &ctrl);
	return rc;
}

int ptp_device_close(struct PtpRuntime *r) {
	jni_close_usb(get_jni_env());
	return -1;
}

int ptp_device_reset(struct PtpRuntime *r) {
	return -1;
}

int ptp_read_int(struct PtpRuntime *r, void *to, int length) {
	return -1;
}

int reset_int() {
	return -1;
}
