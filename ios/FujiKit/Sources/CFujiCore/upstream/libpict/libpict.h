/** \file */
// Copyright 2022 by Daniel C (https://github.com/petabyt/libpict)
#ifndef PTP_LIB_H
#define PTP_LIB_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#include "ptp.h"

// Max timeout for command read/writes
#define PTP_TIMEOUT 1000

// Units of time in ms to sleep for r->wait_for_response
#define PTP_WAIT_MS 1000

// Conforms to POSIX 2001, some compilers may not have it
#ifndef PTP_SLEEP
	#include <unistd.h>
	//int usleep(unsigned int usec);
	#define PTP_SLEEP(ms) usleep((unsigned int)(ms) * 1000)
#endif

#ifdef WIN32
#define PUB __declspec(dllexport)
#else
#define PUB
#endif

// 1mb default buffer size
#define PTP_DEFAULT_SIZE 1000000

/// @brief Library errors, not PTP return codes
enum PtpGeneralError {
	PTP_OK = 0,
	/// @brief No device found (USB)
	PTP_NO_DEVICE = -1,
	/// @brief EPERM or other permission denied error
	PTP_NO_PERM = -2,
	/// @brief Found device, but failed to connect
	PTP_OPEN_FAIL = -3,
	/// @brief malloc failed
	PTP_OUT_OF_MEM = -4,
	/// @brief General IO or communication error
	PTP_IO_ERR = -5,
	/// @brief Unexpected, unhandled, or illegal behavior
	PTP_RUNTIME_ERR = -6,
	/// @brief Operation or functionality isn't implemented or supported
	PTP_UNSUPPORTED = -7,
	/// @brief response code is not PTP_RC_OK
	PTP_CHECK_CODE = -8,
	/// @brief Operation (such as download) was canceled by another thread
	PTP_CANCELED = -9,
	/// @brief No response 
	/// @note Used internally only
	PTP_COMMAND_IGNORED = -10,
};

/// @brief Evaluates PtpGeneralError into string message
const char *ptp_perror(int rc);

/// @brief Unique camera types - each type should have similar opcodes and behavior
enum PtpVendors {
	PTP_DEV_EMPTY = 0,
	PTP_DEV_EOS = 1, // EOS DSLR/Mirrorless systems
	PTP_DEV_CANON = 2, // Older powershot cameras
	PTP_DEV_NIKON = 3,
	PTP_DEV_SONY = 4,
	PTP_DEV_FUJI = 5,
	PTP_DEV_PANASONIC = 6,
};

// Wrapper types for vendor specific capture modes
enum ImageFormats {
	IMG_FORMAT_ETC = 0,
	IMG_FORMAT_RAW = 1,
	IMG_FORMAT_STD = 2,
	IMG_FORMAT_HIGH = 3,
	IMG_FORMAT_RAW_JPEG = 4,
};

/// @brief Tells lib what backend and packet style to use
/// @note This is a bitmask so that it can be passed to ptp_new with other options.
enum PtpConnType {
	PTP_IP = (1 << 0),
	PTP_IP_USB = (1 << 1), // TCP-based, but using USB-style packets (Fujifilm)
	PTP_USB = (1 << 2),
	PTP_BLE = (1 << 3), // I can only dream...
};

/// @brief Linked list to handle currently possible values for a property
struct PtpPropAvail {
	void *next;
	void *prev;
	int code;
	unsigned int memb_size;
	unsigned int memb_cnt;
	void *data;
};

/// @brief Represents a single device connection
/// @struct PtpRuntime
struct PtpRuntime {
	/// @brief Set to 1 when it is no longer safe to send any data to the device (socket closed, device unplugged)
	uint8_t io_kill_switch;

	/// @brief Set to 1 when it is no longer safe to run operations (device is unresponsive, pipe issues)
	uint8_t operation_kill_switch;

	/// @brief One of enum PtpConnType
	/// @note Is set to USB by default
	uint8_t connection_type;

	/// @brief Current transaction ID
	/// @note The transaction ID and session ID is managed by the packet generator functions
	int transaction;
	/// @brief Current session ID
	int session;

	/// @brief Global buffer for data reading and writing
	/// @note This buffer is reallocated during runtime to grow as needed
    uint8_t *data;
	/// @brief Size of data in bytes
    unsigned int data_length;
	/// @brief Size of valid PTP data in data in bytes
	unsigned int data_filled_length;

	/// @note Max size of a USB bulk transfer or max TCP packet size
	/// @todo need to have a max packet size for both IN and OUT
	unsigned int max_packet_size;

	/// @brief Info about current connection, used to detect camera type, supported opodes, etc
	/// @note Set by ptp_parse_device_info. This should be NULL when this struct is created.
	struct PtpDeviceInfo *di;

	/// @brief For Windows compatibility, this is set to indicate lenth for a data packet
	/// that will be sent after a command packet. Will be set to zero when ptp_send_packet is called.
	unsigned int data_phase_length;

	/// @brief Private instance of comm/io structure
	struct PtpCommPriv *comm_priv;

	/// @brief void* pointer for comm/io instance
	/// @todo: deprecate
	void *comm_backend;

	/// @Brief Optional function that will be called every time a chunk of data is read
	/// Meant for updating progress bars. NULL by default.
	void (*report_read_progress)(struct PtpRuntime *r, unsigned int size);

	/// @brief Called after ptp/ip socket() call, meant for binding to other interfaces when necessary
	/// NULL by default
	int (*set_extra_socket_settings)(struct PtpRuntime *r, int sockfd);

	/// @brief Free pointer to hold per ptp session information
	void *userdata;

	/// @brief Struct to hold per ptp session information, can be set freely
	struct PtpUserPriv *priv;

	/// @brief Optional (see PTP_DONT_USE_MUTEX)
	pthread_mutex_t *mutex;

	/// @brief Optionally wait up to 256 seconds for a response. Some PTP operations require this, such as EOS capture.
	/// @note Not thread safe. Will be reset after each operation.
	uint8_t wait_for_response;

	/// @brief Default value for wait_for_response.
	uint8_t response_wait_default;

	/// @brief For devices that implement it, this will hold a linked list of properties and an array of their supported values.
	/// generic_ functions will reject set property calls if an invalid value is written.
	/// @note: Optional
	struct PtpPropAvail *avail;

	/// @brief If non-NULL, all reads/writes will be logged to this file
	FILE *comm_dump;
};

// Logging+panic mechanism, define it yourself or link in log.c
/// @brief Verbose log debugging info, could be called dozens of times per second
PUB void ptp_verbose_log(struct PtpRuntime *r, char *fmt, ...);
/// @brief Used for critical IO errors, (not runtime errors)
PUB void ptp_error_log(struct PtpRuntime *r, char *fmt, ...);
/// @brief Client has no way out, crash the application
PUB __attribute__ ((noreturn)) void ptp_panic(char *fmt, ...);

/// @brief Generic event / property change
struct PtpGenericEvent {
	uint16_t code;
	const char *name;
	int value;
	const char *str_value;
};
/// @brief Stream reader state struct for struct PtpGenericEvent
struct PtpEventReader {
	unsigned int data_of;
	unsigned int n_entries;
	unsigned int index;
	void *ptr;
};

/// @brief Generic PTP command structure - accepted by operation API
struct PtpCommand {
	uint16_t code;
	uint32_t params[5];
	unsigned int param_length;
};

/// @brief Generic Struct for arrays
struct PtpArray {
	uint32_t length;
	uint32_t data[];
};

/// @brief Returns the return code (RC) currently in the data buffer.
/// @note Not thread safe.
PUB int ptp_get_return_code(struct PtpRuntime *r);

/// @brief Get number of parameters in packet in data buffer
/// @note Not thread safe.
PUB unsigned int ptp_get_param_length(struct PtpRuntime *r);

/// @brief Get parameter at index i
/// @note Not thread safe.
PUB uint32_t ptp_get_param(struct PtpRuntime *r, int i);

/// @brief Get transaction ID of packet in the data buffer
/// @note Not thread safe.
PUB int ptp_get_last_transaction_id(struct PtpRuntime *r);

/// @brief Get ptr of packet payload in data buffer, after packet header
/// @note Not thread safe.
PUB uint8_t *ptp_get_payload(struct PtpRuntime *r);

/// @brief Get length of payload returned by ptp_get_payload
/// @note Not thread safe.
PUB unsigned int ptp_get_payload_length(struct PtpRuntime *r);

/// @brief Allocate new PtpRuntime based on bitfield options - see PtpConnType
PUB struct PtpRuntime *ptp_new(int options);

/// @brief Resets all session-specific fields of PtpRuntime
/// This must be called if you plan to use the same PtpRuntime between connections
PUB void ptp_reset(struct PtpRuntime *r);

/// @brief Init PtpRuntime struct
PUB void ptp_init(struct PtpRuntime *r);

/// @brief Frees PtpRuntime data buffer - doesn't free the actual structure, or device info (yet)
PUB void ptp_close(struct PtpRuntime *r);

/// @brief Send a command request to the device with no data phase
PUB int ptp_send(struct PtpRuntime *r, struct PtpCommand *cmd);

/// @brief Send a command request to the device with a data phase (thread safe)
PUB int ptp_send_data(struct PtpRuntime *r, const struct PtpCommand *cmd, const void *data, unsigned int length);

/// @brief Try and get an event from the camera over int endpoint (USB-only)
PUB int ptp_get_event(struct PtpRuntime *r, struct PtpEventContainer *ec);

/// @brief Unlock the IO mutex (unless it was kept locked)
PUB void ptp_mutex_unlock(struct PtpRuntime *r);

/// @brief Completely unlock the mutex for the current thread, to ensure there isn't a deadlock.
/// This is normally used on handling errors, and when exiting a thread.
PUB void ptp_mutex_unlock_thread(struct PtpRuntime *r);

/// @brief Lock the IO mutex - only should be used by backend
PUB void ptp_mutex_lock(struct PtpRuntime *r);

/// @brief Gets type of device from r->di
/// @returns enum PtpDeviceType
PUB int ptp_device_type(struct PtpRuntime *r);

/// @brief Check if an opcode is supported by looking through supported props in r->di
/// @returns 1 if yes, 0 if no
PUB int ptp_check_opcode(struct PtpRuntime *r, int opcode);

/// @brief Check if a property code is supported by looking through supported props in r->di
/// @returns 1 if yes, 0 if no
PUB int ptp_check_prop(struct PtpRuntime *r, int code);

/// @brief Mostly for internal use - realloc the data buffer
/// @note r->data will be reassigned, any old references must be updated
PUB int ptp_buffer_resize(struct PtpRuntime *r, size_t size);

// Data structure functions
PUB int ptp_write_unicode_string(char *dat, const char *string);
PUB int ptp_read_unicode_string(char *buffer, const char *dat, int max);
PUB int ptp_read_utf8_string(void *dat, char *string, int max);
PUB int ptp_read_string(uint8_t *dat, char *string, int max);
PUB int ptp_write_string(uint8_t *dat, const char *string);
PUB int ptp_write_utf8_string(void *dat, const char *string);
PUB int ptp_read_uint16_array(const uint8_t *dat, uint16_t *buf, int max, int *length);
PUB int ptp_read_uint16_array_s(uint8_t *bs, uint8_t *be, uint16_t *buf, int max, int *length);
inline static int ptp_write_u8(void *buf, uint8_t out) {
	((uint8_t *)buf)[0] = out;
	return 1;
}
inline static int ptp_write_u16(void *buf, uint16_t out) {
	uint8_t *b = (uint8_t *)buf;
	b[0] = out & 0xFF;
	b[1] = (out >> 8) & 0xFF;
	return 2;
}
inline static int ptp_write_u32(void *buf, uint32_t out) {
	uint8_t *b = (uint8_t *)buf;
	b[0] = out & 0xFF;
	b[1] = (out >> 8) & 0xFF;
	b[2] = (out >> 16) & 0xFF;
	b[3] = (out >> 24) & 0xFF;
	return 4;
}
inline static int ptp_read_u8(const void *buf, uint8_t *out) {
	const uint8_t *b = (const uint8_t *)buf;
	*out = b[0];
	return 1;
}
inline static int ptp_read_u16(const void *buf, uint16_t *out) {
	const uint8_t *b = (const uint8_t *)buf;
	*out = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
	return 2;
}
inline static int ptp_read_u32(const void *buf, uint32_t *out) {
	const uint8_t *b = (const uint8_t *)buf;
	*out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
	return 4;
}
inline static int ptp_read_u64(const void *buf, uint64_t *out) {
	uint32_t lo, hi;
	ptp_read_u32(buf, &lo);
	ptp_read_u32((const uint8_t *)buf + 4, &hi);
	*out = ((uint64_t)hi << 32) | lo;
	return 8;
}

// Build a new PTP/IP or PTP/USB command packet in r->data
unsigned int ptp_new_cmd_packet(struct PtpRuntime *r, const struct PtpCommand *cmd);

// Only for PTP_USB or PTP_USB_IP use
int ptpusb_new_data_packet(struct PtpRuntime *r, const struct PtpCommand *cmd, const void *data, unsigned int data_length);

// Only use for PTP_IP
unsigned int ptpip_data_start_packet(struct PtpRuntime *r, unsigned int data_length);
unsigned int ptpip_data_end_packet(struct PtpRuntime *r, const void *data, unsigned int data_length);

// Set avail info for prop
void ptp_set_prop_avail_info(struct PtpRuntime *r, int code, unsigned int memb_size, unsigned int cnt, void *data);

/// @brief Write r->data to a file called DUMP
/// @note Debugging only
int ptp_dump(struct PtpRuntime *r);

#include "cl_data.h"
#include "cl_backend.h"
#include "cl_ops.h"
#include "cl_enum.h"
#include "cl_bind.h"

#undef PUB // avoid conflict

#endif
