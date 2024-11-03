#include <stdio.h>
#include <stdlib.h>
#include <app.h>

int app_get_os_network_handle(struct NetworkHandle *h) {
	return 0;
}

int app_get_wifi_network_handle(struct NetworkHandle *h) {
	return -1;
}

int app_bind_socket_to_network(int fd, struct NetworkHandle *h) {
	return 0;
}
