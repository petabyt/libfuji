// Fujifilm WiFi connection library - this code is a portable extension to camlib.
// Don't add any iOS, JNI, or Dart stuff to it
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include "app.h"
#include "fuji.h"
#include "fujiptp.h"

struct FujiDeviceKnowledge fuji_known = {0};

int fuji_reset_ptp(struct PtpRuntime *r) {
	ptp_reset(r);
	r->connection_type = PTP_IP_USB;
	r->response_wait_default = 3; // Fuji cams are slow!

	return 0;
}

// Call after cmd socket is opened
int fuji_setup(struct PtpRuntime *r, char *ip) {
	memset(&fuji_known, 0, sizeof(struct FujiDeviceKnowledge));

	struct PtpFujiInitResp resp;
	int rc = ptpip_fuji_init_req(r, DEVICE_NAME, &resp);
	if (rc) {
		app_print("Failed to initialize connection");
		return rc;
	}
	app_print("Initialized connection.");

	ui_send_text("cam_name", resp.cam_name);

	// Fuji cameras require delay after init
	app_print("Waiting on camera..");
	usleep(50000);

	rc = ptp_open_session(r);
	if (rc) {
		app_print("Failed to open session.");
		return rc;
	}

	rc = fuji_wait_for_access(r);
	if (rc) {
		app_print("Failed to get access to the camera.");
		return rc;
	}

	// Remote cams don't need to wait for access, so waiting for the 'OK'
	// is done somewhere else
	if (fuji_known.camera_state != FUJI_REMOTE_ACCESS) {
		app_print("Gained access to the camera.");
	}

	// NOTE: cFujiConfigInitMode *must* be called before fuji_config_version, or anything else.
	// If not, it will break up the connection and destroy packets for any file operation.
	rc = fuji_config_init_mode(r);
	if (rc) {
		app_print("Failed to setup the camera's mode");
		return rc;
	}

	if (fuji_known.camera_state == FUJI_MULTIPLE_TRANSFER) {
		rc = fuji_download_multiple(r);
		if (rc) {
			app_print("Error downloading images");
			return rc;
		}
		app_print("Check your file manager app/gallery.");
		ptp_report_error(r, "Disconnected", 0);
		return 0;
	}

	// Misnomer, should be config_image_viewer
	rc = fuji_config_version(r);
	if (rc) {
		app_print("Failed to check versions.");
		return rc;
	}

//	rc = fuji_config_device_info_routine(r);
//	if (rc) return rc;

	// Setup remote mode
	if (fuji_known.remote_version != -1) {
		rc = fuji_setup_remote_mode(r, ip);
		if (rc) return rc;
	}

	return 0;
}

int fuji_setup_remote_mode(struct PtpRuntime *r, char *ip) {
	int rc = fuji_remote_mode_open_sockets(r);
	if (rc) {
		app_print("Failed to start remote mode");
		return rc;
	} else {
		app_print("Started remote mode.");
	}

	rc = ptpip_connect_events(r, ip, FUJI_EVENT_IP_PORT);
	if (rc) return rc;

	rc = ptpip_connect_video(r, ip, FUJI_LIVEVIEW_IP_PORT);
	if (rc) return rc;

	rc = fuji_remote_mode_end(r);
	if (rc) {
		app_print("Failed to finish remote setup.");
		return rc;
	} else {
		app_print("Finished remote setup.");
	}

	return 0;
}

int ptpip_fuji_init_req(struct PtpRuntime *r, char *device_name, struct PtpFujiInitResp *resp) {
	struct FujiInitPacket *p = (struct FujiInitPacket *)r->data;
	memset(p, 0, sizeof(struct FujiInitPacket));
	p->length = 0x52;
	p->type = PTPIP_INIT_COMMAND_REQ;

	p->version = FUJI_PROTOCOL_VERSION;

	p->guid1 = 0x5d48a5ad;
	p->guid2 = 0xb7fb287;
	p->guid3 = 0xd0ded5d3;
	p->guid4 = 0x0;

	ptp_write_unicode_string(p->device_name, device_name);

	if (ptpip_cmd_write(r, r->data, p->length) != p->length) return PTP_IO_ERR;

	// Read the packet size, then receive the rest
	int x = ptpip_cmd_read(r, r->data, 4);
	if (x < 0) return PTP_IO_ERR;
	x = ptpip_cmd_read(r, r->data + 4, p->length - 4);
	if (x < 0) return PTP_IO_ERR;

	ptp_fuji_get_init_info(r, resp);

	if (ptp_get_return_code(r) == 0x0) {
		return 0;
	} else {
		return PTP_IO_ERR;
	}
}

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
	ptp_mutex_keep_locked(r);
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_EventsList);
	if (rc) {
		ptp_mutex_unlock(r);
		return rc;
	}

	struct PtpFujiEvents *ev = (struct PtpFujiEvents *)(ptp_get_payload(r));

	uint16_t length, code;
	uint32_t value;

	ptp_read_u16(&ev->length, &length);

	ptp_verbose_log("Found %d events\n", ev->length);
	for (int i = 0; i < ev->length; i++) {
		ptp_read_u16(&ev->events[i].code, &code);
		ptp_read_u32(&ev->events[i].value, &value);
		ptp_verbose_log("%X changed to %d\n", code, value);
	}

	for (int i = 0; i < ev->length; i++) {
		ptp_read_u16(&ev->events[i].code, &code);
		ptp_read_u32(&ev->events[i].value, &value);
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

	ptp_mutex_unlock(r);

	return 0;
}

// Call this immediately after session init
int fuji_wait_for_access(struct PtpRuntime *r) {
	// We *need* these properties on camera init - otherwise, produce an error
	fuji_known.camera_state = FUJI_WAIT_FOR_ACCESS;
	fuji_known.num_objects = -1;
	fuji_known.selected_imgs_mode = -1;

	while (1) {
		// After opening session, immediately get events
		int rc = fuji_get_events(r);
		if (rc) return rc;

		// Wait until camera state is unlocked
		if (fuji_known.camera_state != FUJI_WAIT_FOR_ACCESS) {
			if (fuji_known.selected_imgs_mode != -1) {
				// Multiple mode doesn't send num_objects
				return 0;
			} else {
				if (fuji_known.num_objects == -1) {
					ptp_verbose_log("Failed to get num_objects from first event\n");
					return PTP_RUNTIME_ERR;
				}
			}
			return 0;
		}

		CAMLIB_SLEEP(100);
	}
}

// Handles critical init sequence. This is after initing the socket, and opening session.
// Called right after obtaining access to the device.
int fuji_config_init_mode(struct PtpRuntime *r) {
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_GetObjectVersion);
	if (rc) return rc;
	fuji_known.get_object_version = ptp_parse_prop_value(r);
	ptp_verbose_log("GetObjectVersion: 0x%X", fuji_known.get_object_version);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteGetObjectVersion);
	if (rc) return rc;
	fuji_known.remote_image_view_version = ptp_parse_prop_value(r);
	ptp_verbose_log("RemoteGetObjectVersion: 0x%X", fuji_known.remote_image_view_version);

	// TODO: set PTP_PC_FUJI_RemoteGetObjectVersion

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_ImageGetVersion);
	if (rc) return rc;
	fuji_known.image_get_version = ptp_parse_prop_value(r);
	ptp_verbose_log("ImageGetVersion: 0x%X", fuji_known.image_get_version);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteVersion);
	if (rc) return rc;
	fuji_known.remote_version = ptp_parse_prop_value(r);
	ptp_verbose_log("RemoteVersion: 0x%X", fuji_known.remote_version);

	ptp_verbose_log("CameraState is %d", fuji_known.camera_state);

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

	ptp_verbose_log("Setting mode to %d", mode);

	// On newer cams, setting function mode causes cam to have a dialog (Yes/No accept connection)
	// We have to wait for a response in this case
	r->wait_for_response = 255;

	rc = ptp_set_prop_value16(r, PTP_PC_FUJI_FunctionMode, mode);
	if (rc) return rc;

	return 0;
}

// TODO: rename config image view version
int fuji_config_version(struct PtpRuntime *r) {
	if (fuji_known.remote_version == -1) {
		int rc = ptp_get_prop_value(r, PTP_PC_FUJI_GetObjectVersion);
		if (rc) return rc;

		int version = ptp_parse_prop_value(r);

		fuji_known.image_view_version = version;

		// The property must be set again (to it's own value) to tell the camera
		// that the current version is supported - Fuji's app does this, so we assume it's necessary
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_GetObjectVersion, version);
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

	// Begin camera remote - (per spec, OpenCapture is much more broad than 'take picture')
	// This tells the camera to open the remote mode sockets (video/event)
	fuji_known.open_capture_trans_id = r->transaction;
	int rc = ptp_init_open_capture(r, 0, 0);
	if (rc) return rc;

	return 0;
}

// 'End' remote mode (or more like finish setup)
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

		rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteGetObjectVersion);
		fuji_known.remote_image_view_version = ptp_parse_prop_value(r);
		if (rc) return rc;

		// Check SD card slot, not really useful for now
		rc = ptp_get_prop_value(r, PTP_PC_FUJI_StorageID);
		if (rc) return rc;
		ptp_verbose_log("Storage ID: %d\n", ptp_parse_prop_value(r));

		// Now we finally enter the remote image viewer
		rc = ptp_set_prop_value16(r, PTP_PC_FUJI_FunctionMode, FUJI_MODE_REMOTE_IMG_VIEW);
		if (rc) return rc;

		// Set the prop higher - X-S10 and X-H1 want 4
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_RemoteGetObjectVersion, 4);
		if (rc) return rc;

		// The props we set should show up here
		rc = fuji_get_events(r);
		if (rc) return rc;
	}

	return 0;
}
