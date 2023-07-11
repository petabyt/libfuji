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

// TODO: Construct a standard device info from bits and pieces of info

// Holds vital info about the camera
struct FujiDeviceKnowledge {
	int function_version;
}fuji_known;

// Handles critical init sequence. This is after initing the socket, and opening session.
// If this isn't called right after opening session, the camera will hang the connection.
int fuji_config_init_mode(struct PtpRuntime *r) {
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_Mode);
	if (rc) return rc;

#if 0
	// TODO: might need quirks for different modes preset by camera
	int mode = ptp_parse_prop_value(r);
	switch (mode) {
		case 1:
		case 2: // 2015
		case 3:
		case 4:
		case 5: // 2020
		// ...
		case 14: // latest mode (?)
	}
#endif

	// TODO: If mode > 5?
	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteVersion);
	if (rc) return rc;

	int remote_version = ptp_parse_prop_value(r);

	ptp_verbose_log("Version uint32 was %X\n", remote_version);

	// RemoteVersion is actually two words. The old app gave 11.2, so we'll try that
	uint16_t version[] = {
		11, 2
	};

	rc = ptp_set_prop_value_data(r, PTP_PC_FUJI_RemoteVersion, (void *)(&version), sizeof(version));
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
