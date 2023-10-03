#ifndef FUJIAPP_FUJI_H
#define FUJIAPP_FUJI_H

#include "models.h"

int ptp_get_thumbnail_smart_cache(struct PtpRuntime *r, int handle, void **ptr, int *length);

int fuji_config_version(struct PtpRuntime *r);
int fuji_config_init_mode(struct PtpRuntime *r);

int fuji_config_image_viewer(struct PtpRuntime *r);
int fuji_config_device_info_routine(struct PtpRuntime *r);

int fuji_remote_mode_open_sockets(struct PtpRuntime *r);
int fuji_remote_mode_end(struct PtpRuntime *r);

int fuji_wait_for_access(struct PtpRuntime *r);

int fuji_get_events(struct PtpRuntime *r);

int fuji_get_first_events(struct PtpRuntime *r);

int fuji_disable_compression(struct PtpRuntime *r);
int fuji_enable_compression(struct PtpRuntime *r);

// Holds vital info about the camera
struct FujiDeviceKnowledge {
	struct FujiCameraInfo *info;
	int camera_state;
	int selected_imgs_mode;

	int image_explore_version; // should be image_explore_version
	int remote_image_view_version;
	int image_view_version;
	int image_get_version;
	int remote_version;

	int num_objects;

	int open_capture_trans_id;
};

extern struct FujiDeviceKnowledge fuji_known;

#endif
