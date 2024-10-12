#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifdef WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <poll.h>
	#include <arpa/inet.h>
#endif
#include "app.h"
#include "fuji.h"

// Note: This code was previously refactored to support multiple instances of DiscoveryState
struct DiscoveryState {
	int reg_fd;
	int con_fd;
	int tether_fd;
	int pcss_fd;
};


// Main tether handshake port
#define FUJI_TETHER 51560
// Tether client must advertise itself on this port
#define FUJI_PCSS_BROADCAST 51562
// Register and connect events are sent over two different ports.
#define FUJI_AUTOSAVE_REGISTER 51542
#define FUJI_AUTOSAVE_CONNECT 51541
// A TCP port used for PC AutoSave handshake process
#define FUJI_AUTOSAVE_NOTIFY 51540

static int get_local_ip(char buffer[64]) {
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr("1.1.1.1");
	serv.sin_port = htons(1234);
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	int rc = connect(sock, (const struct sockaddr*) &serv, sizeof(serv));
	if (rc < 0) return rc;

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	rc = getsockname(sock, (struct sockaddr*) &name, &namelen);
	if (rc < 0) return rc;

	inet_ntop(AF_INET, &name.sin_addr, buffer, 64);

	return 0;
}

static int connect_to_notify_server(struct DiscoveryState *s, char *ip, int port) {
	int server_fd;

	plat_dbg("Connecting to %s:%d\n", ip, port);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		plat_dbg("Failed to create TCP socket");
		return -1;
	}

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &(sa.sin_addr)) <= 0) {
		plat_dbg("inet_pton");
		return -1;
	}

	if (connect(server_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		if (errno != EINPROGRESS) {
			plat_dbg("Connect fail");
			return -1;
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
			return -1;
		}

		if (so_error == 0) {
			plat_dbg("notify server: Connection established");
			return server_fd;
		}
	} else {
		plat_dbg("select failed to connect");
	}

	close(server_fd);

	return 0;
}

static int start_invite_server(struct DiscoveryState *s, struct DiscoverInfo *info, int port) {
	int server_fd, client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		plat_dbg("Failed to create TCP socket");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	int yes = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		plat_dbg("Failed to set sockopt");
		return -1;
	}

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		plat_dbg("upnp: Binding failed");
		return -1;
	}

	// Listen for incoming connections
	if (listen(server_fd, 5) < 0) {
		plat_dbg("upnp: Listening failed");
		return -1;
	}

	struct timeval timeout;
	fd_set read_fds;

	FD_ZERO(&read_fds);
	FD_SET(server_fd, &read_fds);

	timeout.tv_sec = 20;
	timeout.tv_usec = 0;

	plat_dbg("invite server is listening...");

	int rc = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);
	if (rc < 0) {
		plat_dbg("select() failed");
		close(server_fd);
		return -1;
	} else if (rc == 0) {
		plat_dbg("select() timeout");
		close(server_fd);
		return -1;
	}

	client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
	if (client_fd < 0) {
		plat_dbg("invite server: Accepting connection failed: %d", errno);
		return -1;
	}

	plat_dbg("invite server: Connection accepted from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	fuji_discovery_update_progress(NULL, 3);

	// We don't really care about this info
	char buffer[1024];
	rc = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (rc < 0) {
		perror("recv fail");
		return -1;
	}
	buffer[rc] = '\0';

	char *saveptr;
	char *delim = " :\r\n";
	char *cur = strtok_r(buffer, delim, &saveptr);
	while (cur != NULL) {
		if (!strcmp(cur, "DSCNAME")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) return -1;
			snprintf(info->camera_name, sizeof(info->camera_name), "%s", cur);
		} else if (!strcmp(cur, "DSCMODEL")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) return -1;
			snprintf(info->camera_model, sizeof(info->camera_model), "%s", cur);
		}
		cur = strtok_r(NULL, delim, &saveptr);
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
		return -1;
	}

	rc = send(client_fd, resp, strlen(resp), 0);
	if (rc < 0) {
		plat_dbg("Failed to send response");
		return -1;
	}

	close(client_fd);
	close(server_fd);

	return 0;
}

// TODO: Respond to any connection
static int respond_to_datagram(struct DiscoveryState *s, char *greeting, struct DiscoverInfo *info) {
	char *saveptr;
	char *delim = " :\r\n";
	char *cur = strtok_r(greeting, delim, &saveptr);
	while (cur != NULL) {
		if (!strcmp(cur, "DISCOVER")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) return -1;
			plat_dbg("Client name: %s\n", cur);
			strncpy(info->client_name, cur, sizeof(info->client_name));
		} else if (!strcmp(cur, "DSCADDR")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) return -1;
			plat_dbg("Client IP: %s\n", cur);
			strncpy(info->camera_ip, cur, sizeof(info->camera_ip));
		}
		cur = strtok_r(NULL, delim, &saveptr);
	}

	fuji_discovery_update_progress(NULL, 1);

//	char *client_name = app_get_client_name();

	char response[] =
		"NOTIFY * HTTP/1.1\r\n"
		"HOST: %s:%d\r\n"
		"IMPORTER: %s\r\n";

	char notify[512];
	snprintf(
		notify, sizeof(notify), response,
		info->camera_ip, FUJI_AUTOSAVE_NOTIFY, info->client_name // use whatever name the camera is looking for :)
	);

//	free(client_name);

	int fd = connect_to_notify_server(s, info->camera_ip, FUJI_AUTOSAVE_NOTIFY);
	if (fd <= 0) {
		plat_dbg("Failed to connect to notify server: %d", fd);
		return -1;
	}

	size_t len = send(fd, notify, strlen(notify), 0);
	if (len != strlen(notify)) {
		plat_dbg("Failed to send datagram response");
		return -1;
	}

	char ack[512];
	len = recv(fd, ack, sizeof(ack), 0);
	if (len <= 0) {
		plat_dbg("Failed to read datagram response");
		return -1;
	}
	response[len] = '\0';
	plat_dbg(response);

	close(fd);

	return 0;
}

static int accept_register(struct DiscoveryState *s, struct DiscoverInfo *info, char *greeting) {
	int rc = respond_to_datagram(s, greeting, info);
	if (rc) return rc;

	fuji_discovery_update_progress(NULL, 2);

	rc = start_invite_server(s, info, FUJI_AUTOSAVE_REGISTER);
	if (rc) return rc;

	plat_dbg("Finished registering");

	fuji_discovery_update_progress(NULL, 4);

	return 0;
}

static int accept_connect(struct DiscoveryState *s, struct DiscoverInfo *info, char *greeting) {
	int rc = respond_to_datagram(s, greeting, info);
	if (rc) return rc;

	fuji_discovery_update_progress(NULL, 2);

	rc = start_invite_server(s, info, FUJI_AUTOSAVE_CONNECT);
	if (rc) return rc;

	plat_dbg("Finished connecting");

	info->camera_port = FUJI_CMD_IP_PORT;

	fuji_discovery_update_progress(NULL, 4);

	return 0;
}

static int open_dgram_socket(struct DiscoveryState *s, int port) {
	struct sockaddr_in addr;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		plat_dbg("socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	plat_dbg("Binding to %d", port);
	int rc = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (rc < 0) {
		plat_dbg("bind: %d", errno);
		close(fd);
		return -FUJI_D_OPEN_DENIED;
	}

	return fd;
}

int fuji_open_tether_server(struct DiscoveryState *s, const char *local_ip) {
	int server_fd;
	struct sockaddr_in server_addr;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		plat_dbg("Failed to create TCP socket");
		return -1;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(FUJI_TETHER);
	server_addr.sin_addr.s_addr = inet_addr(local_ip);

	int yes = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		plat_dbg("Failed to set sockopt %d", errno);
		return -1;
	}
//	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) < 0) {
//		plat_dbg("Failed to set sockopt %d", errno);
//		return -1;
//	}

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		plat_dbg("upnp: Binding failed %d", errno);
		return -1;
	}

	// TODO: delete and add select()
	if (listen(server_fd, 5) < 0) {
		plat_dbg("upnp: Listening failed");
		return -1;
	}

	return server_fd;
}

static int fuji_tether_accept(struct DiscoverInfo *info, int server_fd, void *arg) {
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
	if (client_fd < 0) {
		perror("accept");
		plat_dbg("invite server: Accepting connection failed");
		return -1;
	}

	fuji_discovery_update_progress(arg, 0);

	plat_dbg("invite server: Connection accepted from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	// We don't really care about this info
	char buffer[1024];
	int rc = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (rc < 0) {
		perror("recv fail");
		return -1;
	}
	buffer[rc] = '\0';

	char *saveptr;
	char *delim = " :\r\n";
	char *cur = strtok_r(buffer, delim, &saveptr);
	while (cur != NULL) {
		if (!strcmp(cur, "DSC")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) return -1;
			snprintf(info->camera_ip, sizeof(info->camera_ip), "%s", cur);
		} else if (!strcmp(cur, "CAMERANAME")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) return -1;
			snprintf(info->camera_model, sizeof(info->camera_model), "%s", cur);
		} else if (!strcmp(cur, "DSCPORT")) {
			cur = strtok_r(NULL, delim, &saveptr);
			if (cur == NULL) return -1;
			char port_buf[16];
			snprintf(port_buf, sizeof(port_buf), "%s", cur);
			info->camera_port = strtol(port_buf, NULL, 10);
		}
		cur = strtok_r(NULL, delim, &saveptr);
	}

	fuji_discover_ask_connect(arg, info);

	const char resp[] = "HTTP/1.1 200 OK\r\n";
	rc = send(client_fd, resp, sizeof(resp), 0);
	if (rc < 0) {
		plat_dbg("Failed to send response");
		return -1;
	}

	close(client_fd);

	return 0;
}

static int open_pcss(struct DiscoveryState *s) {
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	int b = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &b, sizeof(b)) < 0) {
		return -1;
	}

	return sock;
}

static int send_pcss_datagram(int sock, const char *local_ip) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(FUJI_PCSS_BROADCAST);
	addr.sin_addr.s_addr = inet_addr("192.168.1.255");

	char broadcast[512];
	sprintf(
		broadcast,
		"DISCOVERY * HTTP/1.1\r\n"
		"HOST: %s\r\n"
		"MX: 5\r\n"
		"SERVICE: PCSS/1.0\r\n",
		local_ip
	);

	ssize_t rc = sendto(sock, broadcast, strlen(broadcast), 0, (struct sockaddr *)&addr, sizeof(addr));
	if (rc == -1) {
		return -1;
	}

	return 0;
}

static void close_all_sockets(struct DiscoveryState *s) {
	if (s->con_fd > 0) {
		close(s->con_fd);
	}
	if (s->reg_fd > 0) {
		close(s->reg_fd);
	}
	if (s->tether_fd > 0) {
		close(s->tether_fd);
	}
	if (s->pcss_fd > 0) {
		close(s->pcss_fd);
	}
}

static int try_connect_all_sockets(struct DiscoveryState *s, const char *local_ip) {
	if (s->reg_fd == 0) {
		s->reg_fd = open_dgram_socket(s, FUJI_AUTOSAVE_REGISTER);
		if (s->reg_fd <= 0) {
			plat_dbg("Error connect register: %d", s->reg_fd);
			return -1;
		}
	}

	if (s->con_fd == 0) {
		s->con_fd = open_dgram_socket(s, FUJI_AUTOSAVE_CONNECT);
		if (s->con_fd <= 0) {
			plat_dbg("Error connect svr: %d", s->con_fd);
			return -1;
		}
	}

	if (s->tether_fd == 0) {
		s->tether_fd = fuji_open_tether_server(s, local_ip);
		if (s->tether_fd < 0) {
			perror("socket");
			return -1;
		}
	}

	if (s->pcss_fd == 0) {
		s->pcss_fd = open_pcss(s);
		if (s->pcss_fd < 0) {
			plat_dbg("Failed to open pcss port");
			return -1;
		}
	}

	return 0;
}

static int state_idle(struct DiscoveryState *s, struct DiscoverInfo *info, const char *local_ip, void *arg) {
	if (s->pcss_fd != 0) {
		if (send_pcss_datagram(s->pcss_fd, local_ip)) {
			plat_dbg("Failed to send datagram: %d", errno);
			return -1;
		}
	}

	char greeting[1024];
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	int max = 0;

	fd_set fdset;
	FD_ZERO(&fdset);
	if (s->reg_fd > 0) {
		FD_SET(s->reg_fd, &fdset);
		if (s->reg_fd > max) max = s->reg_fd;
	}
	if (s->con_fd > 0) {
		FD_SET(s->con_fd, &fdset);
		if (s->con_fd > max) max = s->con_fd;
	}
	if (s->tether_fd > 0) {
		FD_SET(s->tether_fd, &fdset);
		if (s->tether_fd > max) max = s->tether_fd;
	}

	// TODO: this is triggered when a socket disconnect happens, not only when it's connected.
	int n = select(max + 1, &fdset, NULL, NULL, &tv);
	if (n < 0) {
		plat_dbg("select: %d", n);
		return -1;
	}

	if (s->reg_fd > 0) {
		if (FD_ISSET(s->reg_fd, &fdset)) {
			ptp_verbose_log("AutoSave register\n");
			info->transport = FUJI_FEATURE_AUTOSAVE;
			int len = recvfrom(s->reg_fd, greeting, sizeof(greeting) - 1, 0, NULL, NULL);
			if (len <= 0) {
				plat_dbg("recvfrom: %d", len);
				return -1;
			}
			greeting[len] = '\0';
			fuji_discovery_update_progress(arg, 0);
			int rc = accept_register(s, info, greeting);
			if (rc) return -1;
			return FUJI_D_REGISTERED;
		}
	}
	if (s->con_fd > 0) {
		if (FD_ISSET(s->con_fd, &fdset)) {
			ptp_verbose_log("AutoSave connect\n");
			info->transport = FUJI_FEATURE_AUTOSAVE;
			int len = recvfrom(s->con_fd, greeting, sizeof(greeting) - 1, 0, NULL, NULL);
			if (len <= 0) {
				plat_dbg("recvfrom: %d", len);
				return -1;
			}
			greeting[len] = '\0';
			fuji_discovery_update_progress(arg, 0);
			int rc = accept_connect(s, info, greeting);
			if (rc) return -1;
			return FUJI_D_GO_PTP;
		}
	}
	if (s->tether_fd > 0) {
		if (FD_ISSET(s->tether_fd, &fdset)) {
			ptp_verbose_log("Tether connect\n");
			info->transport = FUJI_FEATURE_WIRELESS_TETHER;
			int rc = fuji_tether_accept(info, s->tether_fd, arg);
			if (rc) return -1;
			return FUJI_D_GO_PTP;
		}
	}

	if (fuji_discovery_check_cancel(arg)) {
		return FUJI_D_CANCELED;
	}

	return 0;
}

int fuji_discover_thread(struct DiscoverInfo *info, char *client_name, void *arg) {
	memset(info, 0, sizeof(struct DiscoverInfo));

	char local_ip[64];
	if (get_local_ip(local_ip)) {
		return -1;
	}

	plat_dbg("You are on %s", local_ip);

	app_get_os_network_handle(&info->h);

	struct DiscoveryState s;
	memset(&s, 0, sizeof(struct DiscoveryState));

	int rc = try_connect_all_sockets(&s, local_ip);
	if (rc == 0) {
		while (rc == 0) {
			rc = state_idle(&s, info, local_ip, arg);
		}
	}

	if (rc == -1) {
		app_print("Error connecting to camera."); // TODO: localize
	}

	close_all_sockets(&s);

	return rc;
}
