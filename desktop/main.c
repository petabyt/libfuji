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
#include <cl_stuff.h>
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

void ptp_error_log(char *fmt, ...) {
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
	puts("");
	fflush(stdout);
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

static int help(void) {
	printf("Fudge 0.1.0\n");
	printf("Compilation date: " __DATE__ "\n");
	printf("  --list <device number>        List all PTP devices connected to this computer\n");
	printf("  --dump-usb     Connect to the first available Fuji camera and dump all information\n");
	printf("  --script <filename>       Execute a Lua script using fudge bindings\n");
	return 0;
}

int main(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		// Typical camlib CLI stuff
		if (!strcmp(argv[i], "--list")) {
			return ptp_list_devices();
		} else if (!strcmp(argv[i], "--info")) {
			int dev_id = 0;
			if (i + 2 <= argc) dev_id = atoi(argv[i + 1]);
			return ptp_dump_device(dev_id);
		}

		if (!strcmp(argv[i], "--script")) {
			return fuji_connect_run_script(argv[i + 1]);
		} else if (!strcmp(argv[i], "--raw")) {
			return fudge_process_raf("/home/daniel/Pictures/DSCF2911.RAF", "output.jpg", NULL);
		}

		if (!strcmp(argv[i], "--test-wifi")) {
			int rc = fudge_test_all_cameras();
			plat_dbg("Result: %d\n", rc);
			return rc;
		} else if (!strcmp(argv[i], "--dump-usb")) {
			return fudge_dump_usb();
		} else if (!strcmp(argv[i], "--test-discovery")) {
			fuji_test_discovery(ptp_new(PTP_USB));
			return 1;
		} else if (!strcmp(argv[i], "--help")) {
			return help();
		} else if (!strcmp(argv[i], "--backup")) {

		} else {
			printf("Invalid arg '%s'\n", argv[i]);
			return -1;
		}
	}

	return fudge_main_ui();
}
