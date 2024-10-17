// NDK backend for camlib
// Copyright 2024 (c) Daniel C
// TODO: ptpusb_free_device_list
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <string.h>
#include <camlib.h>
#include "backend.h"
#include "app.h"
#include <android.h>

struct PrivUSB {
	jobject obj;
	int fd;
	int endpoint_in;
	int endpoint_out;
};

static struct PrivUSB *init_comm(struct PtpRuntime *r) {
	if (r->comm_backend == NULL) {
		r->comm_backend = calloc(1, sizeof(struct PrivUSB));
	}

	return (struct PrivUSB *)r->comm_backend;
}

static jobject get_usb_man(JNIEnv *env, jobject ctx) {
	jclass ClassContext = (*env)->FindClass(env, "android/content/Context");
	jfieldID lid_USB_SERVICE = (*env)->GetStaticFieldID(env, ClassContext, "USB_SERVICE", "Ljava/lang/String;");
	jobject USB_SERVICE = (*env)->GetStaticObjectField(env, ClassContext, lid_USB_SERVICE);

	jmethodID get_sys_service_m = (*env)->GetMethodID(env, ctx, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	return (*env)->CallObjectMethod(env, ctx, get_sys_service_m, USB_SERVICE);
}

static jobject get_ptp_interface(JNIEnv *env, jobject dev) {
	jclass ClassUsbInterface = (*env)->FindClass(env, "android/hardware/usb/UsbInterface");
	jmethodID get_dev_class_m = (*env)->GetMethodID(env, ClassUsbInterface, "getInterfaceClass", "()I");
	jclass ClassUsbDevice = (*env)->FindClass(env, "android/hardware/usb/UsbDevice");
	jmethodID MethodgetInterfaceCount = (*env)->GetMethodID(env, ClassUsbDevice, "getInterfaceCount", "()I");
	jmethodID MethodgetInterface = (*env)->GetMethodID(env, ClassUsbDevice, "getInterface", "(I)Landroid/hardware/usb/UsbInterface;");
	int ifaceCount = (*env)->CallIntMethod(env, dev, MethodgetInterfaceCount);
	for (int i = 0; i < ifaceCount; i++) {
		jobject interf = (*env)->CallObjectMethod(env, dev, MethodgetInterface, i);
		int interf_class = (*env)->CallIntMethod(env, interf, get_dev_class_m);
		if (interf_class == 6) {
			return interf;
		}
	}
	return NULL;
}

struct PtpDeviceEntry *ptpusb_device_list(struct PtpRuntime *r) {
	struct PtpDeviceEntry *curr_ent = malloc(sizeof(struct PtpDeviceEntry));
	memset(curr_ent, 0, sizeof(struct PtpDeviceEntry));

	struct PtpDeviceEntry *orig_ent = curr_ent;

	JNIEnv *env = get_jni_ctx();
	jobject ctx = get_jni_ctx();

	(*env)->PushLocalFrame(env, 100);
	jobject man = get_usb_man(env, ctx);

	jclass man_c = (*env)->FindClass(env, "android/hardware/usb/UsbManager");
	jmethodID get_dev_list_m = (*env)->GetMethodID(env, man_c, "getDeviceList", "()Ljava/util/HashMap;");
	jobject deviceList = (*env)->CallObjectMethod(env, man, get_dev_list_m);

	jclass hashmap_c = (*env)->FindClass(env, "java/util/HashMap");
	jmethodID values_m = (*env)->GetMethodID(env, hashmap_c, "values", "()Ljava/util/Collection;");
	jobject dev_list = (*env)->CallObjectMethod(env, deviceList, values_m);
	jclass collection_c = (*env)->FindClass(env, "java/util/Collection");
	jmethodID iterator_m = (*env)->GetMethodID(env, collection_c, "iterator", "()Ljava/util/Iterator;");
	jobject iterator = (*env)->CallObjectMethod(env, dev_list, iterator_m);
	jclass iterator_c = (*env)->FindClass(env, "java/util/Iterator");
	jmethodID has_next_m = (*env)->GetMethodID(env, iterator_c, "hasNext", "()Z" );
	jmethodID next_m = (*env)->GetMethodID(env, iterator_c, "next", "()Ljava/lang/Object;" );

	int valid_devices = 0;
	while ((*env)->CallBooleanMethod(env, iterator, has_next_m)) {
		jobject device = (*env)->CallObjectMethod(env, iterator, next_m);

		jclass usb_dev_c = (*env)->FindClass(env, "android/hardware/usb/UsbDevice" );
		jclass usb_interf_c = (*env)->FindClass(env, "android/hardware/usb/UsbInterface" );
		jclass usb_endpoint_c = (*env)->FindClass(env, "android/hardware/usb/UsbEndpoint" );
		jmethodID get_vendor_id_m = (*env)->GetMethodID(env, usb_dev_c, "getVendorId", "()I" );
		jmethodID get_product_id_m = (*env)->GetMethodID(env, usb_dev_c, "getProductId", "()I" );

		jmethodID get_endpoint_count_m = (*env)->GetMethodID(env, usb_interf_c, "getEndpointCount", "()I" );
		jmethodID get_endpoint_m = (*env)->GetMethodID(env, usb_interf_c, "getEndpoint", "(I)Landroid/hardware/usb/UsbEndpoint;" );

		jmethodID get_addr_m = (*env)->GetMethodID(env, usb_endpoint_c, "getAddress", "()I" );
		jmethodID get_type_m = (*env)->GetMethodID(env, usb_endpoint_c, "getType", "()I" );
		jmethodID get_dir_m = (*env)->GetMethodID(env, usb_endpoint_c, "getDirection", "()I" );
		jmethodID get_max_packet_size_m = (*env)->GetMethodID(env, usb_endpoint_c, "getMaxPacketSize", "()I" );

		if (valid_devices != 0) {
			struct PtpDeviceEntry *new_ent = malloc(sizeof(struct PtpDeviceEntry));
			memset(new_ent, 0, sizeof(struct PtpDeviceEntry));

			curr_ent->next = new_ent;
			new_ent->prev = curr_ent;

			curr_ent = new_ent;
		}

		jobject interf = get_ptp_interface(env, device);
		if (interf == NULL) {
			continue;
		}

		valid_devices++;

		curr_ent->vendor_id = (*env)->CallIntMethod(env, device, get_vendor_id_m);
		curr_ent->product_id = (*env)->CallIntMethod(env, device, get_product_id_m);
		curr_ent->id = -1;
		curr_ent->device_handle_ptr = (void *)(*env)->NewGlobalRef(env, device);

		int epCount = (*env)->CallIntMethod(env, interf, get_endpoint_count_m);
		for (int i = 0; i < epCount; i++) {
			jobject ep = (*env)->CallObjectMethod(env, interf, get_endpoint_m, i);
			int type = (*env)->CallIntMethod(env, ep, get_type_m);
			int dir = (*env)->CallIntMethod(env, ep, get_dir_m);
			int addr = (*env)->CallIntMethod(env, ep, get_addr_m);
			if (type == 2) {
				if (dir == 0x80) { // USB_DIR_IN
					curr_ent->endpoint_in = addr;
				} else if (dir == 0x0) {
					curr_ent->endpoint_out = addr;
				}
			}
		}
	}

	(*env)->PopLocalFrame(env, NULL);

	return orig_ent;
}

static int get_usb_permission(JNIEnv *env, jobject ctx, jobject man, jobject device) {
	(*env)->PushLocalFrame(env, 20);

	jclass man_c = (*env)->FindClass(env, "android/hardware/usb/UsbManager");

	jstring pkg = jni_get_package_name(env, ctx);
	jstring perm = jni_concat_strings2(env, pkg, ".USB_PERMISSION");

	jclass intent_c = (*env)->FindClass(env, "android/content/Intent");
	jmethodID intent_init = (*env)->GetMethodID(env, intent_c, "<init>", "(Ljava/lang/String;)V");
	jobject permission_intent = (*env)->NewObject(env, intent_c, intent_init, perm);

	jclass pending_intent_class = (*env)->FindClass(env, "android/app/PendingIntent");
	jmethodID pending_intent_get_broadcast = (*env)->GetStaticMethodID(env, pending_intent_class, "getBroadcast", "(Landroid/content/Context;ILandroid/content/Intent;I)Landroid/app/PendingIntent;");

	const jint FLAG_IMMUTABLE = 0x04000000;
	jobject pending_intent = (*env)->CallStaticObjectMethod(env, pending_intent_class, pending_intent_get_broadcast, ctx, 0, permission_intent, FLAG_IMMUTABLE);

	if (pending_intent == NULL) {
		(*env)->PopLocalFrame(env, NULL);
		return PTP_NO_PERM;
	}

	jmethodID req_perm_m = (*env)->GetMethodID(env, man_c, "requestPermission", "(Landroid/hardware/usb/UsbDevice;Landroid/app/PendingIntent;)V");
	(*env)->CallVoidMethod(env, man, req_perm_m, device, pending_intent);

	jmethodID has_perm_m = (*env)->GetMethodID(env, man_c, "hasPermission", "(Landroid/hardware/usb/UsbDevice;)Z");

	for (int i = 0; i < 10; i++) {
		if ((*env)->CallBooleanMethod(env, man, has_perm_m, device)) {
			(*env)->PopLocalFrame(env, NULL);
			return 0;
		}
		usleep(1000 * 1000);
	}

	(*env)->PopLocalFrame(env, NULL);
	return PTP_NO_PERM;
}

int ptp_device_open(struct PtpRuntime *r, struct PtpDeviceEntry *entry) {
	JNIEnv *env = get_jni_ctx();
	jobject ctx = get_jni_ctx();
	(*env)->PushLocalFrame(env, 20);

	jobject man = get_usb_man(env, ctx);
	jclass man_c = (*env)->FindClass(env, "android/hardware/usb/UsbManager");

	get_usb_permission(env, ctx, man, (jobject)entry->device_handle_ptr);

	jmethodID open_dev_m = (*env)->GetMethodID(env, man_c, "openDevice", "(Landroid/hardware/usb/UsbDevice;)Landroid/hardware/usb/UsbDeviceConnection;");
	jobject connection = (*env)->CallObjectMethod(env, man, open_dev_m, (jobject)entry->device_handle_ptr);
	if (connection == NULL) {
		(*env)->PopLocalFrame(env, NULL);
		return PTP_OPEN_FAIL;
	}

	struct PrivUSB *priv = init_comm(r);
	priv->obj = (*env)->NewGlobalRef(env, connection);

	jmethodID get_desc_m = (*env)->GetMethodID(env, (*env)->FindClass(env, "android/hardware/usb/UsbDeviceConnection"), "getFileDescriptor", "()I");
	priv->fd = (*env)->CallIntMethod(env, connection, get_desc_m);

	(*env)->PopLocalFrame(env, NULL);
	return 0;
}

int ptp_device_init(struct PtpRuntime *r) {
	// This can be a portable function in camlib/src/lib.c
	return -1;
}

int ptp_cmd_write(struct PtpRuntime *r, void *to, int length) {
	if (r->io_kill_switch) return -1;
	struct PrivUSB *priv = init_comm(r);
	struct usbdevfs_bulktransfer ctrl;
	ctrl.ep = priv->endpoint_out;
	ctrl.len = length;
	ctrl.data = to;
	ctrl.timeout = PTP_TIMEOUT;
	return ioctl(priv->fd, USBDEVFS_BULK, &ctrl);
}

int ptp_cmd_read(struct PtpRuntime *r, void *to, int length) {
	if (r->io_kill_switch) return -1;
	struct PrivUSB *priv = init_comm(r);
	struct usbdevfs_bulktransfer ctrl;
	ctrl.ep = priv->endpoint_in;
	ctrl.len = length;
	ctrl.data = to;
	ctrl.timeout = PTP_TIMEOUT;
	int rc = ioctl(priv->fd, USBDEVFS_BULK, &ctrl);

	if (rc > 0) app_increment_progress_bar(rc);

	return rc;
}

int ptp_device_close(struct PtpRuntime *r) {
	JNIEnv *env = get_jni_env();
	struct PrivUSB *priv = init_comm(r);
	jclass class = (*env)->GetObjectClass(env, priv->obj);
	jmethodID close = (*env)->GetMethodID(env, class, "close", "()V");
	(*env)->CallVoidMethod(env, priv->obj, close);
	(*env)->DeleteGlobalRef(env, priv->obj);
	return 0;
}

int ptp_device_reset(struct PtpRuntime *r) {
	return -1;
}

int ptp_read_int(struct PtpRuntime *r, void *to, int length) {
	return -1;
}
