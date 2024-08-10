// Fujifilm USB
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>
#include "app.h"
#include "fuji.h"
#include "fujiptp.h"

int fujiusb_setup(struct PtpRuntime *r) {
	int rc = ptp_open_session(r);
	if (rc) {
		app_print("Failed to open session.");
		return rc;
	}

	struct PtpDeviceInfo di;
	rc = ptp_get_device_info(r, &di);
	if (rc) return rc;

	app_send_cam_name(di.model);

	// TODO: Determine transport
	fuji_get(r)->transport = FUJI_FEATURE_USB_CARD_READER;

	return rc;
}

int fujitether_setup(struct PtpRuntime *r) {
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
	app_print("The camera is thinking...");
	usleep(50000);

	fujiusb_setup(r);

	return 0;
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
