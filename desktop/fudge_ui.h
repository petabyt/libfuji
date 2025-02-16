#pragma once

struct State {
	pthread_mutex_t *mutex;
	int is_camera_connected;

	struct PtpDeviceEntry *camlib_entries;

	struct FujiProfile fp;
};

extern struct State fuji_state;
