// Copyright 2024 (c) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libpict.h>
#include "app.h"
#include "fuji.h"
#include "fujiptp.h"

static int get_prop_size(uint16_t code) {
	switch (code) {
	case PTP_DPC_CaptureDelay:
		return 2;
	}
	return 0;
}

int fuji_register_device_info(struct PtpRuntime *r, uint8_t *data) {
	// 'Device info' holds all the different limits for different properties
	// TODO: Seems like 'device info' should be renamed to something like PropLimits?
	int of = 0;

	uint32_t length;
	of += ptp_read_u32(data + of, &length);

	if (length > 500) {
		ptp_panic("Too many properties");
		return -1;
	}

	for (uint32_t i = 0; i < length; i++) {
		uint32_t this_length;
		uint16_t code;
		of += ptp_read_u32(data + of, &this_length);
		of += ptp_read_u16(data + of, &code);

		plat_dbg("Code, %X", code);
		plat_dbg("length, %d", this_length);

		//struct PtpPropDesc pd;
		//int rc = ptp_get_prop_desc(r, code, &pd);
		//if (rc) return rc;

		//plat_dbg("data type: %X\n", pd.data_type);

		int payload_len = this_length - 2 - 4;
		for (int x = 0; x < payload_len; x++) {
			uint8_t b;
			of += ptp_read_u8(data + of, &b);
		}
	}

	return 0;
}

int ptp_fuji_get_init_info(struct PtpRuntime *r, struct PtpFujiInitResp *resp) {
	uint8_t *d = ptp_get_payload(r);

	d += ptp_read_u32(d, &resp->x1);
	d += ptp_read_u32(d, &resp->x2);
	d += ptp_read_u32(d, &resp->x3);
	d += ptp_read_u32(d, &resp->x4);

	ptp_read_unicode_string(resp->cam_name, (char *)d, sizeof(resp->cam_name));

	return 0;
}

int ptp_fuji_parse_object_info(struct PtpRuntime *r, struct PtpFujiObjectInfo *oi) {
	uint8_t *d = ptp_get_payload(r);
	memcpy(oi, d, PTP_FUJI_OBJ_INFO_VAR_START);
	d += PTP_FUJI_OBJ_INFO_VAR_START;
	d += ptp_read_string(d, oi->filename, sizeof(oi->filename));

	/* TODO: Figure out payload later:
		0D 44 00 53 00 43 00 46 00 35 00 30 00 38 00 37 00 2E 00 4A 00 50 00 47 00
		00 00 10 32 00 30 00 31 00 35 00 30 00 35 00 32 00 34 00 54 00 30 00 31 00
		31 00 37 00 31 00 30 00 00 00 00 0E 4F 00 72 00 69 00 65 00 6E 00 74 00 61
		00 74 00 69 00 6F 00 6E 00 3A 00 31 00 00 00 0C 00 00 00 03 00 01 20 0E 00
		00 00 00
	*/

	return 0;
}
