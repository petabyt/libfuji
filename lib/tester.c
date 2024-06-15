// Fuji Test suite - this is dev code, it doesn't need to be tidy perfect
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include "app.h"
#include "fuji.h"
#include "fujiptp.h"

// Test suite verbose logging
void tester_log(char *fmt, ...);
void tester_fail(char *fmt, ...);

static void ptp_verbose_print_events(struct PtpRuntime *r) {
	struct PtpFujiEvents *ev = (struct PtpFujiEvents *)(ptp_get_payload(r));
	for (int i = 0; i < ev->length; i++) {
		tester_log("%X is %d", ev->events[i].code, ev->events[i].value);
	}
}

static void log_payload(struct PtpRuntime *r) {
	char buffer[512];
	uint8_t *data = ptp_get_payload(r);
	int length = ptp_get_payload_length(r);

	if (length == 0) {
		tester_log("No payload");
		return;
	}

	int c = sprintf(buffer, "Payload: ");
	for (int i = 0; i < length; i++) {
		c += sprintf(buffer + c, "%02X ", data[i]);
		if (c + 10 > sizeof(buffer)) break;
	}

	tester_log(buffer);
}

int fuji_test_get_props(struct PtpRuntime *r) {
	uint16_t test_props[] = {
		PTP_PC_FUJI_ImageGetVersion,
		PTP_PC_FUJI_GetObjectVersion,
		PTP_PC_FUJI_RemoteVersion,
		PTP_PC_FUJI_RemoteGetObjectVersion,
		PTP_PC_FUJI_ImageGetLimitedVersion,
		PTP_PC_FUJI_Unknown_D400,
		PTP_PC_FUJI_Unknown_D52F,
		PTP_PC_FUJI_Unknown_DF28,
	};

	for (int i = 0; i < (int)(sizeof(test_props) / sizeof(uint16_t)); i++) {
		ptp_verbose_log("Trying prop %x\n", test_props[i]);
		int rc = ptp_get_prop_value(r, test_props[i]);
		if (rc) {
			tester_fail("Err getting prop 0x%X - rc: %d", test_props[i], rc);
			return rc;
		} else {
			tester_log("Read property 0x%X %s (%d bytes)", test_props[i], ptp_get_enum_all(test_props[i]), ptp_get_payload_length(r));
		}

		log_payload(r);
	}

	return 0;
}

int fuji_test_init_access(struct PtpRuntime *r) {
	tester_log("Waiting for device access...");
	int rc = fuji_wait_for_access(r);
	if (rc) {
		tester_fail("Error trying to gain device access: %d", rc);
		return rc;
	} else {
		tester_log("Gained access to device (or already have access)");
	}

	return 0;
}

int fuji_init_setup(struct PtpRuntime *r) {
	tester_log("Configuring mode property");

	int rc = fuji_config_init_mode(r);
	if (rc) {
		tester_fail("Failed to setup mode: %d", rc);
		return rc;
	} else {
		tester_log("Mode property is configured.");
	}

	tester_log("Configuring version properties");
	rc = fuji_config_version(r);
	if (rc) {
		tester_fail("Failed to configure FunctionMode: %d", rc);
		return rc;
	} else {
		tester_log("Configured FunctionMode, no errors detected (yet)");
	}

	rc = fuji_config_device_info_routine(r);
	if (rc) {
		tester_fail("Failed to get device info");
		return rc;
	} else {
		tester_log("Received device info (or not supported)");
	}

	return 0;
}

int fuji_test_filesystem(struct PtpRuntime *r) {
	if (fuji_known.num_objects == 0) {
		tester_log("There are no images on the SD card!");
		return 0;
	}

	if (fuji_known.remote_version == -1) {
		if (fuji_known.selected_imgs_mode == FUJI_FULL_ACCESS) {
			if (fuji_known.num_objects == -1) {
				tester_fail(
						"The camera return didn't want to give access to num_objects property!");
				return 1;
			}

			tester_log("There are %d images on the SD card.", fuji_known.num_objects);
		} else if (fuji_known.selected_imgs_mode == FUJI_MULTIPLE_TRANSFER) {
			tester_log(
					"Camera is in multiple transfer mode. Doesn't tell us how many images there are.");
		} else if (fuji_known.selected_imgs_mode == -1) {
			tester_log("Camera is not in multiple transfer mode.");
		}
	}

	{ // test filesystem
		tester_log("Attempting to get object info for 1...");
		struct PtpObjectInfo oi;
		int rc = ptp_get_object_info(r, 1, &oi);
		if (rc) {
			tester_fail("Failed to get object info: %d", rc);
			return rc;
		} else {
			tester_log("Got object info");
		}

		char buffer[1024];
		ptp_object_info_json(&oi, buffer, sizeof(buffer));

		tester_log("Object info: %s", buffer);
		tester_log("Tag: '%s'", oi.keywords);

		tester_log("Trying to get thumbnail for 1...");
		rc = ptp_get_thumbnail(r, 1);
		if (rc) {
			tester_fail("Failed to get thumbnail: %d", rc);
		} else {
			tester_log("Got thumbnail: %u bytes", ptp_get_payload_length(r));
		}

		int offset, length;
		rc = ptp_dirty_rotten_thumb_hack(r, 1, &offset, &length);
		if (rc) {
			tester_fail("Failed to get dirty rotten thumb");
		} else {
			tester_log("Found EXIF thumb at %d %d", offset, length);
		}
	}

	return 0;
}

// Test the init/setup part of comms with the camera - once this finishes, connection is ready for stuff
int fuji_test_setup(struct PtpRuntime *r) {
	tester_log("Running test suite from C");

	struct PtpFujiInitResp resp;
	int rc = ptpip_fuji_init_req(r, DEVICE_NAME, &resp);
	if (rc) {
		tester_fail("Failed to initialize command socket");
		return rc;
	} else {
		tester_log("Initialized command socket");
	}

	tester_log("Connected to %s", resp.cam_name);

	tester_log("sleep 500ms for good measure...");
	CAMLIB_SLEEP(500);

	rc = ptp_open_session(r);
	if (rc) {
		tester_fail("Failed to open session");
		return rc;
	} else {
		tester_log("Opened session");
	}

	// This has already been tested extensively, no need
	rc = fuji_test_get_props(r);
	if (rc) return rc;

	rc = fuji_test_init_access(r);
	if (rc) return rc;

	rc = fuji_init_setup(r);
	if (rc) return rc;

	return 0;
}

int fuji_test_usb(struct PtpRuntime *r) {
	int rc = ptp_open_session(r);
	if (rc) return rc;

	struct PtpDeviceInfo di;
	rc = ptp_get_device_info(r, &di);
	if (rc) return rc;

	tester_log("Camera name: %s", di.model);

	struct PtpArray *arr;
	rc = ptp_get_storage_ids(r, &arr);
	if (rc) return rc;

	if (arr->length == 0) {
		tester_log("Camera has no SD card. It's okay.");
		return 0;
	}
	int id = arr->data[0];

	rc = ptp_get_object_handles(r, id, PTP_OF_JPEG, 0, &arr);
	if (rc) return rc;
	tester_log("Found %d objects", arr->length);

	// Check size

	tester_log("Downloading object of ID %X", arr->data[1]);
	rc = ptp_get_object(r, arr->data[1]);
	tester_fail("Return code: %x, %d", ptp_get_return_code(r), ptp_get_payload_length(r));
	if (rc) return rc;

	rc = ptp_close_session(r);
	if (rc) return rc;

	ptp_device_close(r);
	return 0;
}

int fuji_test_suite(struct PtpRuntime *r, const char *ip) {
	if (r->connection_type == PTP_USB) {
		return fuji_test_usb(r);
	}

	int rc = fuji_test_setup(r);
	if (rc) return rc;

	if (fuji_known.remote_version != -1) {
		rc = fuji_setup_remote_mode(r, ip);
		if (rc) return rc;
	}

	rc = fuji_config_image_viewer(r);
	if (rc) {
		tester_fail("Failed to config image viewer");
		return rc;
	} else {
		tester_log("Configured image viewer");
	}

	rc = fuji_test_filesystem(r);
	if (rc) return rc;

	return 0;
}
