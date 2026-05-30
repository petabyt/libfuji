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
int plat_update_object_info(struct PtpRuntime *r, int handle, const struct PtpObjectInfo *oi) {
	return 0;
}

__attribute__((weak))
void plat_dbg(char *fmt, ...) {
	printf("DBG: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__((weak))
int fuji_discover_ask_connect(struct PtpRuntime *r, struct DiscoverInfo *info) {
	return 1;
}

__attribute__((weak))
void app_print(struct PtpRuntime *r, char *fmt, ...) {
	printf("APP: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__((weak))
void app_send_cam_name(struct PtpRuntime *r, const char *name) {
	printf("Got camera name '%s'\n", name);
}

__attribute__((weak))
int app_get_os_network_handle(struct NetworkHandle *h) {
	return 0;
}

__attribute__((weak))
int app_get_wifi_network_handle(struct NetworkHandle *h) {
	return -1;
}

__attribute__((weak))
int app_bind_socket_to_network(int fd, struct NetworkHandle *h) {
	return 0;
}

__attribute__((weak))
void tester_log(struct PtpRuntime *r, char *fmt, ...) {
	printf("LOG: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__((weak))
void tester_fail(struct PtpRuntime *r, char *fmt, ...) {
	printf("FAIL: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__((weak))
int fuji_discovery_check_cancel(struct PtpRuntime *r) {
	return 0;
}

__attribute__((weak))
void app_increment_progress_bar(struct PtpRuntime *r, int read) {
	printf("%d\n", read);
}
__attribute__((weak))
void app_report_download_speed(struct PtpRuntime *r, long time, size_t size) {
	int mbps = (int)((size * 8) / (time));
	printf("Download speed: %dmbps\n", mbps);
}
__attribute__((weak))
void app_downloaded_file(struct PtpRuntime *r, const struct PtpObjectInfo *oi, const char *path) {
	printf("File has been downloaded to '%s'\n", path);
}
__attribute__((weak))
void app_get_file_path(struct PtpRuntime *r, char buffer[256], const char *filename) {abort();}
__attribute__((weak))
void app_downloading_file(struct PtpRuntime *r, const struct PtpObjectInfo *oi) {}
__attribute__((weak))
int app_check_thread_cancel(struct PtpRuntime *r) {return 0;}
__attribute__((weak))
void app_get_tether_file_path(struct PtpRuntime *r, char buffer[256]) {abort();}
