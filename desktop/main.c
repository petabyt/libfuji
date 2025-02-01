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

void app_send_cam_name(const char *name) {

}

void fuji_discovery_update_progress(void *arg, enum DiscoverUpdateMessages progress) {
}

void app_downloading_file(const struct PtpObjectInfo *oi) {
	// ...
}

void app_downloaded_file(const struct PtpObjectInfo *oi, const char *path) {
	// ...
}

void app_increment_progress_bar(int read) {
	// ...
}

void app_report_download_speed(long time, size_t size) {
	// ...
}

void app_log_clear(void) {
	// ...
}

void app_print(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	printf("app_print: %s\n", buffer);	
}

void tester_log(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	printf("LOG: %s\n", buffer);
}

void tester_fail(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	printf("FAIL: %s\n", buffer);
}

void app_update_connected_status(int connected) {
	
}

static int help(void) {
	printf("Fudge 0.1.0\n");
	printf("Compilation date: " __DATE__ "\n");
	printf("  --list <device number>\n");
	printf("\tList all PTP devices connected to this computer\n");
	printf("  --dump-usb\n");
	printf("\tConnect to the first available Fuji camera and dump all information\n");
	printf("  --script <filename>\n");
	printf("\tExecute a Lua script using fudge bindings\n");
	return 0;
}

int fudge_ui_backend(void);

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
			if (i + 3 > argc) {
				printf("Invalid argument\n");
				return -1;
			}
			return fudge_process_raf(argv[i + 1], argv[i + 2], argv[i + 3]);
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

	//printf("TODO: Implement UI\n");

	fudge_ui_backend();

	return 0;
}
