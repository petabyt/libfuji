#ifndef FUJI_MODELS_H
#define FUJI_MODELS_H

struct FujiCameraInfo {
	char *name;
	int gps_support;
	int has_bluetooth;
	int capture_support;
	int firm_update_support;

	// TODO: Release date (last resort guessing)
};

extern struct FujiCameraInfo fuji_cameras[];

#endif
