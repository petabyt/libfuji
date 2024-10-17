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
	jobject man = get_usb_man(env, ctx);

	(*env)->PushLocalFrame(env, 100);

	jclass man_c = (*env)->FindClass(env, "android/hardware/usb/UsbManager");
	jmethodID get_dev_list_m = (*env)->GetMethodID(env, man_c, "getDeviceList", "()Ljava/util/HashMap;");
	jobject deviceList = (*env)->CallObjectMethod(env, man, get_dev_list_m);

	//Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
	jclass ClassHashMap = (*env)->FindClass(env, "java/util/HashMap");
	jmethodID Methodvalues = (*env)->GetMethodID(env, ClassHashMap, "values", "()Ljava/util/Collection;");
	jobject deviceListCollection = (*env)->CallObjectMethod(env, deviceList, Methodvalues);
	jclass ClassCollection = (*env)->FindClass(env, "java/util/Collection");
	jmethodID Methoditerator = (*env)->GetMethodID(env, ClassCollection, "iterator", "()Ljava/util/Iterator;");
	jobject deviceListIterator = (*env)->CallObjectMethod(env, deviceListCollection, Methoditerator);
	jclass ClassIterator = (*env)->FindClass(env, "java/util/Iterator");

	//while (deviceIterator.hasNext())
	jmethodID MethodhasNext = (*env)->GetMethodID( env, ClassIterator, "hasNext", "()Z" );
	jmethodID Methodnext = (*env)->GetMethodID( env, ClassIterator, "next", "()Ljava/lang/Object;" );

	int valid_devices = 0;
	while ((*env)->CallBooleanMethod(env, deviceListIterator, MethodhasNext)) {
		jobject device = (*env)->CallObjectMethod( env, deviceListIterator, Methodnext);

		jclass usb_dev_c = (*env)->FindClass(env, "android/hardware/usb/UsbDevice" );
		jclass usb_interf_c = (*env)->FindClass(env, "android/hardware/usb/UsbInterface" );
		jclass usb_endpoint_c = (*env)->FindClass(env, "android/hardware/usb/UsbEndpoint" );
		jclass usb_conn_c = (*env)->FindClass(env, "android/hardware/usb/UsbDeviceConnection" );
		jmethodID get_dev_name_m = (*env)->GetMethodID(env, usb_dev_c, "getDeviceName", "()Ljava/lang/String;" );
		jmethodID get_vendor_id_m = (*env)->GetMethodID(env, usb_dev_c, "getVendorId", "()I" );
		jmethodID get_product_id_m = (*env)->GetMethodID(env, usb_dev_c, "getProductId", "()I" );

		jmethodID get_endpoint_count_m = (*env)->GetMethodID(env, usb_interf_c, "getEndpointCount", "()I" );
		jmethodID get_endpoint_m = (*env)->GetMethodID(env, usb_interf_c, "getEndpoint", "(I)Landroid/hardware/usb/UsbEndpoint;" );

		jmethodID MethodgetAddress = (*env)->GetMethodID(env, usb_endpoint_c, "getAddress", "()I" );
		jmethodID get_type_m = (*env)->GetMethodID(env, usb_endpoint_c, "getType", "()I" );
		jmethodID get_dir_m = (*env)->GetMethodID(env, usb_endpoint_c, "getDirection", "()I" );
		jmethodID MethodgetMaxPacketSize = (*env)->GetMethodID(env, usb_endpoint_c, "getMaxPacketSize", "()I" );

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
			int addr = (*env)->CallIntMethod(env, ep, MethodgetAddress);
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

int ptp_device_open(struct PtpRuntime *r, struct PtpDeviceEntry *entry) {
	JNIEnv *env = get_jni_ctx();
	jobject ctx = get_jni_ctx();
	(*env)->PushLocalFrame(env, 20);

	jobject man = get_usb_man(env, ctx);

	jclass man_c = (*env)->FindClass(env, "android/hardware/usb/UsbManager");
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

#if 0
JNI_FUNC(jint, cUSBConnectNative)(JNIEnv *env, jclass thiz, jobject usb) {
	set_jni_env(env);
	struct PtpRuntime *r = ptp_get();

	jni_setup_usb(env, usb);

	ptp_reset(&backend.r);
	backend.r.connection_type = PTP_USB;
	backend.r.max_packet_size = 512; // Android can go higher, but this will do

	fuji_reset_ptp(r);

	r->io_kill_switch = 0;
	strncpy(fuji_get(r)->ip_address, info->camera_ip, 64);
	strncpy(fuji_get(r)->autosave_client_name, info->client_name, 64);
	fuji_get(r)->transport = info->transport;
	memcpy(&fuji_get(r)->net, &info->h, sizeof(struct NetworkHandle));

	return 0;
}
#endif

int ptp_device_init(struct PtpRuntime *r) {
	// connection done by frontend
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
