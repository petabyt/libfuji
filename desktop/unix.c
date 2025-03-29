#include <stdio.h>
#include <stdlib.h>
#include <app.h>

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

void network_init(void) {
	// Linux doesn't need anything to init network
}
