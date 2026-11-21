// Library frontend functions
// Copyright 2022 by Daniel C (https://github.com/petabyt/libpict)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libpict.h>
#include <ptp.h>

void ptp_reset(struct PtpRuntime *r) {
	if (r == NULL) abort();
	r->io_kill_switch = 1;
	r->operation_kill_switch = 1;
	r->transaction = 0;
	r->session = 0;
	r->connection_type = PTP_USB;
	r->response_wait_default = 1;
	r->wait_for_response = 1;
	r->comm_backend = NULL; // leak
	r->comm_dump = NULL;
}

void ptp_init(struct PtpRuntime *r) {
	if (r == NULL) abort();
	memset(r, 0, sizeof(struct PtpRuntime));
	ptp_reset(r);

	r->data = malloc(PTP_DEFAULT_SIZE);
	r->data_length = PTP_DEFAULT_SIZE;
	r->data_filled_length = 0;

	r->avail = calloc(1, sizeof(struct PtpPropAvail));

	#ifndef PTP_DONT_USE_MUTEX
	r->mutex = malloc(sizeof(pthread_mutex_t));

	// We want recursive mutex, so lock can be called multiple times in
	// a single thread
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	if (pthread_mutex_init(r->mutex, &attr)) {
		ptp_panic("Failed to init mutex\n");
	}
	#endif
}

struct PtpRuntime *ptp_new(int options) {
	struct PtpRuntime *r = malloc(sizeof(struct PtpRuntime));
	ptp_init(r);

	// TODO: Working but can maybe add more options?
	if (options & PTP_IP) {
		r->connection_type = PTP_IP;
	} else if (options & PTP_USB) {
		r->connection_type = PTP_USB;
	} else if (options & PTP_IP_USB) {
		r->connection_type = PTP_IP_USB;
	}

	return r;
}

void ptp_set_prop_avail_info(struct PtpRuntime *r, int code, unsigned int memb_size, unsigned int cnt, void *data) {
	if (r->avail == NULL) abort();
	// Traverse backwards to first item
	struct PtpPropAvail *n;
	for (n = r->avail; n != NULL; n = n->prev) {
		if (n->code == code) break;
	}

	if (n != NULL) {
		// Only realloc if needed (eventually will stop allocating once we have hit a maximum)
		if (cnt > n->memb_cnt) {
			n->data = realloc(n->data, memb_size * cnt);
		}

		memcpy(n->data, data, memb_size * cnt);
		return;
	}

	// Handle first element of linked list
	if (r->avail->code == 0x0) {
		n = r->avail;
	} else {
		n = calloc(1, sizeof(struct PtpPropAvail));
		r->avail->next = n;
		n->prev = r->avail;
	}

	n->code = code;
	n->memb_size = memb_size;
	n->memb_cnt = cnt;
	void *dup = malloc(memb_size * cnt);
	memcpy(dup, data, memb_size * cnt);
	n->data = dup;

	r->avail = n;
}

void ptpusb_free_device_list(struct PtpDeviceEntry *e) {
	struct PtpDeviceEntry *next;
	while (e != NULL) {
		next = e->next;
		ptpusb_free_device_list_entry(e->device_handle_ptr);
		free(e);
		e = next;
	}
}

int ptp_buffer_resize(struct PtpRuntime *r, size_t size) {
	if (size <= r->data_length) return 0;
	//if (size < r->data_length) {
	//	ptp_panic("You cannot downsize the data buffer (%u -> %u)", r->data_length, size);
	//}

	// realloc with a little extra space to minimize reallocs later on
	static int extra = 100;
	// data_length is 32-bit; refuse sizes it cannot represent.
	if (size > UINT32_MAX - extra) return PTP_OUT_OF_MEM;
	ptp_verbose_log(r, "Extending IO buffer to %zX\n", size + extra);
	r->data = realloc(r->data, size + extra);
	r->data_length = (unsigned int)(size + extra);
	if (r->data == NULL) {
		return PTP_OUT_OF_MEM;
	}

	return 0;
}

void ptp_mutex_lock(struct PtpRuntime *r) {
	if (r->mutex == NULL) return;
	pthread_mutex_lock(r->mutex);
}

__attribute__((deprecated()))
void ptp_mutex_keep_locked(struct PtpRuntime *r) {
	if (r->mutex == NULL) return;
	pthread_mutex_lock(r->mutex);
}

void ptp_mutex_unlock(struct PtpRuntime *r) {
	if (r->mutex == NULL) return;
	pthread_mutex_unlock(r->mutex);
}

void ptp_close(struct PtpRuntime *r) {
	ptp_comm_deinit(r);
	free(r->data);
    free(r);
}

void ptp_mutex_unlock_thread(struct PtpRuntime *r) {
	if (r->mutex == NULL) return;
	// Wait until we get EPERM (we do not own the mutex anymore)
	while (pthread_mutex_unlock(r->mutex) == 0) {
		ptp_verbose_log(r, "WARN: pid %d had mutex locked\n", getpid());
	}
}

static int ptp_check_rc(struct PtpRuntime *r) {
	int code = ptp_get_return_code(r);
	// This is returned on Fuji cameras
	if (code == 0xffff) {
		ptp_verbose_log(r, "Nonstandard goodbye return code 0xffff");
		return PTP_IO_ERR;
	}
	if (code != PTP_RC_OK) {
		ptp_verbose_log(r, "Invalid return code: %X\n", ptp_get_return_code(r));
		return PTP_CHECK_CODE;
	}

	return 0;
}

static int ptp_send_try(struct PtpRuntime *r, struct PtpCommand *cmd) {
	unsigned int length = ptp_new_cmd_packet(r, cmd);
	if (ptp_send_packet(r, length) != length) {
		ptp_verbose_log(r, "Didn't send all packets\n");
		return PTP_IO_ERR;
	}

	int rc = ptp_receive_all_packets(r);
	if (rc < 0) {
		ptp_verbose_log(r, "Failed to receive packets: %d\n", rc);
		return rc;
	}

	return 0;
}

static void log_cmd(struct PtpRuntime *r, const struct PtpCommand *cmd) {
	if (cmd->param_length == 0) ptp_verbose_log(r, "Sending %04x with no params\n", cmd->code, cmd->param_length);
	else if (cmd->param_length == 1) ptp_verbose_log(r, "Sending %04x with 1 param (%d)\n", cmd->code, cmd->params[0]);
	else if (cmd->param_length == 2) ptp_verbose_log(r, "Sending %04x with 2 params (%d, %d)\n", cmd->code, cmd->params[0], cmd->params[0]);
	else ptp_verbose_log(r, "Sending %04x with %d params\n", cmd->code, cmd->param_length);
}

// Perform a generic command transaction - no data phase
int ptp_send(struct PtpRuntime *r, struct PtpCommand *cmd) {
	if (r->operation_kill_switch) return PTP_IO_ERR;
	ptp_mutex_lock(r);
	if (r->operation_kill_switch) {
		ptp_mutex_unlock(r);
		return PTP_IO_ERR;
	}

	log_cmd(r, cmd);

	r->data_phase_length = 0;

	int rc = ptp_send_try(r, cmd);
	if (rc == PTP_COMMAND_IGNORED) {
		ptp_verbose_log(r, "Command ignored, trying again...\n");
		rc = ptp_send_try(r, cmd);
		if (rc) {
			ptp_verbose_log(r, "Command ignored again.\n");
			r->operation_kill_switch = 1;
			ptp_mutex_unlock(r);
			return PTP_IO_ERR;
		}
	} else if (rc) {
		ptp_mutex_unlock(r);
		r->operation_kill_switch = 1;
		return PTP_IO_ERR;
	}

	r->transaction++;

	rc = ptp_check_rc(r);
	ptp_mutex_unlock(r);
	return rc;
}

static int ptp_send_data_try(struct PtpRuntime *r, const struct PtpCommand *cmd, const void *data, unsigned int length) {
	// Send operation request (data packet later on)
	unsigned int plength = ptp_new_cmd_packet(r, cmd);
	if (ptp_send_packet(r, plength) != plength) {
		return PTP_IO_ERR;
	}

	if (r->connection_type == PTP_IP) {
		// Send data start packet first (only has payload length)
		plength = ptpip_data_start_packet(r, length);
		if (ptp_send_packet(r, plength) != plength) {
			return PTP_IO_ERR;
		}

		// Send data end packet, with payload
		plength = ptpip_data_end_packet(r, data, length);
		if (ptp_send_packet(r, plength) != plength) {
			return PTP_IO_ERR;
		}
	} else {
		// Single data packet
		plength = ptpusb_new_data_packet(r, cmd, data, length);
		if (ptp_send_packet(r, plength) != plength) {
			ptp_verbose_log(r, "Failed to send data packet (%d)\n", plength);
			return PTP_IO_ERR;
		}
	}

	int rc = ptp_receive_all_packets(r);
	if (rc < 0) return rc;
	return 0;
}

// Perform a command request with a data phase to the camera
int ptp_send_data(struct PtpRuntime *r, const struct PtpCommand *cmd, const void *data, unsigned int length) {
	if (r->operation_kill_switch) return PTP_IO_ERR;
	ptp_mutex_lock(r);
	if (r->operation_kill_switch) {
		ptp_mutex_unlock(r);
		return PTP_IO_ERR;
	}

	log_cmd(r, cmd);

	// Required for PTP/IP
	r->data_phase_length = length;

	// Resize buffer if needed
	if (length + 50 > r->data_length) {
		int rc = ptp_buffer_resize(r, length);
		ptp_mutex_unlock(r);
		if (rc) return rc;
	}

	// If our command is ignored (we get 0 bytes as a response), try sending the
	// commands again.
	int rc = ptp_send_data_try(r, cmd, data, length);
	if (rc == PTP_COMMAND_IGNORED) {
		ptp_verbose_log(r, "Command ignored, trying again...\n");
		rc = ptp_send_data_try(r, cmd, data, length);
		if (rc) {
			ptp_verbose_log(r, "Command ignored again.\n");
			r->operation_kill_switch = 1;
			ptp_mutex_unlock(r);
			return PTP_IO_ERR;
		}
	} else if (rc) {
		ptp_mutex_unlock(r);
		r->operation_kill_switch = 1;
		return PTP_IO_ERR;
	}

	r->transaction++;

	rc = ptp_check_rc(r);
	ptp_mutex_unlock(r);
	return rc;
}

int ptp_device_type(struct PtpRuntime *r) {
	struct PtpDeviceInfo *di = r->di;
	if (di == NULL) return PTP_DEV_EMPTY; // TODO: Run GetDeviceInfo manually?
	if (!strcmp(di->manufacturer, "Canon Inc.") || !strcmp(di->manufacturer, "Canon.Inc")) {
		if (ptp_check_opcode(r, PTP_OC_EOS_GetStorageIDs)) {
			return PTP_DEV_EOS;
		}

		return PTP_DEV_CANON;
	} else if (!strcmp(di->manufacturer, "FUJIFILM")) {
		return PTP_DEV_FUJI;
	} else if (!strcmp(di->manufacturer, "Sony Corporation")) {
		return PTP_DEV_SONY;
	} else if (!strcmp(di->manufacturer, "Nikon Corporation")) {
		return PTP_DEV_NIKON;
	}

	return PTP_DEV_EMPTY;
}

int ptp_check_opcode(struct PtpRuntime *r, int op) {
	if (r->di == NULL) return 0;
	for (int i = 0; i < r->di->ops_supported_length; i++) {
		if (r->di->ops_supported[i] == op) {
			return 1;
		}
	}

	return 0;
}

int ptp_check_prop(struct PtpRuntime *r, int code) {
	if (r->di == NULL) return 0;
	for (int i = 0; i < r->di->props_supported_length; i++) {
		if (r->di->props_supported[i] == code) {
			return 1;
		}
	}

	return 0;
}

const char *ptp_perror(int rc) {
	switch (rc) {
	case PTP_OK: return "OK";
	case PTP_NO_DEVICE: return "No device found";
	case PTP_NO_PERM: return "Lacking permissions";
	case PTP_OPEN_FAIL: return "Failed opening device";
	case PTP_OUT_OF_MEM: return "Out of memory";
	case PTP_IO_ERR: return "I/O Error";
	case PTP_RUNTIME_ERR: return "Runtime error";
	case PTP_UNSUPPORTED: return "Unsupported operation";
	case PTP_CHECK_CODE: return "Check code";
	default: return "?";
	}
}

int ptp_dump(struct PtpRuntime *r) {
	FILE *f = fopen("DUMP", "w");
	fwrite(r->data, r->data_length, 1, f);
	fclose(f);
	return 0;
}
