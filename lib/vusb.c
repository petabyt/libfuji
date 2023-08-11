#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <android/log.h>
#include <jni.h>

#include <camlib.h>

#include "jni.h"
#include "fuji.h"
#include "backend.h"

#define _GPHOTO2_INTERNAL_CODE
#define _DARWIN_C_SOURCE
#include "vusb/config.h"
#include <gphoto2/gphoto2-port-library.h>
#include <vcamera.h>
#include <gphoto2/gphoto2-port.h>
#include <gphoto2/gphoto2-port-result.h>
#include <gphoto2/gphoto2-port-log.h>
#include <libgphoto2_port/i18n.h>

struct _GPPortPrivateLibrary {
	int	isopen;
	vcamera	*vcamera;
};

static GPPort *port = NULL;

uint8_t socket_init_resp[] = {0x44, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8, 0x70, 0xb0, 0x61, 0xa, 0x8b, 0x45, 0x93,
	0xb2, 0xe7, 0x93, 0x57, 0xdd, 0x36, 0xe0, 0x50, 0x58, 0x0, 0x2d, 0x0, 0x41, 0x0, 0x32, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, };

JNI_FUNC(jboolean, cIsUsingEmulator)(JNIEnv *env, jobject thiz) {
    backend.env = env;
    return 1;
}

int ptpip_connection_init() {
	android_err("Allocated vusb connection");
	port = malloc(sizeof(GPPort));
	C_MEM (port->pl = calloc (1, sizeof (GPPortPrivateLibrary)));
	port->pl->vcamera = vcamera_new(CANON_1300D);
	port->pl->vcamera->init(port->pl->vcamera);

	if (port->pl->isopen)
		return -1;

	port->pl->vcamera->open(port->pl->vcamera, port->settings.usb.port);
	port->pl->isopen = 1;
	return 0;
}

int ptpip_cmd_write(struct PtpRuntime *r, void *to, int length) {
	android_err("WRITE");
	static int first_write = 1;

	if (first_write) {
		struct FujiInitPacket *p = (struct FujiInitPacket *)to;
		android_err("vusb: init socket");
		first_write = 0;

		ptpip_connection_init();
		
		return length;
	}

	C_PARAMS (port && port->pl && port->pl->vcamera);
	return port->pl->vcamera->write(port->pl->vcamera, 0x02, (unsigned char *)to, length);
}

int ptpip_cmd_read(struct PtpRuntime *r, void *to, int length) {
	android_err("READ");
	static int left_of_init_packet = sizeof(socket_init_resp);

	if (left_of_init_packet) {
		memcpy(to, socket_init_resp + sizeof(socket_init_resp) - left_of_init_packet, length);
		left_of_init_packet -= length;
		android_err("Sent %d", length);
		return length;
	}

	C_PARAMS (port && port->pl && port->pl->vcamera);
	return port->pl->vcamera->read(port->pl->vcamera, 0x81, (unsigned char *)to, length);
}

int ptpip_event_send(struct PtpRuntime *r, void *data, int size) {
    return -1;
}

int ptpip_event_read(struct PtpRuntime *r, void *data, int size) {
    return -1;
}
