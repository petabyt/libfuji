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

	ui_send_text("cam_name", di.model);

	return rc;
}

int fujitether_setup(struct PtpRuntime *r) {
	//memset(&fuji_known, 0, sizeof(struct FujiDeviceKnowledge));

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

	ui_send_text("cam_name", resp.cam_name);

	// Fuji cameras require delay after init
	app_print("The camera is thinking...");
	usleep(50000);

	fujiusb_setup(r);

	return 0;
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
