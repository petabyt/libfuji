// Fujifilm WiFi connection library - this code is a portable extension to camlib -
// don't add any iOS, JNI, or Dart stuff to it
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>

#include "myjni.h"
#include "models.h"
#include "fuji.h"
#include "fujiptp.h"

// TODO: Construct a standard device info from bits and pieces of info

struct FujiDeviceKnowledge fuji_known = {0};

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
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_CameraState);
	if (rc) return rc;

	int value = ptp_parse_prop_value(r);
	if (value == -1) {
		return PTP_RUNTIME_ERR;
	}

	ptp_verbose_log("camera state: %d\n", value);
	fuji_known.camera_state = value;

	// We don't need to poll the device
	if (value != FUJI_WAIT_FOR_ACCESS) {
		return 0;
	}

	while (1) {
		rc = fuji_get_events(r);
		if (rc) return rc;

		// Wait until camera state is unlocked
		if (fuji_known.camera_state != FUJI_WAIT_FOR_ACCESS) {
			return 0;
		}

		CAMLIB_SLEEP(100);
	}
}

// X-App doesn't do this, but the old app did
int fuji_set_remote_version(struct PtpRuntime *r) {
	if (fuji_known.remote_version != -1) {
		int rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteVersion);
		if (rc) return rc;

		int remote_version = ptp_parse_prop_value(r);

		ptp_verbose_log("RemoteVersion was %X\n", remote_version);

		// RemoteVersion is actually two words. The old app gave 11.2, so we'll try that
		uint16_t new_remote_version[] = {
			11, 2
		};

		rc = ptp_set_prop_value_data(r, PTP_PC_FUJI_RemoteVersion,
			(void *)(&new_remote_version), sizeof(new_remote_version));
		return rc;
	}

	return 0;
}

// Handles critical init sequence. This is after initing the socket, and opening session.
// Called right after obtaining access to the device.
int fuji_config_init_mode(struct PtpRuntime *r) {
	// Try and learn about the camera

	// Get image viewer version
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_ImageExploreVersion);
	if (rc) return rc;
	fuji_known.function_version = ptp_parse_prop_value(r);

	// If we haven't gotten number of objects from get_events
	if (fuji_known.num_objects == 0) {
		rc = ptp_get_prop_value(r, PTP_PC_FUJI_ObjectCount);
		if (rc) return rc;
		fuji_known.num_objects = ptp_parse_prop_value(r);	
	}

	// Get image viewer version for remote
	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteImageExploreVersion);
	if (rc) return rc;
	fuji_known.remote_image_view_version = ptp_parse_prop_value(r);

	// Get image downloader version
	rc = ptp_get_prop_value(r, PTP_PC_FUJI_ImageGetVersion);
	if (rc) return rc;
	fuji_known.image_get_version = ptp_parse_prop_value(r);

	// Get remote version (might be none)
	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteVersion);
	if (rc) return rc;
	fuji_known.remote_version = ptp_parse_prop_value(r);

	tester_log("camera state is %d\n", fuji_known.camera_state);

	// Determine function mode from Camera state
	int mode = 0;
	if (fuji_known.camera_state == FUJI_MULTIPLE_TRANSFER) {
		mode = FUJI_VIEW_MULTIPLE;
	} else if (fuji_known.camera_state == FUJI_FULL_ACCESS) {
		// Guess
		if (fuji_known.image_get_version == 1) {
			mode = FUJI_VIEW_ALL_IMGS;
		} else {
			mode = FUJI_REMOTE_MODE;
		}
	} else if (fuji_known.camera_state == FUJI_REMOTE_ACCESS) {
		mode = FUJI_REMOTE_MODE;
	} else {
		mode = FUJI_REMOTE_MODE;
	}

	tester_log("Setting mode to %d\n", mode);

	rc = ptp_set_prop_value(r, PTP_PC_FUJI_FunctionMode, mode);
	if (rc) return rc;

	rc = fuji_set_remote_version(r);
	if (rc) return rc;

	return 0;
}

// TODO: rename config image view version
int fuji_config_version(struct PtpRuntime *r) {
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_ImageExploreVersion);
	if (rc) return rc;

	int version = ptp_parse_prop_value(r);

	fuji_known.image_view_version = version;

	// The property must be set again (to it's own value) to tell the camera
	// that the current version is supported - this may or may not be necessary
	rc = ptp_set_prop_value(r, PTP_PC_FUJI_ImageExploreVersion, version);
	if (rc) return rc;
	return 0;
}

// Very weird init routine seen on older cameras - this is really messed up,
// hopefully the camera doesn't expect this, but it might
int fuji_config_device_info_routine(struct PtpRuntime *r) {
	// Don't want remote capture (?)
	return 0;
	if (fuji_known.remote_version != -1) {
		// Begin camera remote
		int trans = r->transaction;
		int rc = ptp_init_open_capture(r, 0, 0);
		if (rc) return rc;
	}

	return 0;
}

int fuji_config_remote_image_viewer(struct PtpRuntime *r) {
	if (fuji_known.remote_image_view_version != -1) {
		// Tell the camera that we actually want that mode
		int rc = ptp_set_prop_value(r, PTP_PC_FUJI_CameraState, FUJI_REMOTE_ACCESS);
		if (rc) return rc;

		rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteImageExploreVersion);
		if (rc) return rc;

		// For X-T2, the value is 2
		// For X-A2, there is no payload given (-1)

		int view_version = ptp_parse_prop_value(r);

		// Camera doesn't have this property - return silently
		if (view_version == -1) return 0;

		ptp_verbose_log("PTP_PC_FUJI_RemoteImageExploreVersion: %d\n", view_version);

		rc = ptp_set_prop_value(r, PTP_PC_FUJI_FunctionMode, FUJI_MODE_REMOTE_IMG_VIEW);
		if (rc) return rc;

		// Try to get the version again
		rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteImageExploreVersion);
		if (rc) return rc;

		view_version = ptp_parse_prop_value(r);

		ptp_verbose_log("PTP_PC_FUJI_RemoteImageExploreVersion: %d\n", view_version);

		// Assume the version is correct, verify it by setting
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_RemoteImageExploreVersion, view_version);
		if (rc) return rc;
		
	}

	return 0;
}

