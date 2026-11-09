// Easy to use operation (OC) functions. Requires backend.
// Copyright 2022 by Daniel C (https://github.com/petabyt/libpict)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libpict.h>
#include <ptp.h>

static struct PtpArray *dup_uint_array(const uint8_t *buf) {
	uint32_t length;
	int of = ptp_read_u32(buf, &length);

	struct PtpArray *dup = malloc(4 + length * 4);
	if (dup == NULL) ptp_panic("oom");

	dup->length = length;
	for (int i = 0; i < length; i++) {
		uint32_t memb = 0;
		of += ptp_read_u32(buf + of, &memb);
		dup->data[i] = memb;
	}

	return dup;
}

int ptpip_init_command_request(struct PtpRuntime *r, const char *device_name) {
	struct PtpIpInitPacket *p = (struct PtpIpInitPacket *)r->data;
	memset(p, 0, sizeof(struct PtpIpInitPacket));
	p->length = sizeof(struct PtpIpInitPacket);

	p->type = PTPIP_INIT_COMMAND_REQ;

	p->guid1 = 0xffffffff;
	p->guid2 = 0xffffffff;
	p->guid3 = 0xffffffff;
	p->guid4 = 0xffffffff;

	p->minor_ver = 1;

	ptp_write_unicode_string(p->device_name, device_name);

	if (ptpip_cmd_write(r, r->data, p->length) != p->length) return PTP_IO_ERR;

	// Read the packet size, then receive the rest
	int x = ptpip_cmd_read(r, r->data, 4);
	if (x < 0) return PTP_IO_ERR;
	x = ptpip_cmd_read(r, r->data + 4, p->length - 4);
	if (x < 0) return PTP_IO_ERR;

	struct PtpIpHeader *hdr = (struct PtpIpHeader *)r->data;
	if (hdr->type == PTPIP_INIT_FAIL) {
		return PTP_CHECK_CODE;
	}

	return 0;
}

// Experimental, not for use yet - none of my devices seem to use this endpoint
int ptp_get_event(struct PtpRuntime *r, struct PtpEventContainer *ec) {
	int rc = ptp_read_int(r, r->data, r->max_packet_size);
	if (rc) return rc;

	memcpy(ec, r->data, sizeof(struct PtpEventContainer));

	return rc;
}

int ptpip_init_events(struct PtpRuntime *r) {
	struct PtpIpHeader h;
	h.length = 12;
	h.type = PTPIP_INIT_EVENT_REQ;
	h.params[0] = 1;
	if (ptpip_event_send(r, &h, h.length) != h.length) {
		return PTP_IO_ERR;
	}

	// ack is always 4 bytes
	if (ptpip_event_read(r, r->data, 8) != 8) {
		return PTP_IO_ERR;
	}

	return 0;
}

int ptp_open_session(struct PtpRuntime *r) {
	r->session++;

	struct PtpCommand cmd;
	cmd.code = PTP_OC_OpenSession;
	cmd.params[0] = r->session;
	cmd.param_length = 1;
	//cmd.data_length = 0;

	// PTP open session transaction ID is always 0
	r->transaction = 0;

	return ptp_send(r, &cmd);
}

int ptp_close_session(struct PtpRuntime *r) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_CloseSession;
	cmd.param_length = 0;
	return ptp_send(r, &cmd);
}

int ptp_get_device_info(struct PtpRuntime *r, struct PtpDeviceInfo *di) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetDeviceInfo;
	cmd.param_length = 0;
	int rc = ptp_send(r, &cmd);
	if (rc) return rc;

	return ptp_parse_device_info(r, di);
}

int ptp_init_capture(struct PtpRuntime *r, int storage_id, int object_format) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_InitiateCapture;
	cmd.param_length = 2;
	cmd.params[0] = storage_id;
	cmd.params[1] = object_format;

	return ptp_send(r, &cmd);
}

int ptp_init_open_capture(struct PtpRuntime *r, int storage_id, int object_format) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_InitiateOpenCapture;
	cmd.param_length = 2;
	cmd.params[0] = storage_id;
	cmd.params[1] = object_format;

	return ptp_send(r, &cmd);
}

int ptp_terminate_open_capture(struct PtpRuntime *r, int trans) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_TerminateOpenCapture;
	cmd.param_length = 1;
	cmd.params[0] = trans;

	return ptp_send(r, &cmd);
}

int ptp_get_storage_ids(struct PtpRuntime *r, struct PtpArray **a) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetStorageIDs;
	cmd.param_length = 0;

	ptp_mutex_lock(r);

	int rc = ptp_send(r, &cmd);
	if (rc) {
		(*a) = NULL;
		ptp_mutex_unlock(r);
		return rc;
	}

	(*a) = dup_uint_array(ptp_get_payload(r));

	ptp_mutex_unlock(r);
	
	return rc;
}

int ptp_get_storage_info(struct PtpRuntime *r, int id, struct PtpStorageInfo *si) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetStorageInfo;
	cmd.param_length = 1;
	cmd.params[0] = id;

	ptp_mutex_lock(r);

	int rc = ptp_send(r, &cmd);
	if (rc) {
		ptp_mutex_unlock(r);
		return rc;
	}

	rc = ptp_parse_storage_info(r, si);
	ptp_mutex_unlock(r);
	return rc;
}

int ptp_get_partial_object(struct PtpRuntime *r, uint32_t handle, unsigned int offset, unsigned int max) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetPartialObject;
	cmd.param_length = 3;
	cmd.params[0] = handle;
	cmd.params[1] = offset;
	cmd.params[2] = max;

	return ptp_send(r, &cmd);
}

int ptp_get_object_info(struct PtpRuntime *r, uint32_t handle, struct PtpObjectInfo *oi) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetObjectInfo;
	cmd.param_length = 1;
	cmd.params[0] = handle;

	int rc = ptp_send(r, &cmd);
	if (rc) return rc;

	ptp_parse_object_info(r, oi);
	return 0;
}

int ptp_send_object_info(struct PtpRuntime *r, int storage_id, int handle, struct PtpObjectInfo *oi) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SendObjectInfo;
	cmd.param_length = 2;
	cmd.params[0] = storage_id;
	cmd.params[1] = handle;

	uint8_t temp[2048];
	int length = ptp_pack_object_info(r, oi, temp, sizeof(temp));
	if (length < 0) return length;

	return ptp_send_data(r, &cmd, temp, length);
}

int ptp_send_object(struct PtpRuntime *r, const void *data, unsigned int data_length) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SendObject;
	cmd.param_length = 0;
	return ptp_send_data(r, &cmd, data, data_length);
}

int ptp_get_object_handles(struct PtpRuntime *r, int id, int format, int in, struct PtpArray **a) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetObjectHandles;
	cmd.param_length = 3;
	cmd.params[0] = id;
	cmd.params[1] = format;
	cmd.params[2] = in;

	ptp_mutex_lock(r);

	int rc = ptp_send(r, &cmd);
	if (rc) goto end;

	(*a) = dup_uint_array(ptp_get_payload(r));

	end:;
	ptp_mutex_unlock(r);

	return rc;
}

int ptp_get_num_objects(struct PtpRuntime *r, int id, int format, int in) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetNumObjects;
	cmd.param_length = 3;
	cmd.params[0] = id;
	cmd.params[1] = format;
	cmd.params[2] = in;

	return ptp_send(r, &cmd);
}

int ptp_get_prop_value(struct PtpRuntime *r, int code) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetDevicePropValue;
	cmd.param_length = 1;
	cmd.params[0] = code;
	return ptp_send(r, &cmd);
}

int ptp_get_prop_desc(struct PtpRuntime *r, int code, struct PtpPropDesc *pd) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetDevicePropDesc;
	cmd.param_length = 1;
	cmd.params[0] = code;

	ptp_mutex_lock(r);
	int rc = ptp_send(r, &cmd);
	if (rc) goto end;

	ptp_parse_prop_desc(r, pd);

	end:;
	ptp_mutex_unlock(r);

	return rc;
}

int ptp_get_thumbnail(struct PtpRuntime *r, int handle) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetThumb;
	cmd.param_length = 1;
	cmd.params[0] = handle;

	// NOTE: raw JPEG contents is directly in the payload

	return ptp_send(r, &cmd);
}

int ptp_move_object(struct PtpRuntime *r, int storage_id, int handle, int folder) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetThumb;
	cmd.param_length = 3;
	cmd.params[0] = handle;
	cmd.params[1] = storage_id;
	cmd.params[2] = folder;

	return ptp_send(r, &cmd);
}

int ptp_set_prop_value(struct PtpRuntime *r, int code, int value) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SetDevicePropValue;
	cmd.param_length = 1;
	cmd.params[0] = code;

	uint32_t dat[] = {(uint32_t)value};

	return ptp_send_data(r, &cmd, dat, sizeof(dat));
}

int ptp_set_prop_value16(struct PtpRuntime *r, int code, uint16_t value) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SetDevicePropValue;
	cmd.param_length = 1;
	cmd.params[0] = code;
	return ptp_send_data(r, &cmd, &value, 2);
}

int ptp_set_prop_value_data(struct PtpRuntime *r, int code, void *data, unsigned int length) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SetDevicePropValue;
	cmd.param_length = 1;
	cmd.params[0] = code;

	return ptp_send_data(r, &cmd, data, length);
}

int ptp_delete_object(struct PtpRuntime *r, int handle) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_DeleteObject;
	cmd.param_length = 1;
	cmd.params[0] = handle;
	// TODO: Want a separate function to handle this
	//cmd.params[1] = format_code;

	return ptp_send(r, &cmd);	
}

int ptp_get_object(struct PtpRuntime *r, int handle) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_GetObject;
	cmd.param_length = 1;
	cmd.params[0] = handle;

	return ptp_send(r, &cmd);	
}

int ptp_download_object(struct PtpRuntime *r, int handle, FILE *f, size_t max) {
	unsigned int read = 0;
	while (1) {
		ptp_mutex_lock(r);
		int x = ptp_get_partial_object(r, handle, read, max > UINT32_MAX ? UINT32_MAX : (unsigned int)max);
		if (x) {
			ptp_mutex_unlock(r);
			return x;
		}

		size_t partial_len = ptp_get_payload_length(r);

		if (partial_len == 0) {
			fclose(f);
			ptp_mutex_unlock(r);
			return 0;
		}

		fwrite(ptp_get_payload(r), 1, partial_len, f);

		ptp_mutex_unlock(r);

		read += partial_len;

		if (partial_len != max) {
			return 0;
		}
	}
}
