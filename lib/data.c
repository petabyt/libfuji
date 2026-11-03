// Copyright 2024 (c) Fudge
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libpict.h>
#include "app.h"
#include "fuji.h"
#include "fujiptp.h"

const char *get_val_name(uint32_t code, uint32_t val) {
	switch (code) {
			case PTP_DPC_CaptureDelay:
				switch (val) {
					case 0: return "off";
					case 2: return "2s";
					case 4: return "10s";
					default: return NULL;
				}
			case PTP_DPC_FlashMode: return NULL;
			case PTP_DPC_WhiteBalance: return NULL;
			case PTP_DPC_ExposureBiasCompensation: return NULL;
			case PTP_DPC_FUJI_FilmSimulation:
				switch (val) {
					case 1: return "Standard";
					case 2: return "Vivid";
					case 3: return "Soft";
					case 4: return "4";
					case 5: return "5";
					case 6: return "6";
					case 7: return "7";
					case 8: return "8";
					case 9: return "9";
					case 10: return "10";
					case 11: return "11";
					default: return NULL;
				}
			case PTP_DPC_FUJI_ExposureIndex:
				switch (val) {
					// guess
					case 0x80000190: return "AUTO(400)";
					case 0x80000320: return "AUTO(800)";
					case 0x80000640: return "AUTO(1600)";
					case 0x80000c80: return "AUTO(3200)";
					case 0x80001900: return "AUTO(6400)";
					case 0x40000064: return "AUTO(100)";
					case 0xc8: return  "200";
					case 0xfa: return  "250";
					case 0x140: return "320";
					case 0x190: return "400";
					case 0x1f4: return "500";
					case 0x280: return "640";
					case 0x320: return "800";
					case 0x3e8: return "1000";
					case 0x4e2: return "1250";
					case 0x640: return "1600";
					case 0x7d0: return "2000";
					case 0x9c4: return "2500";
					case 0xc80: return "3200";
					case 0xfa0: return "4000";
					case 0x1388: return "5000";
					case 0x1900: return "6400";
					// guess
					case 0x40003200: return "12800 (?)";
					case 0x40006400: return "25600 (?)";
					case 0x4000c800: return "51200 (?)";
					default: return NULL;
				}
			case PTP_DPC_FUJI_RecMode: return NULL;
			case PTP_DPC_FUJI_FocusMeteringMode: return NULL;
			case PTP_DPC_FNumber: switch (val) {
				case 350: return  "f/3.5";
				case 450: return  "f/4.5";
				case 530: return  "f/5.3";
				case 710: return  "f/7.1";
				case 1000: return "f/10.0";
				case 1100: return "f/11.0";
				case 1600: return "f/16.0";
				default: return NULL;
			}
	}
	return NULL;
}

int fuji_register_device_info(struct PtpRuntime *r, struct FujiPropDescList *list) {
	uint8_t *data = ptp_get_payload(r);

	unsigned int of = 0;

	uint32_t n_entries;
	of += ptp_read_u32(data + of, &n_entries);

	if (list->items != NULL) {
		for (unsigned int i = 0; i < list->length; i++) {
			free(list->items[i]);
		}
		free(list->items);
	}

	list->length = n_entries;
	list->items = malloc(sizeof(struct FujiPropDesc *) * n_entries);

	for (unsigned int i = 0; i < n_entries; i++) {
		if (of > ptp_get_payload_length(r)) ptp_panic("overflow");
		unsigned int start_of = of;
		uint32_t this_length;
		uint16_t code, type;
		of += ptp_read_u32(data + of, &this_length);
		of += ptp_read_u16(data + of, &code);
		of += ptp_read_u16(data + of, &type);
		of += 1; // ???

		uint32_t default_value;
		uint16_t n_items;

		if (type == 3 || type == 4) {
			of += ptp_read_u32(data + of, &default_value);
			of += 1; // ???
			of += ptp_read_u16(data + of, &n_items);
		} else if (type == 6) {
			of += ptp_read_u32(data + of, &default_value);
			of += ptp_read_u32(data + of, &default_value);
			of += 1; // ???
			of += ptp_read_u16(data + of, &n_items);
		} else {
			of = start_of + this_length;
			continue;
		}

		list->items[i] = malloc(sizeof(struct FujiPropDesc) + (sizeof(struct PropDescValue) * n_items));
		list->items[i]->n_items = n_items;
		list->items[i]->code = code;

		for (int v = 0; v < n_items; v++) {
			uint32_t val = 0;
			uint16_t val16 = 0;
			if (type == 6) of += ptp_read_u32(data + of, &val);
			else if (type == 3 || type == 4) { of += ptp_read_u16(data + of, &val16); val = val16; }
			list->items[i]->items[v] = (struct PropDescValue){
				.val_name = get_val_name(code, val),
				.raw_val = val,
			};
		}

		of = start_of + this_length;
	}

	return 0;
}

int ptp_fuji_parse_init_struct(struct PtpRuntime *r, struct PtpFujiInitResp *resp) {
	uint8_t *d = r->data;

	d += ptp_read_u32(d, &resp->length);
	d += ptp_read_u32(d, &resp->type);
	d += ptp_read_u32(d, &resp->unk);
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

__attribute__((unused)) int fuji_d228(void) {
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
	return 0;
}