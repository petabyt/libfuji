// Generic portable 
// Copyright 2023 by Daniel C (https://github.com/petabyt/camlib)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#ifdef WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <netinet/tcp.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include <time.h>
#include <camlib.h>
#include "app.h"
#include "fuji.h"

struct PtpIpBackend {
	int fd;
	int evfd;
	int vidfd;
	FILE *dump;
};

//#define DUMP_COMM

#ifdef WIN32
static int set_nonblocking_io(int fd, int enable) {
	//u_long mode = enable;
	//ioctlsocket(fd, FIONBIO, &mode);
	return 0;
}

static void set_receive_timeout(int fd, int sec) {
	DWORD x = sec * 1000;
	int rc = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &x, sizeof(x));
	if (rc < 0) {
		ptp_verbose_log("Failed to set rcvtimeo: %d", errno);
	}
}
#else
static void set_receive_timeout(int fd, int sec) {
	struct timeval tv_rcv;
	tv_rcv.tv_sec = sec;
	tv_rcv.tv_usec = 0;
	int rc = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv_rcv, sizeof(tv_rcv));
	if (rc < 0) {
		ptp_verbose_log("Failed to set rcvtimeo: %d", errno);
	}
}

static int set_nonblocking_io(int fd, int enable) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;

	if (enable) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}

	return fcntl(fd, F_SETFL, flags);
}
#endif

int ptpip_new_timeout_socket(const char *addr, int port, long timeout_sec) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd <= 0) {
		ptp_verbose_log("Bad socket fd: %d %d\n", sockfd, errno);
		return -1;
	}

	int rc = app_bind_socket_wifi(sockfd);
	if (rc) {
		ptp_verbose_log("Error binding to wifi network: %d\n", errno);
		return rc;
	}

	int yes = 1;
//	rc = setsockopt(sockfd, IPPROTO_TCP, SO_KEEPALIVE, (char *)&yes, sizeof(int));
//	if (rc < 0) {
//		ptp_verbose_log("Failed to set keepalive: %d\n", errno);
//	}

//	rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(int));
//	if (rc < 0) {
//		ptp_verbose_log("Failed to set nodelay: %d\n", errno);
//	}
//
	rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(int));
	if (rc < 0) {
		ptp_verbose_log("Failed to set reuseaddr: %d\n", errno);
	}

	if (set_nonblocking_io(sockfd, 1) < 0) {
		close(sockfd);
		ptp_verbose_log("Failed to set non-blocking IO\n");
		return -1;
	}

	plat_dbg("Connecting to %s:%d", addr, port);

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	if (inet_pton(AF_INET, addr, &(sa.sin_addr)) <= 0) {
		close(sockfd);
		ptp_verbose_log("Failed to convert IP address\n");
		return -1;
	}

	if (connect(sockfd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		if (errno != EINPROGRESS) {
			close(sockfd);
			ptp_verbose_log("Failed to connect to socket\n");
			return -1;
		}
	}

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	struct timeval tv;
	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;

	set_receive_timeout(sockfd, 5);

	// Wait for socket event
	if (errno == EINPROGRESS) {
		rc = select(sockfd + 1, NULL, &fdset, NULL, &tv);
		if (rc != 1) {
			ptp_verbose_log("select() returned 0 fds: %d\n", errno);
			return -1;
		}
	}

	int so_error = 0;
	socklen_t len = sizeof(so_error);
	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) {
		close(sockfd);
		ptp_verbose_log("Failed to get socket options\n");
		return -1;
	}

	if (so_error == 0) {
		ptp_verbose_log("Connection established %s:%d (%d)\n", addr, port, sockfd);
		set_nonblocking_io(sockfd, 0);
		return sockfd;
	} else {
		ptp_verbose_log("Failed to connect: %d\n", so_error);
		// 111: Connection refused, invalid IP
	}

	close(sockfd);
	return -1;
}

static struct PtpIpBackend *init_comm(struct PtpRuntime *r) {
	if (r->comm_backend == NULL) {
		r->comm_backend = calloc(1, sizeof(struct PtpIpBackend));

#ifdef DUMP_COMM
#warning "Dumping all comms"
		char filepath[256];
		app_get_file_path(filepath, "dump3.jpeg");
		((struct PtpIpBackend *)r->comm_backend)->dump = fopen(filepath, "wb");
		if (((struct PtpIpBackend *)r->comm_backend)->dump == NULL) abort();
#endif
	}

	return (struct PtpIpBackend *)r->comm_backend;
}

int ptpip_connect(struct PtpRuntime *r, const char *addr, int port, int extra_tmout) {
	ptp_verbose_log("Extra tmout: %d\n", extra_tmout);
	int fd = ptpip_new_timeout_socket(addr, port, 2 + extra_tmout);

	struct PtpIpBackend *b = init_comm(r);

	if (fd > 0) {
		b->fd = fd;
		r->io_kill_switch = 0;
		return 0;
	} else {
		b->fd = 0;
		return fd;
	}
}

int ptpip_connect_events(struct PtpRuntime *r, const char *addr, int port) {
	int fd = ptpip_new_timeout_socket(addr, port, 3);
	struct PtpIpBackend *b = init_comm(r);
	if (fd > 0) {
		b->evfd = fd;
		return 0;
	} else {
		b->evfd = 0;
		return fd;
	}
}

int ptpip_connect_video(struct PtpRuntime *r, const char *addr, int port) {
	int fd = ptpip_new_timeout_socket(addr, port, 3);
	struct PtpIpBackend *b = init_comm(r);
	if (fd > 0) {
		b->vidfd = fd;
		return 0;
	} else {
		b->vidfd = 0;
		return fd;
	}
}

int ptpip_close(struct PtpRuntime *r) {
	struct PtpIpBackend *b = init_comm(r);
	if (b->fd) close(b->fd);
	b->fd = 0;
	if (b->evfd) close(b->evfd);
	b->evfd = 0;
	if (b->vidfd) close(b->vidfd);
	b->vidfd = 0;
	return 0;
}

int ptpip_cmd_write(struct PtpRuntime *r, void *data, int size) {
	if (r->io_kill_switch) {
		ptp_verbose_log("WARN: kill switch on\n");
		return -1;
	}
	struct PtpIpBackend *b = init_comm(r);

	#ifdef DUMP_COMM
	fwrite(data, 1, size, b->dump);
	#endif

	// This is here because of the most bizarre timing issue I've ever seen. With TCP_NODELAY on, on my X-H1,
	// the connection will hang in a very specific place (after setting RemoteVersion). This only happens on my
	// LG VS501 with Android 7. It works perfectly fine on my Google Pixel 6. A bug report was made with the same issue on a sony xperia 5 IV.
	// Once I removed TCP_NODELAY, it works fine again. (And also note the whole time it has worked fine
	// on my X-A2 with all of my devices, regardless of TCP_NODELAY.)
	// It seems that Nagle's algorithm caused a slight delay that made either the camera or Android happy, most likely the former.
	// So... we're just gonna have to put a 5ms delay here. It doesn't hurt anything, it just adds a tiny delay
	// when sending packets. Download speeds are mostly unaffected.
	// I don't know what Fuji does internally with their app, but I truly hope they have a better solution than me.
	//usleep(5000);

	errno = 0;
	int result = send(b->fd, data, size, 0);
	if (result < 0) {
		return -1;
	} else {
		return result;
	}
}

int ptpip_cmd_read(struct PtpRuntime *r, void *data, int size) {
	if (r->io_kill_switch) {
		ptp_verbose_log("WARN: kill switch on\n");
		return -1;
	}
	struct PtpIpBackend *b = init_comm(r);
	int result = read(b->fd, data, size);

#ifdef DUMP_COMM
	fwrite(data, 1, result, b->dump);
#endif

	if (result < 0) {
		ptp_verbose_log("read(): %d %d\n", result, errno);
		return -1;
	} else {
		// TODO: Slow
		app_increment_progress_bar(result);
		return result;
	}
}

int ptpip_event_send(struct PtpRuntime *r, void *data, int size) {
	if (r->io_kill_switch) return -1;
	struct PtpIpBackend *b = init_comm(r);
	int result = write(b->evfd, data, size);
	if (result < 0) {
		return -1;
	} else {
		return result;
	}
}

int ptpip_event_read(struct PtpRuntime *r, void *data, int size) {
	if (r->io_kill_switch) return -1;
	struct PtpIpBackend *b = init_comm(r);
	int result = read(b->evfd, data, size);
	if (result < 0) {
		return -1;
	} else {
		return result;
	}
}
