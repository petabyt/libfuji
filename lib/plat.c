// Default weak functions that implement platform specific functions
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <libpict.h>
#include <fuji.h>
#include <app.h>

__attribute__((weak))
void plat_dbg(char *fmt, ...) {
	printf("DBG: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
}

__attribute__((weak))
void app_print(struct PtpRuntime *r, char *fmt, ...) {
	printf("APP: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
}

__attribute__((weak))
void app_send_cam_name(struct PtpRuntime *r, const char *name) {
	printf("Got camera name '%s'\n", name);
}

__attribute__((weak))
void tester_log(struct PtpRuntime *r, char *fmt, ...) {
	printf("LOG: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
}

__attribute__((weak))
void tester_fail(struct PtpRuntime *r, char *fmt, ...) {
	printf("FAIL: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
}
