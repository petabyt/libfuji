#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <app.h>
#include "desktop.h"

// Windows does not have kill!
void kill(int pid) {}

void app_get_file_path(char buffer[256], const char *filename) {
	snprintf(buffer, 256, "%s", filename);
}

void app_get_tether_file_path(char buffer[256]) {
	snprintf(buffer, 256, "TETHER.JPG");
}

int app_get_os_network_handle(struct NetworkHandle *h) {
	return 0;
}

int app_get_wifi_network_handle(struct NetworkHandle *h) {
	return -1;
}

int app_bind_socket_to_network(int fd, struct NetworkHandle *h) {
	return 0;
}

void network_init() {
	// Windows wants to init every thread for socket stuff
	WSADATA wsaData = {0};
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int fudge_test_all_cameras(void) {
	printf("CI is only included on Linux build\n");
	return 0;
}
