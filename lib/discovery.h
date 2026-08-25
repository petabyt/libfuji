#ifndef LIBFUJI_DISCOVERY_H
#define LIBFUJI_DISCOVERY_H

#include "fuji.h"

int fuji_discovery_parse_datagram(
	struct PtpRuntime *runtime,
	char *greeting,
	struct DiscoverInfo *info
);
int fuji_discovery_read_ack(struct PtpRuntime *runtime, int socket_fd);

#endif
