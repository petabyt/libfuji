#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifdef WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <arpa/inet.h>
#endif
#include "app.h"
#include "fuji.h"

#define FUJI_AUTOSAVE_REGISTER 51542
#define FUJI_AUTOSAVE_CONNECT 51541
#define FUJI_AUTOSAVE_NOTIFY 51540

static int connect_to_notify_server(char *ip, int port) {
	int server_fd, client_fd;
	struct sockaddr_in server_addr;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int rc = app_bind_socket_wifi(server_fd);
	if (rc) return -1;
	if (server_fd < 0) {
		plat_dbg("Failed to create TCP socket");
		abort();
	}

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &(sa.sin_addr)) <= 0) {
		abort();
	}

	if (connect(server_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		if (errno != EINPROGRESS) {
			plat_dbg("Connect fail");
			abort();
		}
	}

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(server_fd, &fdset);
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	if (select(server_fd + 1, NULL, &fdset, NULL, &tv) == 1) {
		int so_error = 0;
		socklen_t len = sizeof(so_error);
		if (getsockopt(server_fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
			plat_dbg("Sockopt fail");
			abort();
		}

		if (so_error == 0) {
			plat_dbg("notify server: Connection established");
			return server_fd;
		}
	}

	close(server_fd);

	return 0;
}

static int start_invite_server(struct DiscoverInfo *info, int port) {
	int server_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	app_bind_socket_wifi(server_fd);
	if (server_fd < 0) {
		plat_dbg("Failed to create TCP socket");
		abort();
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	int yes = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		plat_dbg("Failed to set sockopt");
		abort();
	}

	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		plat_dbg("upnp: Binding failed");
		abort();
	}

	// Listen for incoming connections
	if (listen(server_fd, 5) < 0) {
		plat_dbg("upnp: Listening failed");
		abort();
	}

	plat_dbg("invite server is listening...");

	client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
	if (client_fd < 0) {
		plat_dbg("invite server: Accepting connection failed");
		abort();
	}

	plat_dbg("invite server: Connection accepted from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	// We don't really care about this info
	char buffer[1024];
	int rc = recv(client_fd, buffer, sizeof(buffer), 0);
	if (rc < 0) {
		perror("recv fail");
		abort();
	}

	char *resp;
	if (port == FUJI_AUTOSAVE_REGISTER) {
		resp = 
			"HTTP/1.1 200 OK\r\n"
			"FOLDER: guest\r\n"
			"ServiceName: PCAUTOSAVE/1.0\r\n";
	} else if (port == FUJI_AUTOSAVE_CONNECT) {
		resp = "HTTP/1.1 200 OK\r\n";
	} else {
		abort();
	}

	rc = send(client_fd, resp, strlen(resp), 0);
	if (rc < 0) {
		perror("send");
		abort();
	}

	close(client_fd);

	return 0;
}

// TODO: Respond to any connection
static int respond_to_datagram(char *greeting, struct DiscoverInfo *info) {
	memset(info, 0, sizeof(struct DiscoverInfo));

	char *saveptr;
	char *delim = " :\r\n";
	char *cur = strtok_r(greeting, delim, &saveptr);
	while (cur != NULL) {
		if (!strcmp(cur, "DISCOVER")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) abort();
			strcpy(info->client_name, cur);
		} else if (!strcmp(cur, "DSCADDR")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) abort();
			printf("Client IP: %s\n", cur);
			strcpy(info->ip, cur);
		}
		cur = strtok_r(NULL, delim, &saveptr);
	}

	char *client_name = "desktop";

	char response[] =
		"NOTIFY * HTTP/1.1\r\n"
		"HOST: %s:%d\r\n"
		"IMPORTER: %s\r\n";

	char notify[512];
	sprintf(notify, response, info->ip, FUJI_AUTOSAVE_NOTIFY, client_name);

	int fd = connect_to_notify_server(info->ip, FUJI_AUTOSAVE_NOTIFY);
	if (fd <= 0) {
		return -1;
	}

	send(fd, notify, strlen(notify), 0);

	char repsonse[512];
	int len = recv(fd, response, sizeof(response), 0);
	response[len] = '\0';
	puts(response);

	close(fd);

	return 0;
}

static int accept_register(struct DiscoverInfo *info, char *greeting) {
	int rc = respond_to_datagram(greeting, info);
	if (rc) return rc;

	rc = start_invite_server(info, FUJI_AUTOSAVE_REGISTER);
	if (rc) return rc;

	plat_dbg("Finished registering");

	return 0;
}

static int accept_connect(struct DiscoverInfo *info, char *greeting) {
	int rc = respond_to_datagram(greeting, info);
	if (rc) return rc;

	rc = start_invite_server(info, FUJI_AUTOSAVE_CONNECT);
	if (rc) return rc;

	plat_dbg("Finished connecting");

	return 0;
}

int fuji_discover_thread(struct DiscoverInfo *info) {
	int reg_fd, con_fd;
	struct sockaddr_in reg_addr, con_addr;

	reg_fd = socket(AF_INET, SOCK_DGRAM, 0);
	app_bind_socket_wifi(reg_fd);
	if (reg_fd == -1) {
		plat_dbg("socket");
		abort();
	}

	memset(&reg_addr, 0, sizeof(reg_addr));
	reg_addr.sin_family = AF_INET;
	reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	reg_addr.sin_port = htons(FUJI_AUTOSAVE_REGISTER);
	plat_dbg("Binding to %d", FUJI_AUTOSAVE_REGISTER);
	if (bind(reg_fd, (struct sockaddr *)&reg_addr, sizeof(reg_addr)) == -1) {
		plat_dbg("bind");
		close(reg_fd);
		abort();
	}

	con_fd = socket(AF_INET, SOCK_DGRAM, 0);
	app_bind_socket_wifi(con_fd);
	if (con_fd == -1) {
		plat_dbg("socket");
		abort();
	}

	memset(&con_addr, 0, sizeof(con_addr));
	con_addr.sin_family = AF_INET;
	con_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	con_addr.sin_port = htons(FUJI_AUTOSAVE_CONNECT);
	plat_dbg("Binding to %d", FUJI_AUTOSAVE_CONNECT);
	if (bind(con_fd, (struct sockaddr *)&con_addr, sizeof(con_addr)) == -1) {
		plat_dbg("bind");
		close(con_fd);
		abort();
	}

	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	#define ERRNO errno

	char greeting[1024];
	while (1) {
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
	
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(reg_fd, &fdset);
		FD_SET(con_fd, &fdset);

		int n = select((reg_fd > con_fd ? reg_fd : con_fd) + 1, &fdset, NULL, NULL, &tv);
		if (n == -1) {
			plat_dbg("select: %d", ERRNO);
			abort();	
		}

		if (FD_ISSET(reg_fd, &fdset)) {
			int len = recvfrom(reg_fd, greeting, sizeof(greeting) - 1, 0, NULL, NULL);
			if (len <= 0) {
				plat_dbg("recvfrom: %d", ERRNO);
				abort();
			}
			greeting[len] = '\0';
			accept_register(info, greeting);
		}
		if (FD_ISSET(con_fd, &fdset)) {
			int len = recvfrom(con_fd, greeting, sizeof(greeting) - 1, 0, NULL, NULL);
			if (len <= 0) {
				plat_dbg("recvfrom");
				abort();
			}
			greeting[len] = '\0';
			accept_connect(info, greeting);
			printf("Ready for PTP/IP\n");
			break;
		}

		// idle, TODO cancel
	}

	close(con_fd);
	close(reg_fd);
	return 0;
}
