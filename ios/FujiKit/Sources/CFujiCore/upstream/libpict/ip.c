// Windows and POSIX compatible PTP/IP backend
// Copyright 2024 by Daniel C (https://github.com/petabyt/libpict)
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
#include <libpict.h>

// Dump all communication to a file
//#define DUMP_COMM

struct PtpCommPriv {
	int fd;
	int evfd;
	int vidfd; // Some cameras have a mjpeg stream
	FILE *dump;
};

static int set_nonblocking_io(int fd, int enable) {
#ifdef WIN32
	return 0;
#else
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;

	if (enable) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}

	return fcntl(fd, F_SETFL, flags);
#endif
}

static int create_socket(struct PtpRuntime *r, const char *addr, int port, long timeout_sec) {
	int rc;
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd <= 0) {
		ptp_verbose_log(r, "Bad socket fd: %d %d\n", sockfd, errno);
		return -1;
	}

	if (r->set_extra_socket_settings != NULL) {
		rc = r->set_extra_socket_settings(r, sockfd);
		if (rc) {
			ptp_verbose_log(r, "Error binding to wifi network: %d\n", errno);
			return rc;
		}
	}

	int yes = 1;
	// PTP/IP says that TCP_NODELAY must be set to true
	rc = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const void *)&yes, sizeof(int));
	if (rc < 0) {
		ptp_verbose_log(r, "Failed to set nodelay: %d\n", errno);
	}

	rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&yes, sizeof(int));
	if (rc < 0) {
		ptp_verbose_log(r, "Failed to set reuseaddr: %d\n", errno);
	}

	if (set_nonblocking_io(sockfd, 1) < 0) {
		close(sockfd);
		ptp_verbose_log(r, "Failed to set non-blocking IO\n");
		return -1;
	}

	ptp_verbose_log(r, "Connecting to %s:%d\n", addr, port);

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	if (inet_pton(AF_INET, addr, &(sa.sin_addr)) <= 0) {
		close(sockfd);
		ptp_verbose_log(r, "Failed to convert IP address\n");
		return -1;
	}

	if (connect(sockfd, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
		if (errno != EINPROGRESS) {
			close(sockfd);
			ptp_verbose_log(r, "Failed to connect to socket\n");
			return -1;
		}
	}

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);

	struct timeval tv_rcv;
	tv_rcv.tv_sec = 5;
	tv_rcv.tv_usec = 0;
	rc = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&tv_rcv, sizeof(tv_rcv));
	if (rc < 0) {
		ptp_verbose_log(r, "Failed to set rcvtimeo: %d", errno);
	}

	// Wait for socket event
	if (errno == EINPROGRESS) {
		struct timeval tv;
		tv.tv_sec = timeout_sec;
		tv.tv_usec = 0;
		rc = select(sockfd + 1, NULL, &fdset, NULL, &tv);
		if (rc != 1) {
			ptp_verbose_log(r, "select() returned 0 fds: %d\n", errno);
			close(sockfd);
			return -1;
		}
	}

	int so_error = 0;
	socklen_t len = sizeof(so_error);
	if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)&so_error, &len) < 0) {
		close(sockfd);
		ptp_verbose_log(r, "Failed to get socket options\n");
		return -1;
	}

	if (so_error == 0) {
		ptp_verbose_log(r, "Connection established %s:%d (%d)\n", addr, port, sockfd);
		set_nonblocking_io(sockfd, 0);
		return sockfd;
	}
	if (so_error == ECONNREFUSED) {
		ptp_verbose_log(r, "Connection refused - probably invalid IP\n");
	} else {
		ptp_verbose_log(r, "Failed to connect: %d\n", so_error);
	}

	close(sockfd);
	return -1;
}

static struct PtpCommPriv *init_comm(struct PtpRuntime *r) {
	if (r->comm_priv == NULL) {
		r->comm_priv = calloc(1, sizeof(struct PtpCommPriv));

#ifdef DUMP_COMM
#warning "Dumping all comms"
		((struct PtpIpBackend *)r->comm_backend)->dump = fopen("dump.bin", "wb");
		if (((struct PtpIpBackend *)r->comm_backend)->dump == NULL) abort();
#endif
	}

	// Max packet size for TCP
	r->max_packet_size = 0xffff;

	return r->comm_priv;
}

int ptpip_connect(struct PtpRuntime *r, const char *addr, int port, int extra_tmout) {
	ptp_verbose_log(r, "Extra tmout: %d\n", extra_tmout);

	int fd = create_socket(r, addr, port, 2 + extra_tmout);

	struct PtpCommPriv *b = init_comm(r);

	if (fd > 0) {
		b->fd = fd;
		r->io_kill_switch = 0;
		r->operation_kill_switch = 0;
		return 0;
	} else {
		b->fd = 0;
		return fd;
	}
}

int ptpip_connect_events(struct PtpRuntime *r, const char *addr, int port) {
	// TODO: Open as non-blocking
	int fd = create_socket(r, addr, port, 3);
	struct PtpCommPriv *b = init_comm(r);
	if (fd > 0) {
		b->evfd = fd;
		return 0;
	} else {
		b->evfd = 0;
		return fd;
	}
}

int ptpip_connect_video(struct PtpRuntime *r, const char *addr, int port) {
	int fd = create_socket(r, addr, port, 3);
	struct PtpCommPriv *b = init_comm(r);
	if (fd > 0) {
		b->vidfd = fd;
		return 0;
	} else {
		b->vidfd = 0;
		return fd;
	}
}

int ptpip_device_close(struct PtpRuntime *r) {
	struct PtpCommPriv *b = init_comm(r);
	if (b->fd) close(b->fd);
	b->fd = 0;
	if (b->evfd) close(b->evfd);
	b->evfd = 0;
	if (b->vidfd) close(b->vidfd);
	b->vidfd = 0;
	return 0;
}

int ptpip_shutdown_io(struct PtpRuntime *r) {
	// Unblocks a thread stuck in read()/send() on these sockets without closing
	// the descriptors (closing could race with fd reuse). The owning thread still
	// calls ptpip_device_close() afterwards. The caller must serialise this with
	// ptpip_device_close() and with socket creation.
	struct PtpCommPriv *b = r->comm_priv;
	if (b == NULL) return 0;
#ifdef WIN32
	if (b->fd) shutdown(b->fd, SD_BOTH);
	if (b->evfd) shutdown(b->evfd, SD_BOTH);
	if (b->vidfd) shutdown(b->vidfd, SD_BOTH);
#else
	if (b->fd) shutdown(b->fd, SHUT_RDWR);
	if (b->evfd) shutdown(b->evfd, SHUT_RDWR);
	if (b->vidfd) shutdown(b->vidfd, SHUT_RDWR);
#endif
	return 0;
}

void ptpip_comm_deinit(struct PtpRuntime *r) {
	if (r->comm_priv == NULL) return;
	ptpip_device_close(r);
	free(r->comm_priv);
	r->comm_priv = NULL;
}

int ptpip_cmd_write(struct PtpRuntime *r, void *data, unsigned int size) {
	if (r->io_kill_switch) {
		ptp_verbose_log(r, "WARN: kill switch on\n");
		return -1;
	}
	struct PtpCommPriv *b = init_comm(r);

#ifdef DUMP_COMM
	fwrite(data, 1, size, b->dump);
#endif

	int result = (int)send(b->fd, data, size, 0);
	if (result < 0) {
		ptp_verbose_log(r, "read(): %d\n", errno);
		return -1;
	} else {
		return result;
	}
}

int ptpip_cmd_read(struct PtpRuntime *r, void *data, unsigned int size) {
	if (r->io_kill_switch) return -1;
	struct PtpCommPriv *b = init_comm(r);
	int result = (int)read(b->fd, data, size);

#ifdef DUMP_COMM
	fwrite(data, 1, result, b->dump);
#endif

	if (result < 0) {
		ptp_verbose_log(r, "read(): %d\n", errno);
		// To match behavior of USB backends, return 0 bytes when there is no data available
		if (errno == EAGAIN) {
			ptp_verbose_log(r, "EAGAIN\n");
			return 0;
		}
		return -1;
	} else {
		if (r->report_read_progress != NULL) r->report_read_progress(r, result);
		return result;
	}
}

int ptpip_event_send(struct PtpRuntime *r, void *data, unsigned int size) {
	if (r->io_kill_switch) return -1;
	struct PtpCommPriv *b = init_comm(r);
	int result = (int)write(b->evfd, data, size);
	if (result < 0) {
		return -1;
	} else {
		return result;
	}
}

int ptpip_event_read(struct PtpRuntime *r, void *data, unsigned int size) {
	if (r->io_kill_switch) return -1;
	struct PtpCommPriv *b = init_comm(r);
	int result = (int)read(b->evfd, data, size);
	if (result < 0) {
		return -1;
	} else {
		return result;
	}
}

int ptpip_video_read(struct PtpRuntime *r, void *data, unsigned int size) {
	if (r->io_kill_switch) return -1;
	struct PtpCommPriv *b = init_comm(r);
	int result = (int)read(b->vidfd, data, size);
	if (result < 0) {
		if (errno == EAGAIN) return 0;
		return -1;
	} else {
		return result;
	}
}