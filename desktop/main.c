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

static struct PtpRuntime *ptp;

struct PtpRuntime *ptp_get() {
	return ptp;
}

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

void *fudge_usb_connect(void *arg) {
	struct PtpRuntime *r = ptp_get();
	int rc;
	if (ptp_device_init(r)) {
		puts("Device connection error");
		return 0;
	}
	fuji_reset_ptp(r);

	app_print("Hello, World");

	pthread_exit(NULL);
	return NULL;
}

int fuji_discover_ask_connect(void *arg, struct DiscoverInfo *info) {
	// Ask if we want to connect?
	return 1;
}

int fuji_discovery_check_cancel(void *arg) {
	return 0;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--test-wifi")) {
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
