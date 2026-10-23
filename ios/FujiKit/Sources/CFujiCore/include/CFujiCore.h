// Thin C API between Swift and the vendored libfuji/libpict/fp core.
//
// Threading contract:
// - Every function taking a fujiios_session, except fujiios_abort() and
//   fujiios_request_cancel(), must be called from ONE thread at a time (the
//   app uses a single dedicated PTP worker thread per session). PTP is a
//   strictly sequential request/response protocol; libpict additionally holds
//   a recursive mutex per transaction.
// - fujiios_abort() and fujiios_request_cancel() may be called from any thread.
//
// Memory contract:
// - fujiios_buffer.data is malloc()ed by this library; release it with
//   fujiios_buffer_free(). Pointers handed to callbacks are only valid for the
//   duration of the callback.
// - Strings in structs are NUL-terminated and truncated to their array size.
#ifndef CFUJICORE_H
#define CFUJICORE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Return codes. Values -1...-10 are identical to libpict's enum PtpGeneralError.
enum {
	FUJIIOS_OK = 0,
	FUJIIOS_ERR_NO_DEVICE = -1,
	FUJIIOS_ERR_NO_PERM = -2,
	FUJIIOS_ERR_OPEN_FAIL = -3,
	FUJIIOS_ERR_OUT_OF_MEM = -4,
	FUJIIOS_ERR_IO = -5,
	FUJIIOS_ERR_RUNTIME = -6,
	FUJIIOS_ERR_UNSUPPORTED = -7,
	/// The camera answered with a PTP response code other than OK.
	/// See fujiios_last_response_code().
	FUJIIOS_ERR_CHECK_CODE = -8,
	FUJIIOS_ERR_CANCELED = -9,
	FUJIIOS_ERR_IGNORED = -10,
	FUJIIOS_ERR_INVALID_ARG = -100,
	FUJIIOS_ERR_NOT_CONNECTED = -101,
	FUJIIOS_ERR_ABORTED = -102,
};

/// Values identical to libfuji's enum FujiTransport (lib/fujiptp.h).
enum {
	/// 'WIRELESS COMMUNICATION' started from the camera menu (Camera Remote era)
	FUJIIOS_TRANSPORT_WIRELESS_COMM = 2,
	/// Wi-Fi started through a Bluetooth handoff (XApp era)
	FUJIIOS_TRANSPORT_XAPP_WIRELESS_COMM = 9,
};

enum {
	FUJIIOS_LOG_VERBOSE = 0,
	FUJIIOS_LOG_INFO = 1,
	FUJIIOS_LOG_ERROR = 2,
};

/// Called synchronously on whatever thread produced the message.
typedef void (*fujiios_log_fn)(void *user, int32_t level, const char *message);

typedef struct fujiios_session fujiios_session;

typedef struct {
	uint8_t *data;
	uint32_t length;
} fujiios_buffer;

/// Mirrors libpict's struct PtpObjectInfo.
typedef struct {
	uint32_t storage_id;
	uint16_t object_format;
	uint16_t protection;
	uint32_t compressed_size;
	uint16_t thumb_format;
	uint32_t thumb_compressed_size;
	uint32_t thumb_width;
	uint32_t thumb_height;
	uint32_t image_width;
	uint32_t image_height;
	uint32_t image_bit_depth;
	uint32_t parent_object;
	uint16_t association_type;
	uint32_t association_desc;
	uint32_t sequence_number;
	char filename[64];
	char date_created[32];
	char date_modified[32];
	char keywords[64];
} fujiios_object_info;

/// Snapshot of libfuji's struct PtpUserPriv plus shim bookkeeping.
typedef struct {
	int32_t transport;
	int32_t camera_state;
	int32_t num_objects;
	int32_t selected_imgs_mode;
	int32_t remote_version;
	int32_t image_get_version;
	int32_t get_object_version;
	int32_t remote_image_view_version;
	int32_t remote_image_view_ex_version;
	int32_t sort_by_oldest_first;
	int32_t opened_liveview_sockets;
	int32_t compress_small;
	uint32_t storage_info_updates;
	uint32_t pushed_handles_pending;
	char camera_name[64];
	char storage_name[64];
} fujiios_state;

typedef struct {
	void *user;
	/// Called once with the object info before data. Return nonzero to stop
	/// (e.g. not enough disk space); the download then returns FUJIIOS_ERR_CANCELED.
	int32_t (*on_info)(void *user, const fujiios_object_info *info);
	/// Called for every chunk in order. Return nonzero to cancel.
	int32_t (*on_data)(void *user, const uint8_t *data, uint32_t length, uint64_t offset, uint64_t total);
} fujiios_download_callbacks;

/// Property values observed around a download. -1 = could not be read,
/// -2 = not audited.
typedef struct {
	int32_t compress_small_before;   // 0xD226
	int32_t compress_small_after;
	int32_t correct_size_before;     // 0xD227
	int32_t correct_size_after;
	uint64_t expected_size;          // compressed_size reported by GetObjectInfo
	uint64_t bytes_received;         // including start_offset
	int32_t cleanup_rc;              // result of the post-download restore steps
	uint32_t last_response_code;     // PTP response code if rc == CHECK_CODE
} fujiios_download_report;

/// Process-wide setup (idempotent). On non-Apple platforms this ignores SIGPIPE
/// for the test harness; on Apple platforms SO_NOSIGPIPE is set per socket.
void fujiios_global_init(void);
/// Log sink for messages that have no session (ptp_panic).
void fujiios_set_global_log(fujiios_log_fn fn, void *user);

fujiios_session *fujiios_session_new(int32_t transport, fujiios_log_fn log_fn, void *log_user, int32_t min_log_level);
/// Closes any sockets and frees everything. The session must not be in use.
void fujiios_session_free(fujiios_session *s);

/// TCP connect to the camera command port. bind_ifname (nullable) pins the
/// sockets to a network interface (IP_BOUND_IF on Apple platforms).
int32_t fujiios_connect(fujiios_session *s, const char *ip, int32_t port, int32_t extra_timeout_s, const char *bind_ifname);
/// Runs libfuji fuji_setup(): Fuji init handshake, OpenSession, waits for the
/// user to accept on the camera, version negotiation, gallery mode.
int32_t fujiios_setup(fujiios_session *s, const char *client_name);
/// Any thread: shut down all sockets so the worker's blocking call returns.
void fujiios_abort(fujiios_session *s);
/// Any thread: request that a running download/discovery stops at the next chunk.
void fujiios_request_cancel(fujiios_session *s, int32_t cancel);
/// graceful != 0 sends CloseSession and Fuji's goodbye packet first.
int32_t fujiios_close(fujiios_session *s, int32_t graceful);
int32_t fujiios_is_connected(fujiios_session *s);

int32_t fujiios_get_state(fujiios_session *s, fujiios_state *out);
/// libfuji fuji_get_events(): reads the 0xD212 event list and updates state.
int32_t fujiios_poll_events(fujiios_session *s);
/// PTP response code of the last transaction that failed with CHECK_CODE.
uint32_t fujiios_last_response_code(fujiios_session *s);

/// libfuji ptp_fuji_get_object_handles(). Over Wi-Fi these are 1...num_objects
/// and are NOT stable identifiers.
int32_t fujiios_object_handles(fujiios_session *s, uint32_t *out, uint32_t max, uint32_t *count);
int32_t fujiios_get_object_info(fujiios_session *s, uint32_t handle, fujiios_object_info *out);
/// libfuji fuji_get_thumb(). out->length == 0 means the camera has no thumbnail.
/// If info_out is non-NULL and the camera required GetObjectInfo first, it is
/// filled and *info_valid set to 1.
int32_t fujiios_get_thumbnail(fujiios_session *s, uint32_t handle, fujiios_buffer *out, fujiios_object_info *info_out, int32_t *info_valid);
/// Mirrors libfuji fuji_download_file_ex() step for step (same property
/// sequence), adding a start offset and an optional property audit.
int32_t fujiios_download(fujiios_session *s, uint32_t handle, uint64_t start_offset, int32_t audit_properties,
	const fujiios_download_callbacks *callbacks, fujiios_download_report *report);
/// Single GetPartialObject; used to verify offset reads before resuming.
int32_t fujiios_read_partial(fujiios_session *s, uint32_t handle, uint32_t offset, uint32_t max, fujiios_buffer *out);
/// Only sets libfuji's compress_images_for_smartphone flag (used by XApp-mode downloads).
void fujiios_set_compress_small(fujiios_session *s, int32_t on);

int32_t fujiios_liveview_start(fujiios_session *s);
int32_t fujiios_liveview_stop(fujiios_session *s);
/// out->length == 0 means no frame was ready.
int32_t fujiios_liveview_frame(fujiios_session *s, fujiios_buffer *out);

/// Raw GetDevicePropValue payload.
int32_t fujiios_get_prop_raw(fujiios_session *s, uint16_t code, fujiios_buffer *out);
/// Raw GetDevicePropDesc payload. Parsed in Swift; libpict's parser aborts on
/// several valid descriptor forms.
int32_t fujiios_get_prop_desc_raw(fujiios_session *s, uint16_t code, fujiios_buffer *out);
int32_t fujiios_set_prop_raw(fujiios_session *s, uint16_t code, const uint8_t *data, uint32_t length);
/// Standard PTP GetDeviceInfo (0x1001) raw payload.
int32_t fujiios_device_info_raw(fujiios_session *s, fujiios_buffer *out);
/// Fuji GetDeviceInfo (0x902B) raw payload.
int32_t fujiios_vendor_device_info_raw(fujiios_session *s, fujiios_buffer *out);
int32_t fujiios_storage_ids_raw(fujiios_session *s, fujiios_buffer *out);
int32_t fujiios_storage_info_raw(fujiios_session *s, uint32_t storage_id, fujiios_buffer *out);

/// Object handles the camera pushed (camera_state == MULTIPLE_TRANSFER), oldest first.
int32_t fujiios_take_pushed_handles(fujiios_session *s, uint32_t *out, uint32_t max, uint32_t *count);

void fujiios_buffer_free(fujiios_buffer *buffer);
const char *fujiios_error_name(int32_t rc);

/// Finds the local interface whose IPv4 subnet contains `ip`.
/// Returns 0 and fills ifname/local_ip when found, -1 otherwise.
int32_t fujiios_find_interface_for_ipv4(const char *ip, char *ifname, size_t ifname_max, char *local_ip, size_t local_ip_max);

/// Parses an X RAW STUDIO FP1/FP2/FP3 recipe file with the fp library and
/// writes "<Key>Value</Key>" lines into out. Returns 0 on success; on error
/// out holds fp's error message.
int32_t fujiios_recipe_parse_file(const char *path, char *out, size_t max);

#ifdef __cplusplus
}
#endif

#endif
