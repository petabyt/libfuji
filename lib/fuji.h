#ifndef FUJIAPP_FUJI_H
#define FUJIAPP_FUJI_H

#include "models.h"

int fuji_config_version(struct PtpRuntime *r);
int fuji_config_init_mode(struct PtpRuntime *r);

// Holds vital info about the camera
struct FujiDeviceKnowledge {
	struct FujiCameraInfo *info;
	int function_version;
};

extern struct FujiDeviceKnowledge fuji_known;

#endif
