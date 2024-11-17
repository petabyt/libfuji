#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <poll.h>
	#include <arpa/inet.h>
#endif
#include <camlib.h>
#include <fuji.h>
#include <app.h>
#include <fujiptp.h>
#include "desktop.h"

void ptp_verbose_log(char *fmt, ...) {
	printf("PTP: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__ ((noreturn))
void ptp_panic(char *fmt, ...) {
	printf("PTP abort: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	abort();
}

void plat_dbg(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
}

char *app_get_client_name(void) {
	return strdup("desktop");
}

int app_check_thread_cancel(void) {
	// TODO: check signal
	return 0;
}

int fuji_discover_ask_connect(void *arg, struct DiscoverInfo *info) {
	// Ask if we want to connect?
	return 1;
}

int fuji_discovery_check_cancel(void *arg) {
	return 0;
}

int ptp_list_devices(void) {
	struct PtpRuntime *r = ptp_new(PTP_USB);

	struct PtpDeviceEntry *list = ptpusb_device_list(r);

	for (; list != NULL; list = list->next) {
		printf("product id: %04x\n", list->product_id);
		printf("vendor id: %04x\n", list->vendor_id);
		printf("Vendor friendly name: '%s'\n", list->manufacturer);
		printf("Model friendly name: '%s'\n", list->name);
	}

	return 0;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--list")) {
			return ptp_list_devices();
		} else if (!strcmp(argv[i], "--script")) {
			return fuji_connect_run_script(argv[i + 1]);
		} else if (!strcmp(argv[i], "--test-wifi")) {
			int rc = fudge_test_all_cameras();
			plat_dbg("Result: %d\n", rc);
			return rc;
		} else if (!strcmp(argv[i], "--test-usb")) {
			//return fudge_test(ptp);
		} else if (!strcmp(argv[i], "--test-discovery")) {
			fuji_test_discovery(ptp_new(PTP_USB));
			return 1;
		} else {
			plat_dbg("Invalid arg");
		}
	}

	return fudge_main_ui();
}
