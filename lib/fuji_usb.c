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