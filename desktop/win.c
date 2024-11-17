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

void network_init() {
	// Windows wants to init every thread for socket stuff
	WSADATA wsaData = {0};
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

int fudge_test_all_cameras(void) {
	printf("CI is only included on Linux build\n");
	return 0;
}
