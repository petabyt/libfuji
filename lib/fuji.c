// Fujifilm WiFi connection library - this code is a portable extension to camlib -
// don't add any iOS, JNI, or Dart stuff to it
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <jni.h>
#include <camlib.h>

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

// Call this immediately after session init
int fuji_wait_for_access(struct PtpRuntime *r) {
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_Unlocked);
	if (rc) return rc;

	int value = ptp_parse_prop_value(r);
	if (value == -1) {
		return PTP_RUNTIME_ERR;
	}

	ptp_verbose_log("unlocked_mode: %d\n", value);
	fuji_known.unlocked_mode = value;

	// We don't need to poll the device
	if (value != FUJI_WAIT_FOR_ACCESS) {
		return 0;
	}

	while (1) {
		rc = ptp_get_prop_value(r, PTP_PC_FUJI_EventsList);
		if (rc) return rc;

		// Apply events structure to payload, and check for unlocked event (PTP_PC_FUJI_Unlocked)
		struct PtpFujiEvents *ev = (struct PtpFujiEvents *)(ptp_get_payload(r));
		ptp_verbose_log("Found %d events\n", ev->length);
		for (int i = 0; i < ev->length; i++) {
			ptp_verbose_log("%X changed to %d\n", ev->events[i].code, ev->events[i].value);
		}
		
		for (int i = 0; i < ev->length; i++) {
			if (ev->events[i].code == PTP_PC_FUJI_Unlocked && (ev->events[i].value != FUJI_WAIT_FOR_ACCESS)) {
				fuji_known.unlocked_mode = ev->events[i].value;
				return 0;
			}
		}

		struct PtpDevPropDesc *pc = (struct PtpDevPropDesc *)(ptp_get_payload(r));
		CAMLIB_SLEEP(100);
	}
}

// X-App doesn't do this, but the old app did
int fuji_set_remote_version(struct PtpRuntime *r) {
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

// Handles critical init sequence. This is after initing the socket, and opening session.
// Called right after obtaining access to the device.
int fuji_config_init_mode(struct PtpRuntime *r) {
	// Try and learn about the camera

	// Get function version
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_FunctionVersion);
	if (rc) return rc;
	fuji_known.function_version = ptp_parse_prop_value(r);

	// Get photo viewer version
	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemotePhotoViewVersion);
	if (rc) return rc;
	fuji_known.photo_view_version = ptp_parse_prop_value(r);

	// Get photo downloader version
	rc = ptp_get_prop_value(r, PTP_PC_FUJI_PhotoGetVersion);
	if (rc) return rc;
	fuji_known.photo_get_version = ptp_parse_prop_value(r);

	// Get photo downloader version (might be none, -1)
	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteVersion);
	if (rc) return rc;
	fuji_known.remote_version = ptp_parse_prop_value(r);

	ptp_verbose_log("Unlocked mode is %d\n", fuji_known.unlocked_mode);

	// Determine function mode from Unlocked state
	int mode = 0;
	if (fuji_known.unlocked_mode == FUJI_MULTIPLE_TRANSFER) {
		mode = FUJI_VIEW_MULTIPLE;
	} else if (fuji_known.unlocked_mode == FUJI_FULL_ACCESS) {
		// Guess
		if (fuji_known.photo_get_version == 1) {
			mode = FUJI_VIEW_ALL_IMGS;
		} else {
			mode = FUJI_REMOTE_MODE;
		}
	} else if (fuji_known.unlocked_mode == FUJI_REMOTE_ACCESS) {
		mode = FUJI_REMOTE_MODE;
	} else {
		mode = FUJI_REMOTE_MODE;
	}

	ptp_verbose_log("Setting mode to %d\n", mode);

	rc = ptp_set_prop_value(r, PTP_PC_FUJI_Mode, mode);
	if (rc) return rc;

	rc = fuji_set_remote_version(r);
	if (rc) return rc;

	return 0;
}

int fuji_config_version(struct PtpRuntime *r) {
	// Get the FunctionVersion from the camera
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_FunctionVersion);
	if (rc) return rc;

	int version = ptp_parse_prop_value(r);

	fuji_known.function_version = version;

	// The property must be set again (to it's own value) to tell the camera
	// that the current version is supported - this may or may not be necessary
	rc = ptp_set_prop_value(r, PTP_PC_FUJI_FunctionVersion, version);
	if (rc) return rc;
	return 0;
}

// Very weird init routine seen on older cameras - this is really messed up,
// hopefully the camera doesn't expect this, but it might
int fuji_config_device_info_routine(struct PtpRuntime *r) {
	// Don't want remote capture
	return 0;
	if (fuji_known.function_version == 5) {
		// Begin camera remote
		int trans = r->transaction;
		int rc = ptp_init_open_capture(r, 0, 0);
		if (rc) return rc;

		rc = ptpip_fuji_get_events(r);
		if (rc) return rc;
		rc = ptpip_fuji_get_events(r);
		if (rc) return rc;

		rc = fuji_get_device_info(r);
		if (rc) return rc;

		rc = ptpip_fuji_get_events(r);
		if (rc) return rc;
		rc = ptpip_fuji_get_events(r);
		if (rc) return rc;

		rc = ptp_terminate_open_capture(r, trans);
		if (rc) return rc;
		rc = ptp_terminate_open_capture(r, trans);
		if (rc) return rc;
	} else if (fuji_known.function_version == 4) {
		int rc = fuji_get_device_info(r);
		if (rc) return rc;

		int trans = r->transaction;
		rc = ptp_init_open_capture(r, 0, 0);
		if (rc) return rc;
	}

	return 0;
}

// Very weird init routine seen on newer cameras
int fuji_config_remote_photo_viewer(struct PtpRuntime *r) {
	if (fuji_known.function_version == 5) {
		// Set unlocked to what it already is (does this make the camera happy?)
		int rc = ptp_get_prop_value(r, PTP_PC_FUJI_Unlocked);
		if (rc) return rc;
		int unlocked = ptp_parse_prop_value(r);
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_Unlocked, unlocked);
		if (rc) return rc;

		rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemotePhotoViewVersion);
		if (rc) return rc;

		// For X-T2, the value is 2
		// For X-A2, there is no payload given (-1)

		int view_version = ptp_parse_prop_value(r);
		if (view_version == -1) {
			// Camera doesn't have this property - return silently
			return 0;
		}

		ptp_verbose_log("PTP_PC_FUJI_RemotePhotoViewVersion: %d\n", view_version);

		// Set to photo viewer mode (?)
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_Mode, 11);
		if (rc) return rc;

		// Try to get the version again
		rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemotePhotoViewVersion);
		if (rc) return rc;

		view_version = ptp_parse_prop_value(r);

		ptp_verbose_log("PTP_PC_FUJI_RemotePhotoViewVersion: %d\n", view_version);

		// Assume the version is correct, verify it by setting
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_RemotePhotoViewVersion, view_version);
		if (rc) return rc;
		
	}

	return 0;
}

