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
}

__attribute__((weak))
char *app_get_client_name(void) {
	return strdup("app");
}

__attribute__((weak))
void app_print(char *fmt, ...) {
	printf("APP: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__((weak))
void app_send_cam_name(const char *name) {
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
void tester_log(char *fmt, ...) {
	printf("LOG: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__((weak))
void tester_fail(char *fmt, ...) {
	printf("FAIL: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__((weak))
int fuji_discovery_check_cancel(void *arg) {
	return 0;
}

__attribute__((weak))
void fuji_discovery_update_progress(void *arg, enum DiscoverUpdateMessages progress) {
	printf("Discovery progress '%d'\n", progress);
}
__attribute__((weak))
void app_increment_progress_bar(int read) {
	printf("%d\n", read);
}
__attribute__((weak))
void app_report_download_speed(long time, size_t size) {
	int mbps = (int)((size * 8) / (time));
	printf("Download speed: %dmbps\n", mbps);
}
__attribute__((weak))
void app_downloaded_file(const struct PtpObjectInfo *oi, const char *path) {
	printf("File has been downloaded to '%s'\n", path);
}
__attribute__((weak))
void app_get_file_path(char buffer[256], const char *filename) {abort();}
__attribute__((weak))
void app_downloading_file(const struct PtpObjectInfo *oi) {}
__attribute__((weak))
int app_check_thread_cancel(void) {return 0;}
__attribute__((weak))
void app_get_tether_file_path(char buffer[256]) {abort();}
