#ifndef FUJIAPP_FUJI_H
#define FUJIAPP_FUJI_H

#include "models.h"

int fuji_config_version(struct PtpRuntime *r);
int fuji_config_init_mode(struct PtpRuntime *r);

int fuji_config_remote_photo_viewer(struct PtpRuntime *r);
int fuji_config_device_info_routine(struct PtpRuntime *r);

int fuji_wait_for_access(struct PtpRuntime *r);

// Holds vital info about the camera
struct FujiDeviceKnowledge {
	struct FujiCameraInfo *info;
	int unlocked_mode;
	int function_version;
	int photo_view_version;
	int photo_get_version;
	int remote_version;

	int num_objects;
};

extern struct FujiDeviceKnowledge fuji_known;

#endif
