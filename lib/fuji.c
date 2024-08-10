// Implements Fujifilm nonstandard PTP/IP implementation
// This is all portable code
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include "app.h"
#include "fuji.h"
#include "fujiptp.h"
#include "exif.h"

struct FujiDeviceKnowledge *fuji_get(struct PtpRuntime *r) {
	return (struct FujiDeviceKnowledge *)r->userdata;
}

int fuji_reset_ptp(struct PtpRuntime *r) {
	ptp_reset(r);
	// leaks, don't care
	r->userdata = calloc(1, sizeof(struct FujiDeviceKnowledge));
	r->connection_type = PTP_IP_USB;
	r->response_wait_default = 3; // Fuji cams are slow!
	r->io_kill_switch = 0;
	return 0;
}

void ptp_report_error(struct PtpRuntime *r, const char *reason, int code) {
	plat_dbg("Kill switch: %d\n", r->io_kill_switch);
	if (r->io_kill_switch) return;

	// Safely disconnect if intentional
	if (code == 0) {
		plat_dbg("Closing session");
		ptp_close_session(r);
	}

	r->io_kill_switch = 1;

	if (r->connection_type == PTP_IP_USB) {
		ptpip_close(r);
	} else if (r->connection_type == PTP_USB) {
		ptp_device_close(r);
	}

	fuji_reset_ptp(r); // TODO: this should only be called on connected

	if (reason == NULL) {
		if (code == PTP_IO_ERR) {
			app_print("Disconnected: IO Error");
		} else {
			app_print("Disconnected: Runtime error");
		}
	} else {
		app_print("Disconnected: %s", reason);
	}
}

// TODO: use this function
int fuji_connect_from_discoverinfo(struct PtpRuntime *r, struct DiscoverInfo *info) {
	fuji_reset_ptp(r);
	fuji_get(r)->transport = info->transport;
	int rc = ptpip_connect(r, info->camera_ip, info->camera_port);
	if (rc) {
		plat_dbg("Error connecting to %s:%d\n", info->camera_ip, info->camera_port);
		return rc;
	}
}

// Assumes cmd socket is valid
int fuji_setup(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);

	app_print("Waiting on the camera...");
	app_print("Make sure you pressed OK.");

	struct PtpFujiInitResp resp;
	int rc = ptpip_fuji_init_req(r, DEVICE_NAME, &resp);
	if (rc == PTP_RUNTIME_ERR) {
		rc = ptpip_fuji_init_req(r, DEVICE_NAME, &resp);
	}
	if (rc) {
		app_print("Failed to initialize connection");
		return rc;
	}
	app_print("Initialized connection.");

	app_send_cam_name(resp.cam_name);

	// Fuji cameras require delay after init
	if (fuji->transport == FUJI_FEATURE_WIRELESS_COMM) {
		app_print("The camera is thinking...");
		usleep(50000);
	}

	rc = ptp_open_session(r);
	if (rc) {
		app_print("Failed to open session.");
		return rc;
	}

	if (fuji->transport == FUJI_FEATURE_WIRELESS_TETHER) {
		return 0;
	}

	rc = fuji_wait_for_access(r);
	if (rc) {
		app_print("Failed to get access to the camera.");
		return rc;
	}

	// Remote cams don't need to wait for access, so waiting for the 'OK'
	// is done somewhere else
	if (fuji->camera_state != FUJI_REMOTE_ACCESS) {
		app_print("Your camera loves you.");
	}

	// NOTE: cFujiConfigInitMode *must* be called before fuji_config_version, or anything else.
	// If not, it will break up the connection and destroy packets for any file operation.
	rc = fuji_config_init_mode(r);
	if (rc) {
		app_print("Failed to setup the camera's mode");
		return rc;
	}

	if (fuji->camera_state == FUJI_MULTIPLE_TRANSFER) {
		return 0;
	}

	// Misnomer, should be config_image_viewer
	app_print("Setting up image viewer");
	rc = fuji_config_version(r);
	if (rc) {
		app_print("Failed to check versions.");
		return rc;
	}

	if (fuji->transport == FUJI_FEATURE_AUTOSAVE) {
		rc = fuji_enable_compression(r);
		if (rc) return rc;
	}

	rc = fuji_config_device_info_routine(r);
	if (rc) return rc;

	// Setup remote mode
	if (fuji->remote_version != -1 && fuji->camera_state == FUJI_REMOTE_MODE) {
		rc = fuji_setup_remote_mode(r);
		if (rc) return rc;
	}

	return 0;
}

int fuji_setup_remote_mode(struct PtpRuntime *r) {
	int rc = fuji_remote_mode_open_sockets(r);
	if (rc) {
		app_print("Failed to start remote mode");
		return rc;
	} else {
		app_print("Started remote mode.");
	}

	rc = fuji_get_events(r);
	if (rc) return rc;

	const char *ip = fuji_get(r)->ip_address;

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

	if (p->type == PTPIP_INIT_FAIL) {
		// Caller should resend the packet...
		return PTP_RUNTIME_ERR;
	}

	ptp_fuji_get_init_info(r, resp);
	ptp_verbose_log("Connected to %s\n", resp->cam_name);

	if (ptp_get_return_code(r) != 0) {
		return PTP_IO_ERR;
	}
	return 0;
}

int ptp_set_prop_value16(struct PtpRuntime *r, int code, uint16_t value) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SetDevicePropValue;
	cmd.param_length = 1;
	cmd.params[0] = code;

	uint16_t dat[] = {value};

	return ptp_generic_send_data(r, &cmd, dat, sizeof(dat));
}

int fuji_d228(void) {
	// Thing that PC AutoSave does
	//	char buffer[64];
	//	int s = 0;
	//	s += ptp_write_u8(buffer + s, 6);
	//	s += ptp_write_u32(buffer + s, 0x0020);
	//	s += ptp_write_u32(buffer + s, 0x0030);
	//	s += ptp_write_u32(buffer + s, 0x002f);
	//	s += ptp_write_u32(buffer + s, 0x0036);
	//	s += ptp_write_u32(buffer + s, 0x0030);
	//	s += ptp_write_u32(buffer + s, 0x0000);
	//
	//	ptp_set_prop_value_data(r, 0xd228, buffer, s);
}

struct MyAddInfo {
	struct PtpRuntime *r;
	int handle;
	uint8_t *buffer;
	int size;
};

static uint8_t *my_add(void *arg, uint8_t *buffer, int new_len, int old_len) {
	// Needs to be a new buffer
	ptp_verbose_log("downloading more exif %d %d", new_len, old_len);
	struct MyAddInfo *i = (struct MyAddInfo *)arg;
	i->buffer = realloc(i->buffer, new_len);

	int rc = ptp_get_partial_object(i->r, i->handle, old_len, new_len - old_len);
	if (rc) return NULL;

	memcpy(i->buffer + old_len, ptp_get_payload(i->r), ptp_get_payload_length(i->r));
	return i->buffer;
}

int ptp_get_partial_exif(struct PtpRuntime *r, int handle, int *offset, int *length) {
	ptp_mutex_keep_locked(r);

	int rc = fuji_get_events(r);
	if (rc) return rc;

	int max_size = 0x4000;
	rc = ptp_get_partial_object(r, handle, 0, max_size);
	if (rc == PTP_CHECK_CODE) {
		return PTP_RUNTIME_ERR;
	}
	if (rc) {
		ptp_mutex_unlock(r);
		return rc;
	}

	struct MyAddInfo temp = {
		.r = r,
		.handle = handle,
		.buffer = malloc(max_size),
		.size = max_size,
	};

	memcpy(temp.buffer, ptp_get_payload(r), ptp_get_payload_length(r));

	struct ExifC c = {0};
	c.length = ptp_get_payload_length(r);
	c.buf = temp.buffer;
	c.arg = &temp;
	c.get_more = my_add;

	plat_dbg("Exif reader: %d", exif_start_raw(&c));

	if (c.thumb_of == 0 || c.thumb_size == 0) {
		rc = PTP_RUNTIME_ERR;
		goto end;
	}

	*offset = c.thumb_of;
	*length = c.thumb_size;
	plat_dbg("Exif thumb offset: %u size: %u", c.thumb_of, c.thumb_size);

	// Given transfer speed/camera speed 5 event calls is generally enough for the camera
	// to not stop responding to object-related PTP commands
	rc = fuji_get_events(r);
	if (rc) goto end;
	rc = fuji_get_events(r);
	if (rc) goto end;
	rc = fuji_get_events(r);
	if (rc) goto end;
	rc = fuji_get_events(r);
	if (rc) goto end;

	end:;
	ptp_mutex_unlock(r);
	return rc;
}

// TODO: I named these functions completely backwards
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
	// TODO: Parsing should be done here
	return ptp_generic_send(r, &cmd);
}

static int fuji_tether_download(struct PtpRuntime *r) {
	struct PtpArray *a;
	int rc = ptp_get_object_handles(r, 0xffffffff, 0x0, 0x0, &a);
	if (rc) return rc;

	for (int i = 0; i < a->length; i++) {
		struct PtpObjectInfo oi;
		rc = ptp_get_object_info(r, a->data[i], &oi);
		if (rc) return rc;

		app_downloading_file(&oi);
		char buffer[256];
		app_get_file_path(buffer, oi.filename);
		FILE *f = fopen(buffer, "wb");
		if (f == NULL) return PTP_RUNTIME_ERR;
		ptp_download_object(r, a->data[i], f, 0x100000);
		fclose(f);

		app_downloaded_file(&oi, buffer);

		if (fuji_get(r)->transport == FUJI_FEATURE_WIRELESS_TETHER) {
			ptp_delete_object(r, a->data[i], 0x0);
		}
	}

	free(a);
	return 0;
}

int fuji_get_events(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	ptp_mutex_keep_locked(r);
	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_EventsList);
	if (rc == PTP_CHECK_CODE) {
		return 0;
	}
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
			fuji->selected_imgs_mode = ev->events[i].value;
			break;
		case PTP_PC_FUJI_ObjectCount:
			fuji->num_objects = ev->events[i].value;
			break;
		case PTP_PC_FUJI_CameraState:
			fuji->camera_state = ev->events[i].value;
			break;
		case PTP_PC_FUJI_FreeSDRAMImages:
			fuji_tether_download(r);
			break;
		}
	}

	ptp_mutex_unlock(r);

	return 0;
}

// Call this immediately after session init
int fuji_wait_for_access(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	// We *need* these properties on camera init - otherwise, produce an error
	fuji->camera_state = FUJI_WAIT_FOR_ACCESS;
	fuji->num_objects = -1;
	fuji->selected_imgs_mode = -1;

	while (1) {
		// After opening session, immediately get events
		int rc = fuji_get_events(r);
		if (rc) return rc;

		// Wait until camera state is unlocked
		if (fuji->camera_state != FUJI_WAIT_FOR_ACCESS) {
			if (fuji->selected_imgs_mode != -1) {
				// Multiple mode doesn't send num_objects
				return 0;
			} else {
				if (fuji->num_objects == -1) {
					ptp_verbose_log("Failed to get num_objects from first event\n");
					return PTP_RUNTIME_ERR;
				}
			}
			return 0;
		}

		CAMLIB_SLEEP(100);
	}
}

// Handles critical init sequence. This is after initializing the socket, and opening session.
// Called right after obtaining access to the device.
int fuji_config_init_mode(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);

	int rc = ptp_get_prop_value(r, PTP_PC_FUJI_GetObjectVersion);
	if (rc) return rc;
	fuji->get_object_version = ptp_parse_prop_value(r);
	ptp_verbose_log("GetObjectVersion: 0x%X\n", fuji->get_object_version);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteGetObjectVersion);
	if (rc) return rc;
	fuji->remote_image_view_version = ptp_parse_prop_value(r);
	ptp_verbose_log("RemoteGetObjectVersion: 0x%X\n", fuji->remote_image_view_version);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_ImageGetVersion);
	if (rc) return rc;
	fuji->image_get_version = ptp_parse_prop_value(r);
	ptp_verbose_log("ImageGetVersion: 0x%X\n", fuji->image_get_version);

	rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteVersion);
	if (rc) return rc;
	fuji->remote_version = ptp_parse_prop_value(r);
	ptp_verbose_log("RemoteVersion: 0x%X\n", fuji->remote_version);

	ptp_verbose_log("CameraState is %d\n", fuji->camera_state);

	// Determine preferred mode from state and version info
	int mode = 0;
	if (fuji->remote_version != -1) {
		mode = FUJI_REMOTE_MODE;
	} else {
		if (fuji->camera_state == FUJI_MULTIPLE_TRANSFER) {
			mode = FUJI_VIEW_MULTIPLE;
		} else if (fuji->camera_state == FUJI_FULL_ACCESS) {
			mode = FUJI_VIEW_ALL_IMGS;
		} else if (fuji->camera_state == FUJI_PC_AUTO_SAVE) {
			mode = FUJI_OLD_REMOTE;
		} else {
			mode = FUJI_VIEW_ALL_IMGS;
		}
	}

	ptp_verbose_log("Setting mode to %d\n", mode);

	// On newer cams, setting client state causes cam to have a dialog (Yes/No accept connection)
	// We have to wait for a response in this case
	r->wait_for_response = 255;

	rc = ptp_set_prop_value16(r, PTP_PC_FUJI_ClientState, mode);
	if (rc) return rc;

	return 0;
}


int fuji_config_version(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	int rc = 0;
	ptp_mutex_lock(r);
	if (fuji->camera_state == FUJI_PC_AUTO_SAVE) {
		rc = ptp_get_prop_value(r, PTP_PC_FUJI_AutoSaveVersion);
		if (rc) goto end;
		int code = ptp_parse_prop_value(r);
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_AutoSaveVersion, code);
		goto end;
	}
	if (fuji->remote_version == -1) {
		rc = ptp_get_prop_value(r, PTP_PC_FUJI_GetObjectVersion);
		if (rc) goto end;

		int version = ptp_parse_prop_value(r);

		fuji->image_view_version = version;

		// The property must be set again (to it's own value) to tell the camera
		// that the current version is supported - Fuji's app does this, so we assume it's necessary
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_GetObjectVersion, version);
		if (rc) goto end;
	} else {
		// Some cams set from 2000a to 2000b
		// Others set 20006 to 2000c (?)
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_RemoteVersion, FUJI_CAM_CONNECT_REMOTE_VER);
	}

	end:;
	ptp_mutex_unlock(r);
	return rc;
}

int fuji_config_device_info_routine(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	if (fuji->remote_version != -1 && fuji->camera_state != FUJI_PC_AUTO_SAVE) {
		int rc = fuji_get_device_info(r);
		if (rc) return rc;

		fuji_register_device_info(r, ptp_get_payload(r));

		// TODO: Parse device info
		// Only useful for later on when we want to do property setting/getting
	}

	return 0;
}

// Tell camera to open event/video sockets
int fuji_remote_mode_open_sockets(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	if (fuji->remote_version == -1) return 0;

	// Begin camera remote - (per spec, OpenCapture is much more broad than 'take picture')
	// This tells the camera to open the remote mode sockets (video/event)
	fuji->open_capture_trans_id = r->transaction;
	int rc = ptp_init_open_capture(r, 0, 0);
	if (rc) return rc;

	return 0;
}

// 'End' remote mode (or more like finish setup)
int fuji_remote_mode_end(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	if (fuji->remote_version == -1) return 0;

	// Right after remote mode is entered, camera gives off a bunch of properties
	int rc = fuji_get_events(r);
	if (rc) return rc;

	rc = ptp_terminate_open_capture(r, fuji->open_capture_trans_id);
	if (rc) return rc;

	return 0;
}

int fuji_config_image_viewer(struct PtpRuntime *r) {
	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	if (r->connection_type == PTP_USB) return 0;
	if (fuji->transport == FUJI_FEATURE_WIRELESS_TETHER) return 0;
	plat_dbg("remote_image_view_version: %X", fuji->remote_image_view_version);
	if (fuji->remote_image_view_version != -1) {
		// Tell the camera that we actually want that mode
		int rc = ptp_set_prop_value16(r, PTP_PC_FUJI_CameraState, FUJI_REMOTE_ACCESS);
		if (rc) return rc;

		// Will confirm CameraState is set
		rc = fuji_get_events(r);
		if (rc) return rc;

		rc = ptp_get_prop_value(r, PTP_PC_FUJI_RemoteGetObjectVersion);
		fuji->remote_image_view_version = ptp_parse_prop_value(r);
		if (rc) return rc;

		// Check SD card slot, not really useful for now
		//rc = ptp_get_prop_value(r, PTP_PC_FUJI_StorageID);
		//if (rc) return rc;
		//ptp_verbose_log("Storage ID: %d\n", ptp_parse_prop_value(r));

		// Now we finally enter the remote image viewer
		rc = ptp_set_prop_value16(r, PTP_PC_FUJI_ClientState, FUJI_MODE_REMOTE_IMG_VIEW);
		if (rc) return rc;

		// Set the prop higher - X-S10 and X-H1 want 4
		rc = ptp_set_prop_value(r, PTP_PC_FUJI_RemoteGetObjectVersion, 3);
		if (rc) return rc;

		// The props we set should show up here
		rc = fuji_get_events(r);
		if (rc) return rc;
	}

	return 0;
}

int fuji_import_all(struct PtpRuntime *r, int *object_ids, int length) {
	for (int i = 0; i < length; i++) {
		struct PtpObjectInfo oi;
		int rc = ptp_get_object_info(r, 1, &oi);
		if (rc) return rc;

		app_downloading_file(&oi);

		char path[256];
		app_get_file_path(path, oi.filename);
		FILE *f = fopen(path, "wb");
		if (f == NULL) return PTP_RUNTIME_ERR;

		rc = ptp_download_object(r, 1, f, 0x100000);
		fclose(f);
		if (rc) {
			app_print("Failed to save %s: %s", oi.filename, ptp_perror(rc));
			return rc;
		}

		if (app_check_thread_cancel()) {
			return 0;
		}

		app_downloaded_file(&oi, path);
	}

	return 0;
}

int fuji_download_classic(struct PtpRuntime *r) {
	while (1) {
		// This determines whether the connection is terminated or not
		struct PtpObjectInfo oi;
		int rc = ptp_get_object_info(r, 1, &oi);
		if (rc) return rc;

		app_downloading_file(&oi);

		char path[256];
		app_get_file_path(path, oi.filename);
		FILE *f = fopen(path, "wb");
		if (f == NULL) return PTP_RUNTIME_ERR;

		// Not sure if 0x100000 is required or not, but we'll do what Fuji is doing.
		rc = ptp_download_object(r, 1, f, 0x100000);
		fclose(f);
		if (rc) {
			app_print("Failed to save %s: %s", oi.filename, ptp_perror(rc));
			return rc;
		}

		app_downloaded_file(&oi, path);

		// Fuji's fujisystem will swap out object ID 1 with the next image. If there
		// are no more images, the camera shuts down the connection and turns off.

		// In other words, the camera is in superposition - it's on and off at the same time.
		// We don't know until we observe it:
		rc = fuji_get_events(r);
		if (rc) return 0;
	}
}
