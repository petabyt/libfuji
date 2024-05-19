// Test basic opcode, get device properties
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <camlib.h>
#include <fuji.h>
#include <app.h>
#include <fujiptp.h>
#include "desktop.h"

static struct PtpRuntime *ptp;

struct PtpRuntime *ptp_get() {
	return ptp;
}

int fuji_test_filesystem(struct PtpRuntime *r);
int fuji_test_setup(struct PtpRuntime *r);

void ptp_report_error(struct PtpRuntime *r, const char *reason, int code) {
	plat_dbg("Kill switch: %d tid: %d\n", r->io_kill_switch, getpid());
	if (r->io_kill_switch) return;
	r->io_kill_switch = 1;

	if (r->connection_type == PTP_IP_USB) {
		ptpip_close(r);
	} else if (r->connection_type == PTP_USB) {
		ptp_close(r);
	}

	fuji_reset_ptp(r);

	if (reason == NULL) {
		if (code == PTP_IO_ERR) {
			app_print("Disconnected: IO Error");
		} else {
			app_print("Disconnected: Runtime error");
		}
	} else {
		app_print("Disconnected: %s", reason);
	}
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

int fudge_test(struct PtpRuntime *r) {
	if (ptp_device_init(r)) {
		puts("Device connection error");
		return 0;
	}

	int rc;

	ptp_open_session(r);

	struct PtpPropDesc pd;
	rc = ptp_get_prop_desc(r, 0xd20b, &pd);

	printf("Prop data type: %d\n", ptp_get_payload_length(r));

	rc = ptp_get_prop_value(r, 0xD21c);
	if (rc) return rc;

	printf("Prop code: %x\n", ptp_parse_prop_value(r));

	return 0;
}

void *fudge_backup_test(void *arg) {
	struct PtpRuntime *r = ptp_get();
	int rc;
	if (ptp_device_init(r)) {
		puts("Device connection error");
		return 0;
	}

	/*
	For some bizarre reason, Fuji X Acquire ended up doing something like:
	struct PtpDeviceInfo di;
	for (int i = 0; i < 20; i++) {
		rc = ptp_get_device_info(r, &di);
		rc = ptp_get_prop_value(r, 0xd20b);
	}
	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(r, 0, &oi);
	rc = ptp_get_object_info(r, 0, &oi);
	Before running GetObject. I'm guessing they have a background thread running GetDeviceInfo
	and GetPropValue 0xd20b and then I must have actually downloaded my settings a while after.
	*/

	// struct PtpDeviceInfo di;
	// for (int i = 0; i < 20; i++) {
	// 	rc = ptp_get_device_info(r, &di);
	// 	rc = ptp_get_prop_value(r, 0xd20b);
	// }

	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(r, 0, &oi);
	rc = ptp_get_object_info(r, 0, &oi);

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));
	app_print(buffer);

	app_print("Hello, WOrld\n");

	rc = ptp_get_object(r, 0);
	app_print("Downloaded payload %d bytes\n", ptp_get_payload_length(r));

	pthread_exit(NULL);
	return NULL;
}

int fudge_test_wifi(struct PtpRuntime *r) {
	int rc = 0;
	char *ip_addr = "0.0.0.0";
	
	r->connection_type = PTP_IP_USB;
	if (ptpip_connect(r, ip_addr, FUJI_CMD_IP_PORT)) {
		printf("Error connecting to %s:%d\n", ip_addr, FUJI_CMD_IP_PORT);
		return 0;
	}
	
	rc = fuji_test_setup(r);
	if (rc) return rc;
	
	rc = fuji_test_filesystem(r);
	if (rc) return rc;
	
	ptp_close_session(r);
	
	ptpip_close(r);

	return 0;
}

int main(int argc, char **argv) {
	ptp = ptp_new(PTP_USB);
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-tw")) {
			return fudge_test_wifi(ptp);
		} else if (!strcmp(argv[i], "-tu")) {
			return fudge_test(ptp);
		}
	}

	return fudge_main_ui();
}

