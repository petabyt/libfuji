// Test basic opcode, get device properties
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <camlib.h>

#include <fuji.h>

int fuji_test_filesystem(struct PtpRuntime *r);
int fuji_test_setup(struct PtpRuntime *r);

void android_err() {}

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


int main() {
	int rc = 0;
	char *ip_addr = "192.168.1.33";

	struct PtpRuntime r;
	ptp_generic_init(&r);
	r.connection_type = PTP_IP_USB;
	if (ptpip_connect(&r, ip_addr, FUJI_CMD_IP_PORT)) {
		printf("Error connecting to %s:%d\n", ip_addr, FUJI_CMD_IP_PORT);
		return 0;
	}

	rc = fuji_test_setup(&r);
	if (rc) return rc;

	rc = fuji_test_filesystem(&r);
	if (rc) return rc;

	ptp_close_session(&r);

	ptpip_close(&r);

	free(r.data);
	return 0;
}

