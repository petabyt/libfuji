#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <string.h>
#include <jni.h>
#include <camlib.h>

#define PTP_FUNC(ret, name) JNIEXPORT ret JNICALL Java_camlib_CamlibBackend_##name

// TODO: Use comm_backend?
static struct PrivUSB {
	jobject obj;
	int fd;
}usb;

int jni_setup_usb(JNIEnv *env, jobject obj) {
	jclass class = (*env)->GetObjectClass(env, obj);
	jmethodID get_fd = (*env)->GetMethodID(env, class, "getFileDescriptor", "()I");

	jint fd = (*env)->CallIntMethod(env, obj, get_fd);
	usb.fd = (int)fd;
}

int ptp_device_init(struct PtpRuntime *r) {
	// Unimplemented
	return -1;
}

int ptp_cmd_write(struct PtpRuntime *r, void *to, int length) {
	struct usbdevfs_bulktransfer ctrl;
	ctrl.ep = 0x02;
	ctrl.len = length;
	ctrl.data = to;
	ctrl.timeout = PTP_TIMEOUT;
	return ioctl(usb.fd, USBDEVFS_BULK, &ctrl);
}

int ptp_cmd_read(struct PtpRuntime *r, void *to, int length) {
	struct usbdevfs_bulktransfer ctrl;
	ctrl.ep = 0x81;
	ctrl.len = length;
	ctrl.data = to;
	ctrl.timeout = PTP_TIMEOUT;
	return ioctl(usb.fd, USBDEVFS_BULK, &ctrl);
}

int ptp_device_close(struct PtpRuntime *r) {
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
