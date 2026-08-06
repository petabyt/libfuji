// Implements Fujifilm nonstandard PTP/IP implementation
// This is all portable code
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libpict.h>
#include <sys/stat.h>
#include "app.h"
#include "fuji.h"
#include "fujiptp.h"
#include "exif.h"

fujipriv_t *fuji_get(struct PtpRuntime *r) {
	return r->priv;
}

int fuji_reset_ptp(struct PtpRuntime *r) {
	ptp_reset(r);
	if (r->priv == NULL) free(r->priv);
	r->priv = calloc(1, sizeof(struct PtpUserPriv));
	r->connection_type = PTP_IP_USB;
	r->response_wait_default = 3; // Fuji cams are slow!
	return 0;
}

struct PtpRuntime *fuji_ptp_new(int options) {
	struct PtpRuntime *r = ptp_new(options);
	fuji_reset_ptp(r);
	return r;
}

void ptp_report_error(struct PtpRuntime *r, const char *reason, int code) {
	if (r->io_kill_switch) return;
	ptp_mutex_lock(r);
	if (r->io_kill_switch) {
		ptp_mutex_unlock(r);
		return;
	}

	// Safely disconnect if intentional
	if (code == 0) {
		ptp_verbose_log("Closing session\n");
		ptp_close_session(r);
	}

	r->operation_kill_switch = 1;

	ptp_verbose_log("Goodbye\n");

	if (r->connection_type == PTP_IP_USB) {
		// Send Fuji's 'goodbye' packet - we don't care if this fails or not
		uint8_t goodbye_packet[] = {0x8, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff};
		ptpip_cmd_write(r, goodbye_packet, 8);

		ptpip_device_close(r);
	} else if (r->connection_type == PTP_USB) {
		ptp_device_close(r);
	}

	r->io_kill_switch = 1;

	ptp_mutex_unlock(r);

	if (reason == NULL) {
		if (code == PTP_IO_ERR) {
			app_print(r, "Disconnected: IO Error");
		} else {
			app_print(r, "Disconnected: Runtime error");
		}
	} else {
		app_print(r, "Disconnected: %s", reason);
	}
}

int fuji_get_device_info(struct PtpRuntime *r) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_FUJI_GetDeviceInfo;
	cmd.param_length = 0;
	// TODO: Parsing should be done here
	return ptp_send(r, &cmd);
}

static int fuji_tether_download(struct PtpRuntime *r) {
	struct PtpArray *a;
	int rc = ptp_get_object_handles(r, -1, 0x0, 0x0, &a);
	if (rc) return rc;

	for (int i = 0; i < (int)a->length; i++) {
		// oi.filename will always be DSCF0001.JPG
		struct PtpObjectInfo oi;
		rc = ptp_get_object_info(r, a->data[i], &oi);
		if (rc) return rc;

		app_downloading_file(r, &oi);

		// TODO: rewrite
#if 0
		char buffer[256];
		app_get_tether_file_path(r, buffer);
		FILE *f = fopen(buffer, "wb");
		if (f == NULL) return PTP_RUNTIME_ERR;
		app_print(r, "Downloading %s", buffer);
		ptp_download_object(r, (int)a->data[i], f, 0x100000);
		fclose(f);

		app_downloaded_file(r, &oi, buffer);
#endif

		if (fuji_get(r)->transport == FUJI_FEATURE_WIRELESS_TETHER) {
			ptp_delete_object(r, (int)a->data[i]);
		}

		app_print(r, "Done downloading.");
	}

	free(a);
	return 0;
}

int fuji_get_events(struct PtpRuntime *r) {
	fujipriv_t *fuji = fuji_get(r);
	ptp_mutex_lock(r);
	int rc = ptp_get_prop_value(r, PTP_DPC_FUJI_EventsList_D212);
	if (rc == PTP_CHECK_CODE) {
		ptp_mutex_unlock(r);
		return 0;
	} else if (rc) {
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
		ptp_verbose_log("%X changed to %X\n", code, value);
	}

	for (int i = 0; i < ev->length; i++) {
		ptp_read_u16(&ev->events[i].code, &code);
		ptp_read_u32(&ev->events[i].value, &value);
		switch (ev->events[i].code) {
		case PTP_DPC_FUJI_SelectedImgsMode_D220:
			fuji->selected_imgs_mode = (int)ev->events[i].value;
			break;
		case PTP_DPC_FUJI_ObjectCount_D2222:
			fuji->num_objects = (int)ev->events[i].value;
			break;
		case PTP_DPC_FUJI_CameraState_DF00:
			fuji->camera_state = (int)ev->events[i].value;
			break;
		case PTP_DPC_FUJI_FreeSDRAMImages:
			if (fuji->transport == FUJI_FEATURE_WIRELESS_TETHER)
				fuji_tether_download(r);
			break;
		}
	}

	ptp_mutex_unlock(r);

	return 0;
}

// Set PTP_DPC_FUJI_ClientState_DF01/0xdf01
int fuji_config_init_mode(struct PtpRuntime *r) {
	fujipriv_t *fuji = fuji_get(r);

	// Determine preferred mode from state and version info
	int mode = 0;
	if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		mode = FUJI_MODE_REMOTE_IMG_VIEW_XAPP;
	} else {
		if (fuji->remote_version != -1 || fuji->camera_state == FUJI_REMOTE_ACCESS) {
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
	}

	ptp_verbose_log("Setting mode to %d\n", mode);

	// On newer cams, setting client state causes cam to open a dialog (Yes/No accept connection)
	// We have to wait for a response in this case
	r->wait_for_response = 255;

	int rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_ClientState_DF01, mode);
	if (rc) return rc;

	rc = fuji_get_events(r);
	if (rc) return rc;

	return 0;
}

static int fuji_config_version_(struct PtpRuntime *r) {
	fujipriv_t *fuji = fuji_get(r);
	if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) return 0;
	int rc = 0;
	if (fuji->camera_state == FUJI_PC_AUTO_SAVE) {
		rc = ptp_get_prop_value(r, PTP_DPC_FUJI_AutoSaveVersion_DF23);
		if (rc) return rc;
		int code = ptp_parse_prop_value(r);
		rc = ptp_set_prop_value(r, PTP_DPC_FUJI_AutoSaveVersion_DF23, code);
		if (rc) return rc;
	} else if (fuji->remote_version == -1) {
		rc = ptp_get_prop_value(r, PTP_DPC_FUJI_GetObjectVersion_DF22);
		if (rc) return rc;
		int version = ptp_parse_prop_value(r);
		//fuji->image_view_version = version;
		rc = ptp_set_prop_value(r, PTP_DPC_FUJI_GetObjectVersion_DF22, version);
		if (rc) return rc;
	} else {
		// Some cams set from 2000a to 2000b
		// Others set 20006 to 2000c (?)
		// X-T20 has 20004
		// Setting to the highest (supported?) value (2000c) seems to be what Fuji does
		rc = ptp_set_prop_value(r, PTP_DPC_FUJI_RemoteVersion_DF24, FUJI_CAM_CONNECT_REMOTE_VER);
		if (rc) return rc;

#if 0
		// Don't understand this object yet - has some kind of important data
		struct PtpObjectInfo oi;
		rc = ptp_get_object_info(r, 0xfffffff1, &oi);
		if (rc == PTP_CHECK_CODE) {
			ptp_verbose_log("Didn't get valid info for 0xfffffff1\n");
		} else if (rc) {
			return rc;
		} else {
			char buffer[512];
			ptp_object_info_json(&oi, buffer, sizeof(buffer));
			ptp_verbose_log("0xfffffff1: %s\n", buffer);
		}
#endif

	}

	return 0;
}
int fuji_config_version(struct PtpRuntime *r) {
	ptp_mutex_lock(r);
	int rc = fuji_config_version_(r);
	ptp_mutex_unlock(r);
	return rc;
}

int fuji_config_device_info_routine(struct PtpRuntime *r) {
	fujipriv_t *fuji = fuji_get(r);
	if (fuji->remote_version != -1 && fuji->camera_state != FUJI_PC_AUTO_SAVE && fuji->transport != FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		int rc = fuji_get_device_info(r);
		if (rc) return rc;

		fuji_register_device_info(r, ptp_get_payload(r));

		// TODO: Parse device info
		// Only useful for later on when we want to do property setting/getting
	}

	return 0;
}

static int fuji_xapp_config_gallery(struct PtpRuntime *r) {
	int rc;
	fujipriv_t *fuji = fuji_get(r);
	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_RemotePhotoViewExVersion_DF28);
	if (rc) return rc;
	int val = ptp_parse_prop_value(r);
	fuji->remote_image_view_ex_version = ptp_parse_prop_value(r);
	ptp_verbose_log("RemotePhotoViewExVersion: 0x%X\n", fuji->remote_image_view_ex_version);
	rc = ptp_set_prop_value(r, PTP_DPC_FUJI_RemotePhotoViewExVersion_DF28, val);
	if (rc) return rc;

	rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_CompressSmall_D226, 0);
	if (rc) return rc;
	rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_EnableCorrectFileSize_D227, 0);
	if (rc) return rc;

	struct PtpCommand cmd;
	cmd.code = PTP_OC_FUJI_GetExtensionObjectInfo;
	cmd.param_length = 1;
	cmd.params[0] = 0x10000001;
	rc = ptp_send(r, &cmd);
	if (rc) return rc;

	cmd.code = PTP_OC_FUJI_GetExtensionThumb;
	cmd.param_length = 1;
	cmd.params[0] = 0x10000001;
	rc = ptp_send(r, &cmd);
	if (rc) return rc;

	cmd.code = PTP_OC_FUJI_GetImageImportFolders;
	cmd.param_length = 0;
	rc = ptp_send(r, &cmd);
	if (rc) return rc;

	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_Unknown_D22B);
	if (rc) return rc;

	cmd.code = PTP_OC_FUJI_GetImageImportDates;
	cmd.param_length = 2;
	cmd.params[0] = 0;
	cmd.params[1] = 30000;
	rc = ptp_send(r, &cmd);
	if (rc) return rc;

	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_ImageImportObjectCount);
	if (rc) return rc;
	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_ImageImportObjectHandles);
	if (rc) return rc;
	return 0;
}

// Assumes cmd socket is valid
int fuji_setup(struct PtpRuntime *r, const char *client_name) {
	fujipriv_t *fuji = fuji_get(r);

	app_print(r, "Waiting on the camera...");
	app_print(r, "Make sure you pressed OK.");

	struct PtpFujiInitResp resp;
	int rc = ptpip_fuji_connect_handshake(r, client_name, &resp);
	if (rc == PTP_RUNTIME_ERR) {
		rc = ptpip_fuji_connect_handshake(r, client_name, &resp);
	} else if (rc) {
		usleep(1000); // One last chance...
		rc = ptpip_fuji_connect_handshake(r, client_name, &resp);
	}
	if (rc) {
		app_print(r, "Failed to initialize connection (%d)", rc);
		return rc;
	}
	app_print(r, "Initialized connection.");

	app_send_cam_name(r, resp.cam_name);

	if (fuji->transport == FUJI_FEATURE_WIRELESS_COMM || fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		app_print(r, "The camera is thinking...");
		usleep(50000); // Fuji cameras require at least 50ms before OpenSession
	}

	rc = ptp_open_session(r);
	if (rc == PTP_CHECK_CODE) {
		if (ptp_get_return_code(r) != PTP_RC_SessionAlreadyOpened) {
			return rc;
		}
	} else if (rc) {
		app_print(r, "Failed to open session.");
		// TODO: Maybe close and reopen socket
		return rc;
	}

	if (fuji->transport == FUJI_FEATURE_WIRELESS_TETHER) {
		// Wireless tether uses USB style communication
		return 0;
	}

	// These properties must be updated on the first get_events call
	// num_objects will not be updated in FUJI_MULTIPLE_TRANSFER mode
	fuji->camera_state = -1;
	fuji->num_objects = -1;
	fuji->selected_imgs_mode = -1;
	rc = fuji_get_events(r);
	if (rc) return rc;

	if (fuji->camera_state == FUJI_WAIT_FOR_ACCESS) {
		app_print(r, "Press OK to allow access.");
			while (fuji->camera_state == FUJI_WAIT_FOR_ACCESS) {
			rc = fuji_get_events(r);
			if (rc) return rc;
			PTP_SLEEP(100);
		}
	} else {
		app_print(r, "Your camera loves you.");
	}

	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_GetObjectVersion_DF22);
	if (rc) return rc;
	fuji->get_object_version = ptp_parse_prop_value(r);
	ptp_verbose_log("PTP_DPC_FUJI_GetObjectVersion_DF22: 0x%X\n", fuji->get_object_version);

	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_RemoteGetObjectVersion_DF25);
	if (rc) return rc;
	fuji->remote_image_view_version = ptp_parse_prop_value(r);
	ptp_verbose_log("PTP_DPC_FUJI_RemoteGetObjectVersion_DF25: 0x%X\n", fuji->remote_image_view_version);

	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_ImageGetVersion_DF21);
	if (rc) return rc;
	fuji->image_get_version = ptp_parse_prop_value(r);
	ptp_verbose_log("PTP_DPC_FUJI_ImageGetVersion_DF21: 0x%X\n", fuji->image_get_version);

	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_RemoteVersion_DF24);
	if (rc) return rc;
	fuji->remote_version = ptp_parse_prop_value(r);
	ptp_verbose_log("PTP_DPC_FUJI_RemoteVersion_DF24: 0x%X\n", fuji->remote_version);

	rc = fuji_config_init_mode(r);
	if (rc) {
		ptp_verbose_log("fuji_config_init_mode: %d\n", rc);
		app_print(r, "Failed to setup the camera's mode");
		return rc;
	}

	if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		// when xapp enters the gallery for the first time, 0xdf00 isn't set.
		// It is set when otherwise switching between liveview and gallery.
		rc = fuji_xapp_config_gallery(r);
		if (rc && rc != PTP_CHECK_CODE) return rc;
	}

	if (fuji->camera_state == FUJI_MULTIPLE_TRANSFER) {
		return 0;
	}

	app_print(r, "Configuring version properties");
	rc = fuji_config_version(r);
	if (rc) {
		app_print(r, "fuji_config_version()");
		return rc;
	}

	if (fuji->transport == FUJI_FEATURE_AUTOSAVE) {
		rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_EnableCorrectFileSize_D227, 1);
		if (rc) return rc;
	}

	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_StorageID);
	if (rc == 0) {
		sprintf(fuji->storage_device_name, "Card %d", ptp_parse_prop_value(r));
	} else if (rc == PTP_CHECK_CODE) {
		strcpy(fuji->storage_device_name, "Card");
	} else if (rc) {
		return rc;
	}

	if (fuji->remote_version != -1 && fuji->camera_state == FUJI_REMOTE_ACCESS && fuji->transport != FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		// Start and end liveview mode - not sure why this is needed, but IIRC camera stops responding if this isn't done during setup
		app_print(r, "Configuring remote mode");
		rc = fuji_config_liveview(r);
		if (rc) return rc;
		rc = fuji_end_liveview(r);
		if (rc) return rc;

		app_print(r, "Setting up image gallery");
		rc = fuji_config_image_gallery(r);
		if (rc) return rc;
	}

	return 0;
}

static int ptpip_fuji_connect_handshake_(struct PtpRuntime *r, const char *device_name, struct PtpFujiInitResp *resp) {
	struct FujiInitPacket *p = (struct FujiInitPacket *)r->data;
	memset(p, 0, sizeof(struct FujiInitPacket));
	ptp_write_u32(&p->length, 0x52);
	ptp_write_u32(&p->type, PTPIP_INIT_COMMAND_REQ);
	ptp_write_u32(&p->version, FUJI_PROTOCOL_VERSION);

	ptp_write_u32(&p->guid1, 0x5d48a5ad);
	ptp_write_u32(&p->guid2, 0xb7fb287);
	ptp_write_u32(&p->guid3, 0xd0ded5d3);
	ptp_write_u32(&p->guid4, 0x0);

	ptp_write_unicode_string(p->device_name, device_name);

	if (ptpip_cmd_write(r, r->data, (int)p->length) != (int)p->length) {
		ptp_verbose_log("First TCP packet failed");
		return PTP_IO_ERR;
	}

	// Read the packet size, then receive the rest
	int x = ptpip_cmd_read(r, r->data, 4);
	if (x < 0) return PTP_IO_ERR;
	x = ptpip_cmd_read(r, r->data + 4, (int)p->length - 4);
	if (x < 0) return PTP_IO_ERR;

	if (p->type == PTPIP_INIT_FAIL) {
		ptp_verbose_log("PTPIP_INIT_FAIL\n");
		return PTP_RUNTIME_ERR;
	}

	ptp_fuji_parse_init_struct(r, resp);
	ptp_verbose_log("Connected to %s\n", resp->cam_name);

	if (ptp_get_return_code(r) != 0) {
		return PTP_IO_ERR;
	}

	return 0;
}
int ptpip_fuji_connect_handshake(struct PtpRuntime *r, const char *device_name, struct PtpFujiInitResp *resp) {
	ptp_mutex_lock(r);
	int rc = ptpip_fuji_connect_handshake_(r, device_name, resp);
	ptp_mutex_unlock(r);
	return rc;
}

struct MyAddInfo {
	struct PtpRuntime *r;
	int handle;
	uint8_t *buffer;
	int size;
};

static uint8_t *my_add(void *arg, uint8_t *buffer, unsigned int new_len, unsigned int old_len) {
	ptp_verbose_log("Downloading more exif data %d -> %d\n", new_len, old_len);

	struct MyAddInfo *i = (struct MyAddInfo *)arg;
	i->buffer = realloc(i->buffer, new_len);
	if (i->buffer == NULL) abort();

	int rc = ptp_get_partial_object(i->r, i->handle, old_len, new_len - old_len);
	if (rc) return NULL;

	memcpy(i->buffer + old_len, ptp_get_payload(i->r), ptp_get_payload_length(i->r));
	return i->buffer;
}

// Experimental thumbnails in PC AutoSave mode hack
static int ptp_get_partial_exif(struct PtpRuntime *r, int handle, unsigned int *offset, unsigned int *length) {
	ptp_mutex_lock(r);

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

	struct ExifParser c = {0};
	rc = exif_start_raw(&c, temp.buffer, ptp_get_payload_length(r), my_add, &temp);
	ptp_verbose_log("exif_start_raw: %d\n", rc);

	if (c.thumb_of == 0 || c.thumb_size == 0) {
		rc = PTP_RUNTIME_ERR;
		goto end;
	}

	*offset = c.thumb_of;
	*length = c.thumb_size;
	ptp_verbose_log("Exif thumb offset: %u size: %u\n", c.thumb_of, c.thumb_size);

	// Some timing issues have to be worked around to make this work.
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

int fuji_get_thumb(struct PtpRuntime *r, int handle, unsigned int *offset, unsigned int *length) {
	(*offset) = 0;
	(*length) = 0;
	if (fuji_get(r)->transport == FUJI_FEATURE_AUTOSAVE) {
		if (r->priv->allow_autosave_thumbnails) {
			return ptp_get_partial_exif(r, handle, offset, length);
		} else {
			return 0;
		}
	} else {
		// get_thumb blocks forever unless get_object_info is called on newer cameras
		if (fuji_get(r)->remote_version > 0x20006) {
			struct PtpObjectInfo oi;
			int rc = ptp_get_object_info(r, (int) handle, &oi);
			if (rc == PTP_CHECK_CODE) return 0;
			if (rc) return rc;

			plat_update_object_info(r, handle, &oi);
		}

		int rc = ptp_get_thumbnail(r, (int)handle);
		if (rc == PTP_CHECK_CODE) {
			ptp_verbose_log("Thumbnail get failed: %x\n", ptp_get_return_code(r));
			return 0;
		} else if (rc) {
			return rc;
		} else if (ptp_get_payload_length(r) < 100) {
			return 0; // Weird situation, thumbnail is too small.
		} else {
			(*offset) = 0x0;
			(*length) = ptp_get_payload_length(r);
		}
		return 0;
	}
}

int fuji_end_liveview(struct PtpRuntime *r) {
	fujipriv_t *fuji = fuji_get(r);
	if (fuji->remote_version == -1) return 0;

	int rc = fuji_get_events(r);
	if (rc) return rc;

	rc = ptp_terminate_open_capture(r, fuji->open_capture_trans_id);
	if (rc) return rc;

	return 0;
}

int fuji_config_liveview(struct PtpRuntime *r) {
	int rc;
	fujipriv_t *fuji = fuji_get(r);

	if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_CameraState_DF00, FUJI_REMOTE_ACCESS);
		if (rc) return rc;
		rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_ClientState_DF01, FUJI_MODE_REMOTE_LIVE_VIEW_XAPP);
		if (rc) return rc;

		rc = ptp_get_prop_value(r, PTP_DPC_FUJI_Unknown_DF2A);
		if (rc) return rc;
		rc = ptp_set_prop_value(r, PTP_DPC_FUJI_Unknown_DF2A, ptp_parse_prop_value(r));
		if (rc) return rc;
	}

	// TODO: This must only be done once on pre-xapp
	if (fuji->remote_version != -1) {
		// Begin camera remote - (per spec, OpenCapture is much more broad than 'take picture')
		// This tells the camera to open the remote mode sockets (video/event)
		fuji->open_capture_trans_id = r->transaction;
		rc = ptp_init_open_capture(r, 0, 0);
		if (rc) {
			app_print(r, "Failed to enter remote mode");
			return rc;
		}

		rc = fuji_get_events(r);
		if (rc) return rc;

		usleep(1000 * 50); // TODO: fix for vcam's bad timing

		if (!fuji->opened_liveview_sockets) {
			rc = ptpip_connect_events(r, fuji->ip_address, FUJI_EVENT_IP_PORT);
			if (rc) return rc;

			rc = ptpip_connect_video(r, fuji->ip_address, FUJI_LIVEVIEW_IP_PORT);
			if (rc) return rc;

			fuji->opened_liveview_sockets = 1;
		}
	}

	return 0;
}

int fuji_config_image_gallery(struct PtpRuntime *r) {
	fujipriv_t *fuji = fuji_get(r);
	if (r->connection_type == PTP_USB || fuji->transport == FUJI_FEATURE_WIRELESS_TETHER) return 0;
	if (fuji->remote_image_view_version != -1) {
		ptp_verbose_log("remote_image_view_version: %X", fuji->remote_image_view_version);
		int rc = fuji_get_events(r);
		if (rc) return rc;

		rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_CameraState_DF00, FUJI_REMOTE_ACCESS);
		if (rc) return rc;

		// Will confirm CameraState is set
		// This will also update a bunch of other properties
		rc = fuji_get_events(r);
		if (rc) return rc;
		rc = fuji_get_events(r);
		if (rc) return rc;

		if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
			rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_ClientState_DF01, FUJI_MODE_REMOTE_IMG_VIEW_XAPP);
			if (rc) return rc;
		} else {
			rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_ClientState_DF01, FUJI_MODE_REMOTE_IMG_VIEW);
			if (rc) return rc;
		}

		if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
			rc = fuji_xapp_config_gallery(r);
			if (rc) return rc;
		} else {
			rc = fuji_get_events(r);
			if (rc) return rc;

			// NOTE: xapp gets but doesn't set df25 when switching gallery->liveview->gallery

			rc = ptp_get_prop_value(r, PTP_DPC_FUJI_RemoteGetObjectVersion_DF25);
			fuji->remote_image_view_version = ptp_parse_prop_value(r);
			if (rc) return rc;
			// Set the prop higher - X-S10 and X-H1 want 4
			rc = ptp_set_prop_value(r, PTP_DPC_FUJI_RemoteGetObjectVersion_DF25, 5);
			if (rc) return rc;
		}

		// The props we set should show up here
		rc = fuji_get_events(r);
		if (rc) return rc;
	}

	return 0;
}

static long get_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long)(ts.tv_sec * 1000000L + ts.tv_nsec / 1000L);
}

int fuji_download_file_ex(struct PtpRuntime *r, int handle, int (info)(void *arg, struct PtpObjectInfo *oi), int (handle_add)(void *arg, void *buf, unsigned int len, unsigned int offset, unsigned int total_size), void *arg) {
	fujipriv_t *fuji = fuji_get(r);
	int rc = 0;

	ptp_verbose_log("Downloading object %d\n", handle);

	if (fuji->transport == FUJI_FEATURE_WIRELESS_COMM) {
		rc = fuji_get_events(r);
		if (rc) return rc;

		// Seems to take a while in some cases.
		r->wait_for_response = 3;
		rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_EnableCorrectFileSize_D227, 1);
	} else if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		ptp_set_prop_value16(r, PTP_DPC_FUJI_CompressSmall_D226, 2);
	}

	ptp_mutex_lock(r);

	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(r, handle, &oi);
	if (rc) {
		return rc;
	}

	if (info != NULL) info(arg, &oi);

	long then = get_ms();

	unsigned int read = 0;
	while (1) {
		if (app_check_thread_cancel(r)) {
			rc = PTP_CANCELED;
			goto end;
		}

		long then_c = get_ms();

		unsigned int cur = oi.compressed_size - read;
		if (cur > FUJI_MAX_PARTIAL_OBJECT) cur = FUJI_MAX_PARTIAL_OBJECT;
		rc = ptp_get_partial_object(r, handle, read, cur);
		if (rc == PTP_CHECK_CODE) {
			goto end;
		} else if (rc) {
			ptp_verbose_log("Download fail %d", rc);
			ptp_mutex_unlock(r);
			return rc;
		}

		size_t payload_size = ptp_get_payload_length(r);

		app_report_download_speed(r, get_ms() - then_c, payload_size);

		if (payload_size == 0) {
			rc = PTP_RUNTIME_ERR;
			goto end;
		}

		rc = handle_add(arg, ptp_get_payload(r), payload_size, read, oi.compressed_size);
		if (rc) {
			rc = PTP_CANCELED;
			goto end;
		}

		read += (int)payload_size;

		if (read >= oi.compressed_size) {
			long now = get_ms();
			ptp_verbose_log("Took %ld seconds\n", (now - then) / 1000 / 1000);
			rc = 0;
			goto end;
		}
	}

	end:;
	if (fuji->transport == FUJI_FEATURE_WIRELESS_COMM) {
		rc = fuji_get_events(r);
		if (rc) {
			ptp_mutex_unlock(r);
			return rc;
		}
		rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_EnableCorrectFileSize_D227, 0);
	} else if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		ptp_set_prop_value16(r, PTP_DPC_FUJI_CompressSmall_D226, 2);
	}
	ptp_mutex_unlock(r);
	return rc;
}

int fuji_download_file(struct PtpRuntime *r, int handle, int (handle_add)(void *arg, void *buf, unsigned int len, unsigned int offset, unsigned int total_size), void *arg) {
	return fuji_download_file_ex(r, handle, NULL, handle_add, arg);
}

// Functionality of FUJI_MULTIPLE_TRANSFER
int fuji_download_classic(struct PtpRuntime *r) {
	while (1) {
		// This determines whether the connection is terminated or not
		struct PtpObjectInfo oi;
		int rc = ptp_get_object_info(r, 1, &oi);
		if (rc) return rc;

		app_downloading_file(r, &oi);

		char path[256];
		app_get_file_path(r, path, oi.filename);
		FILE *f = fopen(path, "wb");
		if (f == NULL) return PTP_RUNTIME_ERR;

		// Not sure if 0x100000 is required or not, but we'll do what Fuji is doing.
		rc = ptp_download_object(r, 1, f, 0x100000);
		fclose(f);
		if (rc) {
			app_print(r, "Failed to save %s: %s", oi.filename, ptp_perror(rc));
			return rc;
		}

		app_downloaded_file(r, &oi, path);

		// Fuji's filesystem will swap out object ID 1 with the next image. If there
		// are no more images, the camera shuts down the connection and turns off.

		// In other words, the camera is in superposition - it's on and off at the same time.
		// We don't know until we observe it:
		rc = fuji_get_events(r);
		if (rc) return 0;
	}
}

int ptp_fuji_get_object_handles(struct PtpRuntime *r, struct PtpArray **a) {
	fujipriv_t *fuji = fuji_get(r);
	if (r->connection_type == PTP_USB) {
		return ptp_get_object_handles(r, -1, 0x0, 0x0, a);
	} else {
		// By this point num_objects should be known
		if (fuji->num_objects == 0 || fuji->num_objects == -1) {
			return PTP_RUNTIME_ERR;
		}

		// (Object handle 0x0 is invalid, as per PTP spec)
		struct PtpArray *list = malloc(sizeof(int) * fuji->num_objects + sizeof(struct PtpArray));
		list->length = fuji->num_objects;
		for (int i = 0; i < fuji->num_objects; i++) {
			list->data[i] = i + 1;
		}
		(*a) = list;
	}
	return 0;
}
