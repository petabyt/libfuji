#ifndef FUJI_MODELS_H
#define FUJI_MODELS_H

#include <stdint.h>

struct FujiCameraInfo {
	char *name;
	uint8_t gps_support;
	uint8_t has_bluetooth;
	uint8_t capture_support;
	uint8_t firm_update_support;
	int16_t release_year;
};

struct FujiCameraInfo *fuji_get_model_info(char *name);

#endif
