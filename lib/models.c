// Basic model feature support table
// Copyright 2023 (c) Unofficial fujiapp
#include <string.h>
#include "models.h"

struct FujiCameraInfo fuji_cameras[] = {
{ "GFX 50S", 1, 0, 0, 0, 0 },
{ "X-Pro2", 1, 0, 0, 0, 0 },
{ "X-M1", 1, 0, 0, 0, 0 },
{ "X-E2S", 1, 0, 0, 0, 0 },
{ "X-A3", 1, 0, 0, 0, 0 },
{ "X-T3", 1, 1, 1, 1, 0 },
{ "X-T100", 1, 1, 0, 0, 0 },
{ "X-T30", 1, 1, 1, 1, 0 },
{ "X-T2", 1, 0, 0, 0, 0 },
{ "X-E3", 1, 1, 1, 1, 0 },
{ "X-A1", 1, 0, 0, 0, 0 },
{ "X-H1", 1, 1, 1, 1, 0 },
{ "GFX 50R", 1, 1, 1, 1, 0 },
{ "X-A5", 1, 1, 0, 0, 0 },
{ "X-A2", 1, 0, 0, 0, 0 },
{ "X-E2", 1, 0, 0, 0, 0 },
{ "X-T10", 1, 0, 0, 0, 0 },
{ "X-T20", 1, 0, 0, 0, 0 },
{ "X-A10", 1, 0, 0, 0, 0 },
{ "X100F", 1, 0, 0, 0, 0 },
{ "X30", 1, 0, 0, 0, 0 },
{ "X-T1", 1, 0, 0, 0, 0 },
{ "X100T", 1, 0, 0, 0, 0 },
{ "X70", 1, 0, 0, 0, 0 },
{ "XF10", 1, 1, 1, 1, 0 },
{ "XQ1", 1, 0, 0, 0, 0 },
{ "XQ2", 1, 0, 0, 0, 0 },
{ "X-A20", 1, 0, 0, 0, 0 },
{ "GFX 100", 1, 1, 1, 1, 0 },
{ "X-A7", 1, 1, 1, 1, 0 },
{ "X-Pro3", 1, 1, 1, 1, 0 },
{ "X100V", 1, 1, 1, 1, 0 },
{ "X-T200", 1, 1, 1, 1, 0 },
{ "X-T4", 1, 1, 1, 1, 0 },
{ "X-S10", 1, 1, 1, 1, 0 },
};


struct FujiCameraInfo *fuji_get_model_info(char *name) {
	for (int i = 0; i < (int)(sizeof(fuji_cameras) / sizeof(fuji_cameras[0])); i++) {
		if (!strcmp(fuji_cameras[i].name, name)) {
			return &(fuji_cameras[i]);
		}
	}

	return NULL;
}
