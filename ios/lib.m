#include <camlib.h>
#include <stdarg.h>
#include <stdlib.h>

struct PtpRuntime ptp;
struct PtpRuntime *ptp_get() {
	return &ptp;
}

void ptp_report_error(struct PtpRuntime *r, const char *reason, int code) {
	;
}

void ptp_verbose_log(char *fmt, ...) {
	;
}

void ptp_panic(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	abort();
}

void app_increment_progress_bar(int read) {
	
}

int ptp_device_init(struct PtpRuntime *r) { return -1; }
int ptp_cmd_write(struct PtpRuntime *r, void *to, int length) {return -1;}
int ptp_cmd_read(struct PtpRuntime *r, void *to, int length) {return -1;}
int ptp_device_close(struct PtpRuntime *r) {return -1;}
int ptp_device_reset(struct PtpRuntime *r) {return -1;}
int ptp_read_int(struct PtpRuntime *r, void *to, int length) {return -1;}
