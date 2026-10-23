// Packet transport interface for PTPUSB, PTPIP, and PTPUSBIP - uses OS IO functions
// For Windows MTP, this calls functions in libwpd.c
// Copyright 2024 by Daniel C (https://github.com/petabyt/libpict)

#include <stdio.h>
#include <stdint.h>
#include <libpict.h>

int ptp_send_packet(struct PtpRuntime *r, unsigned int length) {
	if (r->comm_dump != NULL) {
		fwrite(r->data, 1, length, r->comm_dump);
	}
	unsigned int sent = 0;
	while (1) {
		unsigned max = length - sent;
		if (max > r->max_packet_size) max = r->max_packet_size;
		int rc;
		if (r->connection_type == PTP_USB) {
			rc = ptp_cmd_write(r, r->data + sent, max);
		} else if (r->connection_type == PTP_IP_USB) {
			rc = ptpip_cmd_write(r, r->data + sent, max);
		} else {
			ptp_panic("illegal connection_type");
		}

		if (rc < 0) {
			ptp_verbose_log(r, "%s: %d\n", __func__, rc);
			r->data_filled_length = 0;
			return PTP_IO_ERR;
		}

		sent += rc;

		if (sent > length) {
			ptp_panic("BUG: Sent too many bytes (?)");
		} else if (sent == length) {
			ptp_verbose_log(r, "%s: Sent %d/%d bytes\n", __func__, sent, length);
			r->data_filled_length = sent;
			return (int)sent;
		}
	}
}

int ptpip_read_packet(struct PtpRuntime *r, int of) {
	int rc = 0;
	int read = 0;

	while (r->wait_for_response) {
		rc = ptpip_cmd_read(r, r->data + of + read, 4);

		r->wait_for_response--;

		if (rc > 0) break;

		if (r->wait_for_response) {
			ptp_error_log(r, "Trying again...\n");
			PTP_SLEEP(PTP_WAIT_MS);
		}
	}

	r->wait_for_response = r->response_wait_default;

	if (rc < 0) {
		ptp_error_log(r, "Failed to read packet length: %d\n", rc);
		return PTP_COMMAND_IGNORED;
	}

	if (rc < 4) {
		ptp_error_log(r, "Failed to read at least packet length: %d\n", rc);
		return PTP_IO_ERR;
	}

	read += rc;

	struct PtpIpHeader *h = (struct PtpIpHeader *)(r->data + of);

	if (h->length - read == 0) {
		return read;
	}

	// Ensure data buffer is large enough for the rest of the packet
	if (of + read + h->length >= r->data_length) {
		rc = ptp_buffer_resize(r, of + read + h->length);
		if (rc) return rc;
	}

	h = (struct PtpIpHeader *)(r->data + of);

	while (1) {
		rc = ptpip_cmd_read(r, r->data + of + read, h->length - read);

		if (rc < 0) {
			ptp_verbose_log(r, "Read error: %d\n", rc);
			return PTP_IO_ERR;
		}

		read += rc;

		if (h->length - read == 0) {
			return read;
		}
	}
}

int ptpip_receive_bulk_packets(struct PtpRuntime *r) {
	r->data_filled_length = 0;
	int rc = ptpip_read_packet(r, 0);
	if (rc < 0) {
		return rc;
	}

	int pk1_of = rc;

	struct PtpIpHeader *h = (struct PtpIpHeader *)(r->data);

	if (h->type == PTPIP_DATA_PACKET_START) {
		rc = ptpip_read_packet(r, pk1_of);
		if (rc < 0) return rc;
		r->data_filled_length += rc;
		h = (struct PtpIpHeader *)(r->data + pk1_of);
		if (h->type != PTPIP_DATA_PACKET_END) {
			ptp_error_log(r, "Didn't receive an END DATA packet (%d)\n", h->type);
			return PTP_IO_ERR;
		}

		int pk2_of = pk1_of + (int)h->length;

		rc = ptpip_read_packet(r, pk2_of);
		if (rc < 0) return rc;
		r->data_filled_length += rc;
		h = (struct PtpIpHeader *)(r->data + pk2_of);
		if (h->type != PTPIP_COMMAND_RESPONSE) {
			ptp_error_log(r, "Non response packet after data end packet (%d)\n", h->type);
			return PTP_IO_ERR;
		}
	} else if (h->type == PTPIP_COMMAND_RESPONSE) {
		ptp_verbose_log(r, "Received response packet\n");
	} else {
		ptp_error_log(r, "Unexpected packet: %X\n", h->type);
		return PTP_IO_ERR;
	}

	ptp_verbose_log(r, "ptpip_receive_bulk_packets: Return code: 0x%X\n", ptp_get_return_code(r));

	return 0;
}

int ptpusb_read_all_packets(struct PtpRuntime *r) {
	r->wait_for_response = r->response_wait_default;
	int rc;
	int read = 0;
	int read_attempts = 0;
	while (1) {
		if (r->data_length < (read + r->max_packet_size)) {
			rc = ptp_buffer_resize(r, read + r->max_packet_size);
			r->data_filled_length = read;
			if (rc) return rc;
		}

		if (r->connection_type == PTP_USB) {
			rc = ptp_cmd_read(r, r->data + read, r->max_packet_size);
		} else if (r->connection_type == PTP_IP_USB) {
			rc = ptpip_cmd_read(r, r->data + read, r->max_packet_size);
		} else {
			ptp_panic("illegal connection type");
		}
		if (rc < 0 && r->wait_for_response) {
			ptp_error_log(r, "Response error %d, trying again\n", rc);
			r->wait_for_response--;
			PTP_SLEEP(PTP_WAIT_MS);
			read_attempts++;
			continue;
		}
		if (rc == 0 && r->wait_for_response) {
			ptp_error_log(r, "Got nothing, trying again\n", rc);
			r->wait_for_response--;
			read_attempts++;
			continue;
		}
		if (rc < 0) {
			r->data_filled_length = read;
			return PTP_IO_ERR;
		}
		read += rc;

		if (r->wait_for_response == 0 && read_attempts) {
			ptp_error_log(r, "Too many attempts, didn't get enough bytes\n");
			r->data_filled_length = read;
			return PTP_COMMAND_IGNORED;
		}

		// Min packet size is at least read
		if (read < 12) continue;

		uint32_t length;
		uint16_t type;
		struct PtpBulkContainer *c = (struct PtpBulkContainer *)(r->data);
		ptp_read_u32(&c->length, &length);
		ptp_read_u16(&c->type, &type);

		// First packet is at least read
		if (read < length) continue;

		if (type == PTP_PACKET_TYPE_RESPONSE) {
			break;
		}
		if (type == PTP_PACKET_TYPE_DATA) {
			// Min data packet is at least read
			if (length + 12 > read) continue;

			c = (struct PtpBulkContainer *)(r->data + length);
			uint32_t data_length;
			ptp_read_u32(&c->length, &data_length);
			ptp_read_u16(&c->type, &type);
			if (type != PTP_PACKET_TYPE_RESPONSE) {
				ptp_error_log(r, "Expected response packet but got %d\n", type);
				r->data_filled_length = read;
				return PTP_IO_ERR;
			}
			if (read == data_length + length) {
				break;
			}
			if (read > data_length + length) {
				// TODO: This is a valid scenario when receiving >4GB objects in PtpGetObject
				// In that case, the data packet length will be 0xffffffff and the size of the payload returned
				// will be the CompressedSize field in PtpObjectInfo for that file.
				ptp_error_log(r, "Read too much data %d\n", read);
				r->data_filled_length = read;
				return PTP_IO_ERR;
			}
		}
	}
	r->data_filled_length = read;
	return 0;
}

int ptp_receive_all_packets(struct PtpRuntime *r) {
	int rc = 0;
	if (r->connection_type == PTP_IP) {
		rc = ptpip_receive_bulk_packets(r);
	} else if (r->connection_type == PTP_USB || r->connection_type == PTP_IP_USB) {
		rc = ptpusb_read_all_packets(r);
	} else {
		ptp_panic("Unknown connection type %d", r->connection_type);
	}

	if (r->comm_dump != NULL) {
		fwrite(r->data, 1, r->data_filled_length, r->comm_dump);
	}

	return rc;
}
