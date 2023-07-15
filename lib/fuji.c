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

// TODO: Construct a standard device info from bits and pieces of info

struct FujiDeviceKnowledge fuji_known = {0};

int fuji_get_device_info(struct PtpRuntime *r) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_FUJI_GetDeviceInfo;
	cmd.param_length = 0;

	return ptp_generic_send(r, &cmd);
}

// Handles critical init sequence. This is after initing the socket, and opening session.
// If this isn't called right after opening session, the camera will hang the connection.
int fuji_config_init_mode(struct PtpRuntime *r) {
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_Mode);
	if (rc) return rc;
	int mode = ptp_parse_prop_value(r);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_FunctionVersion);
	if (rc) return rc;
	int version = ptp_parse_prop_value(r);

	fuji_known.function_version = version;

	// FunctionVersion might be a misnomer for earliest supported Mode (protocol behavior)
	rc = ptp_set_prop_value(r, PTP_PC_FUJI_Mode, version);
	if (rc) return rc;

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteVersion);
	if (rc) return rc;

	int remote_version = ptp_parse_prop_value(r);

	ptp_verbose_log("Version uint32 was %X\n", remote_version);

	// RemoteVersion is actually two words. The old app gave 11.2, so we'll try that
	uint16_t new_remote_version[] = {
		11, 2
	};

	rc = ptp_set_prop_value_data(r, PTP_PC_FUJI_RemoteVersion,
		(void *)(&new_remote_version), sizeof(new_remote_version));
	if (rc) return rc;

	return 0;
}

int fuji_config_version(struct PtpRuntime *r) {
	// Get the FunctionVersion from the camera - 
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_FunctionVersion);
	if (rc) return rc;

	int version = ptp_parse_prop_value(r);

	fuji_known.function_version = version;

	// The property must be set again (to it's own value) to tell the camera
	// that the current version is supported.
	rc = ptp_set_prop_value(r, PTP_PC_FUJI_FunctionVersion, version);
	if (rc) return rc;
	return 0;
}

// Very weird init routine seen on older cameras
int fuji_config_device_info_routine(struct PtpRuntime *r) {
	if (fuji_known.function_version > 3) {
		int rc = fuji_get_device_info(r);
		if (rc) return rc;

		ptp_verbose_log("Device info: length:%d\n", ptp_get_payload_length(r));

		int trans = r->transaction;
		rc = ptp_init_open_capture(r, 0, 0);
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
	}

	return 0;
}

// Very weird init routine seen on older cameras
int fuji_config_remote_photo_viewer(struct PtpRuntime *r) {
	if (fuji_known.function_version > 3) {
		// Set unlocked to what it already is (does this make the camera happy?)
		int rc = ptp_get_prop_value(r, PTP_PC_FUJI_Unlocked);
		if (rc) return rc;
		int unlocked = ptp_parse_prop_value(r);
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_Unlocked, unlocked);
		if (rc) return rc;

		rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemotePhotoViewVersion);
		if (rc) return rc;

		int view_version = ptp_parse_prop_value(r);
		if (view_version == -1) {
			return 0;
		}

		ptp_verbose_log("PTP_PC_FUJI_RemotePhotoViewVersion: %d\n", view_version);

		// For X-T2, the value is 2
		// For X-A2, there is no payload given (-1)

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

