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

void network_init() {
#ifdef WIN32
	// Windows wants to init this thread for socket stuff
	WSADATA wsaData = {0};
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
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

int app_bind_socket_wifi(int sockfd) {
	return 0;
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

int fudge_test_wifi(struct PtpRuntime *r) {
	network_init();
	int rc = 0;
	char *ip_addr = "0.0.0.0";
	
	r->connection_type = PTP_IP_USB;
	if (ptpip_connect(r, ip_addr, FUJI_CMD_IP_PORT)) {
		printf("Error connecting to %s:%d\n", ip_addr, FUJI_CMD_IP_PORT);
		return 0;
	}
	fuji_reset_ptp(r);
	r->connection_type = PTP_IP_USB;
	fuji_get(r)->transport = FUJI_FEATURE_WIRELESS_COMM;
	
	rc = fuji_test_setup(r);
	if (rc) return rc;
	
	rc = fuji_test_filesystem(r);
	if (rc) return rc;
	
	rc = ptp_close_session(r);
	if (rc) return rc;
	
	ptpip_close(r);

	return 0;
}

int fuji_discover_ask_connect(void *arg, struct DiscoverInfo *info) {
	// Ask if we want to connect?
	return 1;
}

int fuji_discovery_check_cancel(void *arg) {
	return 0;
}

int main(int argc, char **argv) {
	ptp = ptp_new(PTP_USB);

	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-tw")) {
			int rc = fudge_test_wifi(ptp);
			plat_dbg("Result: %d\n", rc);
			return rc;
		} else if (!strcmp(argv[i], "-tu")) {
			//return fudge_test(ptp);
		} else if (!strcmp(argv[i], "-d")) {
			fuji_test_discovery(ptp);
			return 1;
		}
	}

	return fudge_main_ui();
}
