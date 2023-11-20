// Fujifilm WiFi connection library - this code is a portable extension to camlib.
// Don't add any iOS, JNI, or Dart stuff to it
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>

//#include "myjni.h"
#include "models.h"
#include "fuji.h"
#include "fujiptp.h"

// TODO: Construct a standard device info from bits and pieces of info

struct FujiDeviceKnowledge fuji_known = {0};

int ptp_set_prop_value16(struct PtpRuntime *r, int code, uint16_t value) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SetDevicePropValue;
	cmd.param_length = 1;
	cmd.params[0] = code;

	uint16_t dat[] = {value};

	return ptp_generic_send_data(r, &cmd, dat, sizeof(dat));
}

// Set the compression prop (allows full images to go through, otherwise puts
// extra data in ObjectInfo and cuts off image downloads)
// This appears to take a while, so r->wait_for_response is used here
int fuji_enable_compression(struct PtpRuntime *r) {
	r->wait_for_response = 3;
	int rc = ptp_set_prop_value16(r, PTP_PC_FUJI_NoCompression, 1);
	return rc;
}

int fuji_disable_compression(struct PtpRuntime *r) {
	int rc = ptp_set_prop_value16(r, PTP_PC_FUJI_NoCompression, 0);
	return rc;
}

int fuji_get_device_info(struct PtpRuntime *r) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_FUJI_GetDeviceInfo;
	cmd.param_length = 0;

	return ptp_generic_send(r, &cmd);
}

int fuji_get_events(struct PtpRuntime *r) {
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_EventsList);
	if (rc) return rc;

	struct PtpFujiEvents *ev = (struct PtpFujiEvents *)(ptp_get_payload(r));

	ptp_verbose_log("Found %d events\n", ev->length);
	for (int i = 0; i < ev->length; i++) {
		ptp_verbose_log("%X changed to %d\n", ev->events[i].code, ev->events[i].value);
	}

	for (int i = 0; i < ev->length; i++) {
		switch (ev->events[i].code) {
		case PTP_PC_FUJI_SelectedImgsMode:
			fuji_known.selected_imgs_mode = ev->events[i].value;
			break;
		case PTP_PC_FUJI_ObjectCount:
			fuji_known.num_objects = ev->events[i].value;
			break;
		case PTP_PC_FUJI_CameraState:
			fuji_known.camera_state = ev->events[i].value;
			break;
		}
	}

	return 0;
}

// Call this immediately after session init
int fuji_wait_for_access(struct PtpRuntime *r) {
	// We *need* these properties on camera init - otherwise, produce an error
	fuji_known.camera_state = FUJI_WAIT_FOR_ACCESS;
	fuji_known.num_objects = -1;

	while (1) {
		// After opening session, immediately get events
		int rc = fuji_get_events(r);
		if (rc) return rc;

		// Wait until camera state is unlocked
		if (fuji_known.camera_state != FUJI_WAIT_FOR_ACCESS) {
			if (fuji_known.num_objects == -1) {
				android_err("Failed to get num_objects from first event\n");
				return PTP_RUNTIME_ERR;
			}

			return 0;
		}

		CAMLIB_SLEEP(100);
	}
}

// Handles critical init sequence. This is after initing the socket, and opening session.
// Called right after obtaining access to the device.
int fuji_config_init_mode(struct PtpRuntime *r) {
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_ImageExploreVersion);
	if (rc) return rc;
	fuji_known.image_explore_version = ptp_parse_prop_value(r);
	tester_log("ImageExploreVersion: 0x%X\n", fuji_known.image_explore_version);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteImageExploreVersion);
	if (rc) return rc;
	fuji_known.remote_image_view_version = ptp_parse_prop_value(r);
	tester_log("RemoteImageExploreVersion: 0x%X\n", fuji_known.remote_image_view_version);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_ImageGetVersion);
	if (rc) return rc;
	fuji_known.image_get_version = ptp_parse_prop_value(r);
	tester_log("ImageGetVersion: 0x%X\n", fuji_known.image_get_version);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteVersion);
	if (rc) return rc;
	fuji_known.remote_version = ptp_parse_prop_value(r);
	tester_log("RemoteVersion: 0x%X\n", fuji_known.remote_version);

	tester_log("CameraState is %d\n", fuji_known.camera_state);

	// Determine preferred mode from state and version info
	int mode = 0;
	if (fuji_known.remote_version != -1) {
		mode = FUJI_REMOTE_MODE;
	} else {
		if (fuji_known.camera_state == FUJI_MULTIPLE_TRANSFER) {
			mode = FUJI_VIEW_MULTIPLE;
		} else if (fuji_known.camera_state == FUJI_FULL_ACCESS) {
			mode = FUJI_VIEW_ALL_IMGS;
		} else {
			mode = FUJI_VIEW_ALL_IMGS;
		}
	}

	tester_log("Setting mode to %d\n", mode);

	rc = ptp_set_prop_value16(r, PTP_PC_FUJI_FunctionMode, mode);
	if (rc) return rc;

	return 0;
}

// TODO: rename config image view version
int fuji_config_version(struct PtpRuntime *r) {
	if (fuji_known.remote_version == -1) {
		int rc = ptp_get_prop_value(r, PTP_PC_FUJI_ImageExploreVersion);
		if (rc) return rc;

		int version = ptp_parse_prop_value(r);

		fuji_known.image_view_version = version;

		// The property must be set again (to it's own value) to tell the camera
		// that the current version is supported - this may or may not be necessary
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_ImageExploreVersion, version);
		if (rc) return rc;
	} else {
		ptp_verbose_log("RemoteVersion was %X\n", fuji_known.remote_version);

		uint32_t new_remote_version = FUJI_CAM_CONNECT_REMOTE_VER;

		int rc = ptp_set_prop_value_data(r, PTP_PC_FUJI_RemoteVersion,
			(void *)(&new_remote_version), 4);
		if (rc) return rc;
	}

	return 0;
}

int fuji_config_device_info_routine(struct PtpRuntime *r) {
	if (fuji_known.remote_version != -1) {
		int rc = fuji_get_device_info(r);
		if (rc) return rc;

		// TODO: Parse device info
		// I don't think we actually need it (?)
	}

	return 0;
}

// Tell camera to open event/video sockets
int fuji_remote_mode_open_sockets(struct PtpRuntime *r) {
	if (fuji_known.remote_version == -1) return 0;

	// Begin camera remote - (per spec, OpenCapture is much more generic than 'take picture')
	fuji_known.open_capture_trans_id = r->transaction;
	int rc = ptp_init_open_capture(r, 0, 0);
	if (rc) return rc;

	return 0;
}

// Init remote mode if camera has it
int fuji_remote_mode_end(struct PtpRuntime *r) {
	if (fuji_known.remote_version == -1) return 0;

	// Right after remote mode is entered, camera gives off a bunch of properties
	int rc = fuji_get_events(r);
	if (rc) return rc;

	rc = ptp_terminate_open_capture(r, fuji_known.open_capture_trans_id);
	if (rc) return rc;

	return 0;
}

int fuji_config_image_viewer(struct PtpRuntime *r) {
	if (fuji_known.remote_image_view_version != -1) {
		// Tell the camera that we actually want that mode
		int rc = ptp_set_prop_value16(r, PTP_PC_FUJI_CameraState, FUJI_REMOTE_ACCESS);
		if (rc) return rc;

		// Will confirm CameraState is set
		rc = fuji_get_events(r);
		if (rc) return rc;

		ptp_verbose_log("PTP_PC_FUJI_RemoteImageExploreVersion: %d\n", fuji_known.remote_image_view_version);

		rc = ptp_set_prop_value(r, PTP_PC_FUJI_RemoteImageExploreVersion, fuji_known.remote_image_view_version);
		if (rc) return rc;

		// SD card slot?
		rc = ptp_get_prop_value(r, PTP_PC_FUJI_StorageID);
		if (rc) return rc;
		ptp_verbose_log("Storage ID: %d\n", ptp_parse_prop_value(r));

		rc = ptp_set_prop_value16(r, PTP_PC_FUJI_FunctionMode, FUJI_MODE_REMOTE_IMG_VIEW);
		if (rc) return rc;

		// Set the prop again! For no reason! beause fuji devs say so
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_RemoteImageExploreVersion, fuji_known.remote_image_view_version);
		if (rc) return rc;		

		// The props we set should show up here
		rc = fuji_get_events(r);
		if (rc) return rc;
	}

	return 0;
}

// Temporary RAM-based thumbnail 'cache'. Rolls over a 'tape' like buffer
__attribute__((deprecated))
int ptp_get_thumbnail_smart_cache(struct PtpRuntime *r, int handle, void **ptr, int *length) {
	#define SMART_CACHE_MAX 100

	// smart cache (static)	
	static struct PtpSmartThumbCache {
		int length;
		int loop;
		struct PtpSmartThumbCacheEntry {
			int handle;
			int length;
			void *bytes;
		}entries[SMART_CACHE_MAX];
	}cache = {
		.length = 0,
		.loop = 0,
	};

	// Shouldn't exceed 1mb
#if 1
	int total = 0;
	for (int i = 0; i < cache.length; i++) {
		total += cache.entries[i].length;
	}
	android_err("Totaling %d bytes of cache\n", total);
#endif

	// Search for cached thumb
	for (int i = 0; i < cache.length; i++) {
		if (handle == cache.entries[i].handle) {
			(*ptr) = cache.entries[i].bytes;
			(*length) = cache.entries[i].length;
			return 0;
		}
	}

	// We do not have thumbnail, so we must get it
	int rc = ptp_get_thumbnail(r, (int)handle);
	if (rc) return rc;

	// Skip a cache every other ID to prevent saturation
	if (handle % 2) {
		(*ptr) = ptp_get_payload(r);
		(*length) = ptp_get_payload_length(r);
		return 0;
	}

	// Figure out where to put the thumbnail
	int real_index = 0;
	if (cache.length < SMART_CACHE_MAX) {
		real_index = cache.length;
		cache.length++;
		cache.loop++;
	} else if (cache.length >= SMART_CACHE_MAX || cache.loop >= SMART_CACHE_MAX) {
		// Ran out of spots, begin at the top
		cache.loop = 0;
		free(cache.entries[0].bytes);
		real_index = 0;
	} else if (cache.loop < SMART_CACHE_MAX) {
		// Still out of spots, but freeing and replacing as user scrolls along
		free(cache.entries[cache.loop].bytes);
		real_index = cache.loop;
		cache.loop++;
	}

	cache.entries[real_index].handle = handle;
	cache.entries[real_index].length = ptp_get_payload_length(r);
	cache.entries[real_index].bytes = malloc(ptp_get_payload_length(r));
	memcpy(cache.entries[real_index].bytes, ptp_get_payload(r), ptp_get_payload_length(r));

	(*ptr) = cache.entries[real_index].bytes;
	(*length) = cache.entries[real_index].length;

	return 0;
}
