// Fujifilm USB
// Copyright 2024 (c) Unofficial Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include "app.h"
#include "fuji.h"
#include "fujiptp.h"

int fujiusb_try_connect(struct PtpRuntime *r) {
	fuji_reset_ptp(r);
	r->connection_type = PTP_USB;
	fuji_get(r)->transport = FUJI_FEATURE_USB;

	struct PtpDeviceEntry *list = ptpusb_device_list(r);

	struct PtpDeviceEntry *curr = NULL;
	for (curr = list; curr != NULL; curr = curr->next) {
		if (curr->vendor_id == 0x4cb) {
			break;
		}
	}

	if (curr == NULL) {
		ptpusb_free_device_list(list);
		return PTP_NO_DEVICE;
	}

	int rc = ptp_device_open(r, curr);
	if (rc) {
		ptpusb_free_device_list(list);
		return rc;
	}

	ptpusb_free_device_list(list);

	return 0;
}

int fujiusb_setup(struct PtpRuntime *r) {
	int rc = ptp_open_session(r);
	if (rc == PTP_CHECK_CODE) {
		// PTP_RC_SessionAlreadyOpened, don't care
	} else if (rc) {
		app_print("Failed to open session.");
		return rc;
	}

	struct PtpDeviceInfo di;
	rc = ptp_get_device_info(r, &di);
	if (rc) return rc;

	app_send_cam_name(di.model);

	struct PtpArray *arr;
	rc = ptp_get_storage_ids(r, &arr);
	if (rc) return rc;

	uint32_t live_id = 0;
	uint32_t still_id = 0;
	for (size_t i = 0; i < arr->length; i++) {
		struct PtpStorageInfo si;
		rc = ptp_get_storage_info(r, (int)arr->data[i], &si);
		if (!strcmp(si.storage_desc, "Still")) {
			live_id = arr->data[i];
		} else if (!strcmp(si.storage_desc, "Live")) {
			still_id = arr->data[i];
		}
	}

	if (live_id != 0 && still_id != 0) {
		fuji_get(r)->transport = FUJI_FEATURE_USB_TETHER_SHOOT;

		// Check for backup file
		struct PtpObjectInfo oi;
		rc = ptp_get_object_info(r, 0, &oi);
		if (rc == PTP_CHECK_CODE) return 0;
		if (rc) return rc;
		if (oi.obj_format == 0x5000) {
			ptp_verbose_log("0x5000 backup object exists\n");
			fuji_get(r)->transport = FUJI_FEATURE_RAW_CONV;
		}

	} else {
		ptp_verbose_log("Not Raw Conv: %d %d\n", still_id, live_id);
	}

	return rc;
}

int fujitether_setup(struct PtpRuntime *r) {
	app_print("Waiting on the camera...");
	app_print("Make sure you pressed OK.");

	if (r->connection_type == PTP_IP_USB) {
		struct PtpFujiInitResp resp;
		int rc = ptpip_fuji_init_req(r, DEVICE_NAME, &resp);
		if (rc == PTP_RUNTIME_ERR) {
			rc = ptpip_fuji_init_req(r, DEVICE_NAME, &resp);
		}
		if (rc == PTP_RUNTIME_ERR) {
			rc = ptpip_fuji_init_req(r, DEVICE_NAME, &resp);
		}
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
		app_print("The camera is thinking...");
		usleep(50000);
	}

	int rc = fujiusb_setup(r);

	return rc;
}

int fujiusb_download_backup(struct PtpRuntime *r, FILE *f) {
	if (fuji_get(r)->transport != FUJI_FEATURE_RAW_CONV) {
		return PTP_UNSUPPORTED;
	}

	/*
	For some bizarre reason, Fuji X Acquire ended up doing something like:
	struct PtpDeviceInfo di;
	for (int i = 0; i < 20; i++) {
		rc = ptp_get_device_info(r, &di);
		rc = ptp_get_prop_value(r, 0xd20b);
	}
	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(r, 0, &oi);
	rc = ptp_get_object_info(r, 0, &oi);
	Before running GetObject. I'm guessing they have a background thread running GetDeviceInfo and GetPropValue 0xd20b.
	*/

	ptp_mutex_lock(r);

	struct PtpObjectInfo oi;
	int rc = ptp_get_object_info(r, 0, &oi);
	if (rc) goto end;

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));
	plat_dbg(buffer);

	rc = ptp_get_object(r, 0);
	if (rc) goto end;
	plat_dbg("Downloaded payload %d bytes\n", ptp_get_payload_length(r));

	fwrite(ptp_get_payload(r), 1, ptp_get_payload_length(r), f);

	end:;
	ptp_mutex_lock(r);

	return rc;
}

/*

struct PtpCommand cmd;
cmd.code = PTP_OC_GetDevicePropValue;
cmd.params[0] = 0xD18D;
cmd.param_length = 1;
ptp_generic_send(&r, &cmd);

ssize_t sz = ptp_get_payload_length(&r);
uint8_t *data = ptp_get_payload(&r);
print_payload(data + 1, sz - 1, false);

cmd.code = PTP_OC_SetDevicePropValue;
cmd.params[0] = 0xD21C;
cmd.param_length = 1;
ptp_generic_send(&r, &cmd);

cmd.code = PTP_OC_SetDevicePropValue;
cmd.params[0] = 0xD18C;
cmd.param_length = 1;
ptp_generic_send(&r, &cmd);

cmd.code = PTP_OC_GetDevicePropValue;
cmd.params[0] = 0xD18D;
cmd.param_length = 1;
ptp_generic_send(&r, &cmd);

sz = ptp_get_payload_length(&r);
data = ptp_get_payload(&r);
print_payload(data + 1, sz  - 1, false);

*/
