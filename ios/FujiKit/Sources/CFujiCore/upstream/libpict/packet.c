// Packet generation, parsing, and manipulation routines
// Copyright 2022 by Daniel C (https://github.com/petabyt/libpict)

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <libpict.h>

// PTP/IP-specific packet
unsigned int ptpip_bulk_packet(struct PtpRuntime *r, const struct PtpCommand *cmd, int type, unsigned int data_length) {
	struct PtpIpRequestContainer bulk;
	unsigned int size = 18 + (sizeof(uint32_t) * cmd->param_length);
	bulk.length = size;
	bulk.type = type;
	bulk.length += data_length;
	bulk.code = cmd->code;
	bulk.transaction = r->transaction;

	if (r->data_phase_length == 0) {
		bulk.data_phase = 1;
	} else {
		bulk.data_phase = 2;
	}

	for (int i = 0; i < 5; i++) {
		bulk.params[i] = cmd->params[i];
	}

	memcpy(r->data, &bulk, size);

	return size;
}

unsigned int ptpip_data_start_packet(struct PtpRuntime *r, unsigned int data_length) {
	struct PtpIpStartDataPacket *pkt = (struct PtpIpStartDataPacket *)(r->data);
	pkt->length = 20; // fixme: This was 0x20, changed it (?????)
	pkt->type = PTPIP_DATA_PACKET_START;
	pkt->transaction = r->transaction;
	pkt->payload_length = (uint64_t)data_length;

	return pkt->length;
}

unsigned int ptpip_data_end_packet(struct PtpRuntime *r, const void *data, unsigned int data_length) {
	struct PtpIpEndDataPacket *pkt = (struct PtpIpEndDataPacket *)(r->data);
	pkt->length = 12 + data_length;
	pkt->type = PTPIP_DATA_PACKET_END;
	pkt->transaction = r->transaction;

	memcpy(((uint8_t *)r->data) + 12, data, data_length);

	return pkt->length;
}

int ptpusb_new_data_packet(struct PtpRuntime *r, const struct PtpCommand *cmd, const void *data, unsigned int data_length) {
	uint32_t size = 12 + data_length;
	struct PtpBulkContainer bulk;
	ptp_write_u32(&bulk.length, size);
	ptp_write_u32(&bulk.type, PTP_PACKET_TYPE_DATA);
	ptp_write_u32(&bulk.code, cmd->code);
	ptp_write_u32(&bulk.transaction, r->transaction);
	memcpy(r->data, &bulk, 12);
	memcpy(r->data + 12, data, data_length);
	return (int)size;
}

int ptpusb_new_cmd_packet(struct PtpRuntime *r, const struct PtpCommand *cmd) {
	uint32_t size = 12 + (4 * cmd->param_length);
	struct PtpBulkContainer bulk;
	ptp_write_u32(&bulk.length, size);
	ptp_write_u32(&bulk.type, PTP_PACKET_TYPE_COMMAND);
	ptp_write_u32(&bulk.code, cmd->code);
	ptp_write_u32(&bulk.transaction, r->transaction);
	for (int i = 0; i < cmd->param_length; i++) {
		ptp_write_u32(&bulk.params[i], cmd->params[i]);
	}
	memcpy(r->data, &bulk, size);
	return (int)size;
}

// Generate a IP or USB style command packet (both are pretty similar)
unsigned int ptp_new_cmd_packet(struct PtpRuntime *r, const struct PtpCommand *cmd) {
	if (r->connection_type == PTP_IP) {
		return ptpip_bulk_packet(r, cmd, PTPIP_COMMAND_REQUEST, 0);
	} else {
		return ptpusb_new_cmd_packet(r, cmd);
	}
}

// PTPIP: Get data start packet, then data end packet, then response packet
static struct PtpIpResponseContainer *ptpip_get_response_packet(struct PtpRuntime *r) {
	struct PtpIpStartDataPacket *ds = (struct PtpIpStartDataPacket*)(r->data);
	if (ds->type == PTPIP_COMMAND_RESPONSE) {
		// If there is no data phase, return first packet
		return (struct PtpIpResponseContainer *)(r->data);
	}

	if (ds->type != PTPIP_DATA_PACKET_START) {
		ptp_panic("ptpip_get_response_packet(): didn't get data start packet");
	}

	struct PtpIpEndDataPacket *de = (struct PtpIpEndDataPacket *)(r->data + ds->length);
	if (de->type != PTPIP_DATA_PACKET_END) {
		ptp_panic("ptpip_get_response_packet(): didn't get data end packet");
	}

	struct PtpIpResponseContainer *resp = (struct PtpIpResponseContainer *)(r->data + ds->length + de->length);
	if (resp->type != PTPIP_COMMAND_RESPONSE) {
		ptp_panic("ptpip_get_response_packet(): didn't get response packet");
	}

	return resp;
}

// Update transid for current request packet
void ptp_update_transaction(struct PtpRuntime *r, int t) {
	if (r->connection_type == PTP_IP) {
		struct PtpIpRequestContainer *bulk = (struct PtpIpRequestContainer *)(r->data);
		bulk->transaction = t;
	} else {
		struct PtpBulkContainer *bulk = (struct PtpBulkContainer *)(r->data);
		bulk->transaction = t;
	}
}

// Get rccode from response packet
int ptp_get_return_code(struct PtpRuntime *r) {
	if (r->connection_type == PTP_IP) {
		struct PtpIpResponseContainer *resp = ptpip_get_response_packet(r);
		return resp->code;
	} else {
		struct PtpBulkContainer *bulk = (struct PtpBulkContainer*)(r->data);
		if (bulk->type == PTP_PACKET_TYPE_DATA) {
			bulk = (struct PtpBulkContainer*)(r->data + bulk->length);
			return bulk->code;
		} else {
			return bulk->code;
		}
	}
}

uint8_t *ptp_get_payload(struct PtpRuntime *r) {
	if (r->connection_type == PTP_IP) {
		// For IP, payload is in the DATA_END packet
		struct PtpIpStartDataPacket *ds = (struct PtpIpStartDataPacket*)(r->data);
		if (ds->type != PTPIP_DATA_PACKET_START) {
			ptp_panic("ptp_get_payload(): non data start packet");
		}

		struct PtpIpHeader *de = (struct PtpIpHeader*)(r->data + ds->length);
		if (de->type != PTPIP_DATA_PACKET_END) {
			ptp_panic("ptp_get_payload(): non data end packet");
		}

		return r->data + ds->length + 12;
	} else {
		struct PtpBulkContainer *bulk = (struct PtpBulkContainer*)(r->data);
		if (bulk->type == PTP_PACKET_TYPE_RESPONSE) {
			return r->data + 12;
		} else if (bulk->type == PTP_PACKET_TYPE_COMMAND) {
			return r->data + 28;
		} else {
			// TODO: Fatal
			return r->data + 12;
		}
	}
}

unsigned int ptp_get_payload_length(struct PtpRuntime *r) {
	if (r->connection_type == PTP_IP) {
		struct PtpIpStartDataPacket *ds = (struct PtpIpStartDataPacket*)(r->data);
		if (ds->type != PTPIP_DATA_PACKET_START) {
			ptp_panic("ptp_get_payload_length(): non data start packet");
		}

		return (int)ds->payload_length;
	} else {
		struct PtpBulkContainer *bulk = (struct PtpBulkContainer*)(r->data);
		return bulk->length - 12;
	}
}

unsigned int ptp_get_param_length(struct PtpRuntime *r) {
	if (r->connection_type == PTP_IP) {
		struct PtpIpResponseContainer *resp = ptpip_get_response_packet(r);	
		return (resp->length - 14) / 4;	
	} else {
		struct PtpBulkContainer *bulk = (struct PtpBulkContainer*)(r->data);

		// Get response packet, which backend stores after data packet
		if (bulk->type == PTP_PACKET_TYPE_DATA) {
			bulk = (struct PtpBulkContainer*)(r->data + bulk->length);
		}

		return (bulk->length - 12) / 4;
	}
}

uint32_t ptp_get_param(struct PtpRuntime *r, int index) {
	if (r->connection_type == PTP_IP) {
		struct PtpIpResponseContainer *resp = ptpip_get_response_packet(r);	
		return resp->params[index];		
	} else {
		struct PtpBulkContainer *bulk = (struct PtpBulkContainer*)(r->data);

		// Get response packet, which backend stores after data packet
		if (bulk->type == PTP_PACKET_TYPE_DATA) {
			bulk = (struct PtpBulkContainer*)(r->data + bulk->length);
			return bulk->code;
		}

		return bulk->params[index];
	}
}

int ptp_get_last_transaction_id(struct PtpRuntime *r) {
	if (r->connection_type == PTP_IP) {
		struct PtpIpResponseContainer *resp = ptpip_get_response_packet(r);	
		return (int)resp->transaction;
	} else {
		struct PtpBulkContainer *bulk = (struct PtpBulkContainer*)(r->data);
		if (bulk->type == PTP_PACKET_TYPE_DATA) {
			bulk = (struct PtpBulkContainer*)(r->data + bulk->length);
			return (int)bulk->transaction;
		} else {
			return (int)bulk->transaction;
		}
	}
}
