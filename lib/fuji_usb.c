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
#include <fp.h>

int fuji_send_object_info_ex(struct PtpRuntime *r, int storage_id, int handle, struct PtpObjectInfo *oi) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_FUJI_SendObjectInfo;
	cmd.param_length = 3;
	cmd.params[0] = storage_id;
	cmd.params[1] = handle;
	cmd.params[2] = 0;

	uint8_t buf[2048];
	int of = 0;
	of += ptp_write_u32(buf + of, oi->storage_id);
	of += ptp_write_u16(buf + of, oi->obj_format);
	of += ptp_write_u16(buf + of, oi->protection);
	of += ptp_write_u32(buf + of, oi->compressed_size);
	of += ptp_write_u16(buf + of, oi->thumb_format);
	of += ptp_write_u32(buf + of, oi->thumb_compressed_size);
	of += ptp_write_u32(buf + of, oi->thumb_width);
	of += ptp_write_u32(buf + of, oi->thumb_height);
	of += ptp_write_u32(buf + of, oi->img_width);
	of += ptp_write_u32(buf + of, oi->img_height);
	of += ptp_write_u32(buf + of, oi->img_bit_depth);
	of += ptp_write_u32(buf + of, oi->parent_obj);
	of += ptp_write_u16(buf + of, oi->assoc_type);
	of += ptp_write_u32(buf + of, oi->assoc_desc);
	of += ptp_write_u32(buf + of, oi->sequence_num);
	of += ptp_write_string(buf + of, oi->filename);
	of += ptp_write_string(buf + of, oi->date_created);
	of += ptp_write_string(buf + of, oi->date_modified);
	of += ptp_write_string(buf + of, oi->keywords);

	return ptp_send_data(r, &cmd, buf, of);
}

int fuji_send_object_ex(struct PtpRuntime *r, const void *data, size_t length) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_FUJI_SendObject2;
	cmd.param_length = 0;
	return ptp_send_data(r, &cmd, data, (int)length);
}

int fujiusb_dump_info(struct PtpRuntime *r) {
	// TODO: move from backend.c
	return 0;
}

int fujiusb_try_connect(struct PtpRuntime *r, int num) {
	fuji_reset_ptp(r);
	r->connection_type = PTP_USB;
	fuji_get(r)->transport = FUJI_FEATURE_USB;

	struct PtpDeviceEntry *list = ptpusb_device_list(r);

	struct PtpDeviceEntry *curr = NULL;
	int i = 0;
	for (curr = list; curr != NULL; curr = curr->next) {
		if (curr->vendor_id == 0x4cb) {
			if (num == -1) {
				break;
			}
			if (i == num) {
				break;
			}
			i++;
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

	if (strlen(di.manufacturer) > sizeof("FUJIFILM")) {
		if (strncmp(di.manufacturer, "FUJIFILM", sizeof("FUJIFILM")) != 0) {
			ptp_verbose_log("Weird - manufac doesn't start with 'fujifilm'??\n");
		}
	}

	app_send_cam_name(di.model);

	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_USBMode);
	if (rc == PTP_CHECK_CODE) {
		// This could also be FUJI_FEATURE_MOVIE_SHOOT
		fuji_get(r)->transport = FUJI_FEATURE_USB_CARD_READER;
	} else if (rc) {
		return rc;
	} else {
		int mode = ptp_parse_prop_value(r);
		if (mode == 5) {
			fuji_get(r)->transport = FUJI_FEATURE_USB_TETHER_SHOOT;
		} else if (mode == 6) {
			fuji_get(r)->transport = FUJI_FEATURE_RAW_CONV;
		} else if (mode == 8) {
			fuji_get(r)->transport = FUJI_FEATURE_WEBCAM;
		} else {
			ptp_verbose_log("Unknown Fuji USB mode %d, assuming MTP\n", mode);
			fuji_get(r)->transport = FUJI_FEATURE_USB;
		}
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

	ptp_mutex_lock(r);

	struct PtpObjectInfo oi;
	int rc = ptp_get_object_info(r, 0, &oi);
	if (rc) goto end;

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));
	plat_dbg(buffer);

	rc = ptp_get_object(r, 0);
	if (rc) goto end;
	plat_dbg("Downloaded payload %d bytes", ptp_get_payload_length(r));

	fwrite(ptp_get_payload(r), 1, ptp_get_payload_length(r), f);

	end:;
	ptp_mutex_lock(r);

	return rc;
}

int fujiusb_restore_backup(struct PtpRuntime *r, FILE *input) {
	fseek(input, 0, SEEK_END);
	long file_size = ftell(input);
	fseek(input, 0, SEEK_SET);
	if (file_size > 100000) {
		printf("Backup file seems to be too big, is the path correct?\n");
		return -1;
	}

	struct PtpObjectInfo oi = {0};
	oi.obj_format = 0x5000;
	oi.compressed_size = (uint32_t)file_size;

	uint8_t buf[1088] = {0};
	int of = 0;
	of += ptp_write_u32(buf + of, 0);
	of += ptp_write_u16(buf + of, 0x5000);
	of += ptp_write_u16(buf + of, 0);
	of += ptp_write_u32(buf + of, (uint32_t)file_size);

	//int rc = ptp_send_object_info(r, 0, 0, &oi);
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SendObjectInfo;
	cmd.param_length = 2;
	cmd.params[0] = 0;
	cmd.params[1] = 0;
	int rc = ptp_send_data(r, &cmd, buf, 1088);
	if (rc) return rc;

	void *buffer = (void *)malloc(file_size + 1);
	if (buffer == NULL) abort();

	if (fread(buffer, 1, file_size, input) != file_size) {
		ptp_error_log("Error reading backup file");
	}

	rc = ptp_send_object(r, buffer, file_size);
	return rc;
}

int fuji_get_battery_percent(struct PtpRuntime *r, int *value) {
	ptp_mutex_lock(r);
	int rc = ptp_get_prop_value(r, PTP_DPC_FUJI_BatteryInfo1);
	if (rc == 0) {
		(*value) = ptp_parse_prop_value(r);
	}
	ptp_mutex_unlock(r);
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

int fuji_send_raf(struct PtpRuntime *r, const char *path) {
	FILE* f = fopen(path, "rb");
	if (!f) {
		ptp_verbose_log("'%s' not found\n", path);
		return PTP_RUNTIME_ERR;
	}

	fseek(f, 0, SEEK_END);
	long file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *buffer = (char *)malloc(file_size + 1);
	if (!buffer) abort();

	fread(buffer, 1, file_size, f);
	buffer[file_size] = '\0';

	fclose(f);

	struct PtpObjectInfo oi = {0};
	oi.obj_format = 0xf802;
	oi.compressed_size = (uint32_t)file_size;
	strcpy(oi.filename, "FUP_FILE.dat");

	int rc = fuji_send_object_info_ex(r, 0, 0, &oi);
	if (rc) {
		free(buffer);
		return rc;
	}

	r->max_packet_size = 261632;

	rc = fuji_send_object_ex(r, buffer, file_size);

	r->max_packet_size = 512;

	free(buffer);
	return rc;
}

int fuji_process_raf(struct PtpRuntime *r, const char *input_raf_path, const char *output_path, const char *profile_xml_path) {
	int rc;

	struct FujiDeviceKnowledge *fuji = fuji_get(r);
	if (fuji->transport != FUJI_FEATURE_RAW_CONV) {
		ptp_error_log("Not in raw transfer mode\n");
		return PTP_RUNTIME_ERR;
	}

	rc = fuji_send_raf(r, input_raf_path);
	if (rc) return rc;

	// Download the profile
	ptp_mutex_lock(r);
	rc = ptp_get_prop_value(r, PTP_DPC_FUJI_RawConvProfile);
	if (rc) {
		ptp_mutex_unlock(r);
		return rc;
	}
	int profile_len = ptp_get_payload_length(r);
	void *profile = malloc(profile_len);
	memcpy(profile, ptp_get_payload(r), profile_len);
	ptp_mutex_unlock(r);

	ptp_verbose_log("Got %d bytes of profile\n", profile_len);

	uint8_t buffer[1024];
	struct FujiProfile fp;
	fp_parse_d185(profile, profile_len, &fp);

	struct FujiProfile user_fp;
	rc = fp_parse_fp1(profile_xml_path, &user_fp);
	if (rc == 0) {
		rc = fp_apply_profile(&user_fp, &fp);
		if (rc) {
			ptp_error_log("Failed to merge profile\n");
			return PTP_RUNTIME_ERR;
		}
	} else {
		ptp_error_log("Failed to parse %s\n", profile_xml_path);
		return PTP_RUNTIME_ERR;
	}

	profile_len = fp_create_d185(&fp, buffer, sizeof(buffer));
	if (profile_len < 0) {
		printf("Error creating d185\n");
		return -1;
	}

	rc = ptp_set_prop_value_data(r, PTP_DPC_FUJI_RawConvProfile, buffer, profile_len);
	if (rc) return rc;

	free(profile);

	rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_StartRawConversion, 0);
	if (rc) return rc;

	for (int i = 0; i < 20; i++) {
		struct PtpArray *list;
		rc = ptp_get_object_handles(r, -1, 0, 0, &list);
		if (rc) return rc;

		if (list->length == 0) {
			printf("Waiting..\n");
			usleep(1000 * 1000);
			free(list);
		} else {
			rc = ptp_get_object(r, (int) list->data[0]);
			if (rc) return rc;

			FILE *f = fopen(output_path, "wb");
			fwrite(ptp_get_payload(r), 1, ptp_get_payload_length(r), f);
			fclose(f);

			rc = ptp_delete_object(r, (int)list->data[0]);
			if (rc) return rc;

			free(list);

			break;
		}
	}

	return 0;
}
