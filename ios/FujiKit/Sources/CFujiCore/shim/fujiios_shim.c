// iOS/Swift glue for the vendored libfuji + libpict core.
// Implements the frontend callbacks libfuji expects (lib/app.h), libpict's
// logging hooks, and the fujiios_* API declared in include/CFujiCore.h.
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <libpict.h>
#include "fuji.h"
#include "fujiptp.h"
#include "app.h"
#include "fp.h"
#include "../include/CFujiCore.h"

_Static_assert(FUJIIOS_TRANSPORT_WIRELESS_COMM == FUJI_FEATURE_WIRELESS_COMM, "transport mismatch");
_Static_assert(FUJIIOS_TRANSPORT_XAPP_WIRELESS_COMM == FUJI_FEATURE_XAPP_WIRELESS_COMM, "transport mismatch");
_Static_assert(FUJIIOS_ERR_CHECK_CODE == PTP_CHECK_CODE, "rc mismatch");
_Static_assert(FUJIIOS_ERR_CANCELED == PTP_CANCELED, "rc mismatch");
_Static_assert(FUJIIOS_ERR_IO == PTP_IO_ERR, "rc mismatch");

#define PUSHED_MAX 64

struct fujiios_session {
	struct PtpRuntime *r;
	/// Serialises fujiios_abort() (any thread) with socket close (worker).
	pthread_mutex_t io_lock;
	atomic_int cancel;
	atomic_int aborted;
	int connected;
	fujiios_log_fn log_fn;
	void *log_user;
	int min_log_level;
	char bind_ifname[IFNAMSIZ];
	char camera_name[64];
	uint32_t last_response_code;
	uint32_t storage_info_updates;
	uint32_t pushed[PUSHED_MAX];
	uint32_t pushed_count;
	// Last object info libfuji reported through plat_update_object_info()
	int last_oi_valid;
	uint32_t last_oi_handle;
	struct PtpObjectInfo last_oi;
};

static fujiios_log_fn g_log_fn = NULL;
static void *g_log_user = NULL;

static struct fujiios_session *session_of(struct PtpRuntime *r) {
	return r ? (struct fujiios_session *)r->userdata : NULL;
}

static void emit_log(struct fujiios_session *s, int level, const char *fmt, va_list ap) {
	fujiios_log_fn fn = g_log_fn;
	void *user = g_log_user;
	int min_level = FUJIIOS_LOG_VERBOSE;
	if (s != NULL && s->log_fn != NULL) {
		fn = s->log_fn;
		user = s->log_user;
		min_level = s->min_log_level;
	}
	if (fn == NULL || level < min_level) return;
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	size_t n = strlen(buffer);
	while (n > 0 && (buffer[n - 1] == '\n' || buffer[n - 1] == '\r')) buffer[--n] = '\0';
	fn(user, level, buffer);
}

static void shim_log(struct fujiios_session *s, int level, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	emit_log(s, level, fmt, ap);
	va_end(ap);
}

// ---------------------------------------------------------------------------
// libpict logging hooks (declared in libpict.h)

void ptp_verbose_log(struct PtpRuntime *r, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	emit_log(session_of(r), FUJIIOS_LOG_VERBOSE, fmt, ap);
	va_end(ap);
}

void ptp_error_log(struct PtpRuntime *r, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	emit_log(session_of(r), FUJIIOS_LOG_ERROR, fmt, ap);
	va_end(ap);
}

__attribute__((noreturn)) void ptp_panic(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	emit_log(NULL, FUJIIOS_LOG_ERROR, fmt, ap);
	va_end(ap);
	// libpict only panics on internal invariant violations; continuing would
	// corrupt memory. The app avoids the known panicking paths
	// (e.g. ptp_parse_prop_desc) and parses those payloads in Swift.
	abort();
}

// ---------------------------------------------------------------------------
// libfuji frontend callbacks (lib/app.h)

void app_send_cam_name(struct PtpRuntime *r, const char *name) {
	struct fujiios_session *s = session_of(r);
	if (s == NULL || name == NULL) return;
	snprintf(s->camera_name, sizeof(s->camera_name), "%s", name);
}

void app_update_storage_info(struct PtpRuntime *r) {
	struct fujiios_session *s = session_of(r);
	if (s != NULL) s->storage_info_updates++;
}

void plat_dbg(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	emit_log(NULL, FUJIIOS_LOG_VERBOSE, fmt, ap);
	va_end(ap);
}

void app_print(struct PtpRuntime *r, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	emit_log(session_of(r), FUJIIOS_LOG_INFO, fmt, ap);
	va_end(ap);
}

void tester_log(struct PtpRuntime *r, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	emit_log(session_of(r), FUJIIOS_LOG_INFO, fmt, ap);
	va_end(ap);
}

void tester_fail(struct PtpRuntime *r, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	emit_log(session_of(r), FUJIIOS_LOG_ERROR, fmt, ap);
	va_end(ap);
}

void app_report_download_speed(struct PtpRuntime *r, long time, size_t size) {
	// Swift measures throughput itself from the data callback.
	(void)r; (void)time; (void)size;
}

int app_check_thread_cancel(struct PtpRuntime *r) {
	struct fujiios_session *s = session_of(r);
	return s != NULL && atomic_load(&s->cancel);
}

int fuji_discovery_check_cancel(struct PtpRuntime *r) {
	return app_check_thread_cancel(r);
}

int plat_update_object_info(struct PtpRuntime *r, int handle, const struct PtpObjectInfo *oi) {
	struct fujiios_session *s = session_of(r);
	if (s == NULL || oi == NULL) return 0;
	s->last_oi = *oi;
	s->last_oi_handle = (uint32_t)handle;
	s->last_oi_valid = 1;
	return 0;
}

int app_ptp_download_file(struct PtpRuntime *r, struct PtpObjectInfo *oi, int object_id, unsigned int max_chunk_size, int index) {
	// Only used by libfuji's wireless tether path, which the app does not enable.
	(void)r; (void)oi; (void)object_id; (void)max_chunk_size; (void)index;
	return PTP_UNSUPPORTED;
}

int app_queue_file_for_download(struct PtpRuntime *r, int object_id) {
	// Called from inside fuji_get_events() when the camera pushes images
	// (camera_state == FUJI_MULTIPLE_TRANSFER). Only record the handle here; the
	// Swift transfer queue downloads it as a normal job later.
	struct fujiios_session *s = session_of(r);
	if (s == NULL) return 0;
	for (uint32_t i = 0; i < s->pushed_count; i++) {
		if (s->pushed[i] == (uint32_t)object_id) return 0;
	}
	if (s->pushed_count < PUSHED_MAX) {
		s->pushed[s->pushed_count++] = (uint32_t)object_id;
	} else {
		shim_log(s, FUJIIOS_LOG_ERROR, "pushed handle queue full, dropping %d", object_id);
	}
	return 0;
}

// ---------------------------------------------------------------------------
// Socket setup hook (called by libpict right after socket())

static int socket_settings_cb(struct PtpRuntime *r, int fd) {
	struct fujiios_session *s = session_of(r);
#ifdef SO_NOSIGPIPE
	// A camera that powers off mid-transfer must not kill the app with SIGPIPE.
	int one = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one)) != 0) {
		shim_log(s, FUJIIOS_LOG_ERROR, "SO_NOSIGPIPE failed: %d", errno);
	}
#endif
	if (s != NULL && s->bind_ifname[0] != '\0') {
#if defined(IP_BOUND_IF)
		unsigned int index = if_nametoindex(s->bind_ifname);
		if (index == 0 || setsockopt(fd, IPPROTO_IP, IP_BOUND_IF, &index, sizeof(index)) != 0) {
			shim_log(s, FUJIIOS_LOG_ERROR, "IP_BOUND_IF(%s) failed: %d", s->bind_ifname, errno);
		}
#else
		shim_log(s, FUJIIOS_LOG_VERBOSE, "interface binding not supported on this platform");
#endif
	}
	// Never return non-zero: libpict would return that value as if it were a
	// socket descriptor and leak the real one.
	return 0;
}

// ---------------------------------------------------------------------------

static int32_t finish(struct fujiios_session *s, int rc) {
	if (rc == PTP_CHECK_CODE) {
		s->last_response_code = (uint32_t)ptp_get_return_code(s->r);
	}
	if (rc != 0 && atomic_load(&s->aborted)) return FUJIIOS_ERR_ABORTED;
	return rc;
}

static int copy_payload(struct PtpRuntime *r, fujiios_buffer *out) {
	out->data = NULL;
	out->length = 0;
	unsigned int len = ptp_get_payload_length(r);
	if (len == 0) return 0;
	out->data = malloc(len);
	if (out->data == NULL) return PTP_OUT_OF_MEM;
	memcpy(out->data, ptp_get_payload(r), len);
	out->length = len;
	return 0;
}

static void convert_object_info(const struct PtpObjectInfo *in, fujiios_object_info *out) {
	memset(out, 0, sizeof(*out));
	out->storage_id = in->storage_id;
	out->object_format = in->obj_format;
	out->protection = in->protection;
	out->compressed_size = in->compressed_size;
	out->thumb_format = in->thumb_format;
	out->thumb_compressed_size = in->thumb_compressed_size;
	out->thumb_width = in->thumb_width;
	out->thumb_height = in->thumb_height;
	out->image_width = in->img_width;
	out->image_height = in->img_height;
	out->image_bit_depth = in->img_bit_depth;
	out->parent_object = in->parent_obj;
	out->association_type = in->assoc_type;
	out->association_desc = in->assoc_desc;
	out->sequence_number = in->sequence_num;
	snprintf(out->filename, sizeof(out->filename), "%s", in->filename);
	snprintf(out->date_created, sizeof(out->date_created), "%s", in->date_created);
	snprintf(out->date_modified, sizeof(out->date_modified), "%s", in->date_modified);
	snprintf(out->keywords, sizeof(out->keywords), "%s", in->keywords);
}

static int32_t audit_prop(struct PtpRuntime *r, int code) {
	int rc = ptp_get_prop_value(r, code);
	if (rc) return -1;
	return (int32_t)ptp_parse_prop_value(r);
}

#define REQUIRE_SESSION(s) do { if ((s) == NULL || (s)->r == NULL) return FUJIIOS_ERR_INVALID_ARG; } while (0)
#define REQUIRE_CONNECTED(s) do { REQUIRE_SESSION(s); if (!(s)->connected) return FUJIIOS_ERR_NOT_CONNECTED; \
	if (atomic_load(&(s)->aborted)) return FUJIIOS_ERR_ABORTED; } while (0)

// ---------------------------------------------------------------------------
// Public API

void fujiios_global_init(void) {
#ifndef SO_NOSIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
}

void fujiios_set_global_log(fujiios_log_fn fn, void *user) {
	g_log_fn = fn;
	g_log_user = user;
}

fujiios_session *fujiios_session_new(int32_t transport, fujiios_log_fn log_fn, void *log_user, int32_t min_log_level) {
	if (transport != FUJIIOS_TRANSPORT_WIRELESS_COMM && transport != FUJIIOS_TRANSPORT_XAPP_WIRELESS_COMM) return NULL;
	struct fujiios_session *s = calloc(1, sizeof(*s));
	if (s == NULL) return NULL;
	pthread_mutex_init(&s->io_lock, NULL);
	atomic_init(&s->cancel, 0);
	atomic_init(&s->aborted, 0);
	s->log_fn = log_fn;
	s->log_user = log_user;
	s->min_log_level = min_log_level;
	s->r = fuji_ptp_new(PTP_IP_USB);
	if (s->r == NULL || s->r->priv == NULL) {
		pthread_mutex_destroy(&s->io_lock);
		free(s);
		return NULL;
	}
	s->r->userdata = s;
	s->r->set_extra_socket_settings = socket_settings_cb;
	fuji_get(s->r)->transport = (enum FujiTransport)transport;
	return s;
}

void fujiios_session_free(fujiios_session *s) {
	if (s == NULL) return;
	struct PtpRuntime *r = s->r;
	if (r != NULL) {
		pthread_mutex_lock(&s->io_lock);
		ptpip_comm_deinit(r);
		pthread_mutex_unlock(&s->io_lock);
		free(r->priv);
		r->priv = NULL;
		// r->avail is the tail of a prev-linked list (see ptp_set_prop_avail_info)
		struct PtpPropAvail *n = r->avail;
		while (n != NULL) {
			struct PtpPropAvail *prev = n->prev;
			free(n->data);
			free(n);
			n = prev;
		}
		r->avail = NULL;
		if (r->mutex != NULL) {
			pthread_mutex_destroy(r->mutex);
			free(r->mutex);
			r->mutex = NULL;
		}
		ptp_close(r); // frees r->data and r
		s->r = NULL;
	}
	pthread_mutex_destroy(&s->io_lock);
	free(s);
}

int32_t fujiios_connect(fujiios_session *s, const char *ip, int32_t port, int32_t extra_timeout_s, const char *bind_ifname) {
	REQUIRE_SESSION(s);
	if (ip == NULL || port <= 0 || port > 65535) return FUJIIOS_ERR_INVALID_ARG;
	if (atomic_load(&s->aborted)) return FUJIIOS_ERR_ABORTED;
	struct PtpRuntime *r = s->r;
	snprintf(s->bind_ifname, sizeof(s->bind_ifname), "%s", bind_ifname ? bind_ifname : "");
	snprintf(fuji_get(r)->ip_address, sizeof(fuji_get(r)->ip_address), "%s", ip);
	int rc = ptpip_connect(r, ip, port, extra_timeout_s);
	if (rc != 0) return atomic_load(&s->aborted) ? FUJIIOS_ERR_ABORTED : FUJIIOS_ERR_IO;
	s->connected = 1;
	if (atomic_load(&s->aborted)) {
		// fujiios_abort() ran while connect() was in progress and could not see the socket yet
		pthread_mutex_lock(&s->io_lock);
		ptpip_shutdown_io(r);
		pthread_mutex_unlock(&s->io_lock);
		return FUJIIOS_ERR_ABORTED;
	}
	return 0;
}

int32_t fujiios_setup(fujiios_session *s, const char *client_name) {
	REQUIRE_CONNECTED(s);
	// The Fuji init packet holds 27 UTF-16 code units including the terminator and
	// ptp_write_unicode_string() widens bytes, so restrict to printable ASCII.
	char name[27];
	size_t n = 0;
	for (const char *p = client_name ? client_name : ""; *p != '\0' && n < sizeof(name) - 1; p++) {
		if (*p >= 0x20 && *p < 0x7f) name[n++] = *p;
	}
	name[n] = '\0';
	if (n == 0) snprintf(name, sizeof(name), "iPhone");
	return finish(s, fuji_setup(s->r, name));
}

void fujiios_abort(fujiios_session *s) {
	if (s == NULL || s->r == NULL) return;
	atomic_store(&s->aborted, 1);
	atomic_store(&s->cancel, 1);
	pthread_mutex_lock(&s->io_lock);
	if (s->r->comm_priv != NULL) ptpip_shutdown_io(s->r);
	pthread_mutex_unlock(&s->io_lock);
}

void fujiios_request_cancel(fujiios_session *s, int32_t cancel) {
	if (s == NULL) return;
	atomic_store(&s->cancel, cancel ? 1 : 0);
}

int32_t fujiios_close(fujiios_session *s, int32_t graceful) {
	REQUIRE_SESSION(s);
	struct PtpRuntime *r = s->r;
	if (!s->connected) return 0;
	// Mirrors ptp_report_error(r, NULL, 0), but closes the sockets under io_lock
	// so a concurrent fujiios_abort() never touches a closed descriptor.
	if (graceful && !atomic_load(&s->aborted) && !r->operation_kill_switch) {
		r->wait_for_response = 1;
		ptp_close_session(r);
		uint8_t goodbye_packet[] = {0x8, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff};
		ptpip_cmd_write(r, goodbye_packet, sizeof(goodbye_packet));
	}
	pthread_mutex_lock(&s->io_lock);
	ptpip_device_close(r);
	pthread_mutex_unlock(&s->io_lock);
	r->io_kill_switch = 1;
	r->operation_kill_switch = 1;
	s->connected = 0;
	return 0;
}

int32_t fujiios_is_connected(fujiios_session *s) {
	if (s == NULL || s->r == NULL) return 0;
	return s->connected && !s->r->operation_kill_switch && !atomic_load(&s->aborted);
}

int32_t fujiios_get_state(fujiios_session *s, fujiios_state *out) {
	REQUIRE_SESSION(s);
	if (out == NULL) return FUJIIOS_ERR_INVALID_ARG;
	fujipriv_t *f = fuji_get(s->r);
	memset(out, 0, sizeof(*out));
	out->transport = (int32_t)f->transport;
	out->camera_state = f->camera_state;
	out->num_objects = f->num_objects;
	out->selected_imgs_mode = f->selected_imgs_mode;
	out->remote_version = f->remote_version;
	out->image_get_version = f->image_get_version;
	out->get_object_version = f->get_object_version;
	out->remote_image_view_version = f->remote_image_view_version;
	out->remote_image_view_ex_version = f->remote_image_view_ex_version;
	out->sort_by_oldest_first = f->sort_by_oldest_first;
	out->opened_liveview_sockets = f->opened_liveview_sockets;
	out->compress_small = f->compress_images_for_smartphone;
	out->storage_info_updates = s->storage_info_updates;
	out->pushed_handles_pending = s->pushed_count;
	snprintf(out->camera_name, sizeof(out->camera_name), "%s", s->camera_name);
	snprintf(out->storage_name, sizeof(out->storage_name), "%s", f->storage_device_name);
	return 0;
}

int32_t fujiios_poll_events(fujiios_session *s) {
	REQUIRE_CONNECTED(s);
	return finish(s, fuji_get_events(s->r));
}

uint32_t fujiios_last_response_code(fujiios_session *s) {
	return s ? s->last_response_code : 0;
}

int32_t fujiios_object_handles(fujiios_session *s, uint32_t *out, uint32_t max, uint32_t *count) {
	REQUIRE_CONNECTED(s);
	if (count == NULL) return FUJIIOS_ERR_INVALID_ARG;
	*count = 0;
	// ptp_fuji_get_object_handles() treats an empty card as an error over Wi-Fi.
	if (fuji_get(s->r)->num_objects == 0) return 0;
	struct PtpArray *a = NULL;
	int rc = ptp_fuji_get_object_handles(s->r, &a);
	if (rc) return finish(s, rc);
	if (a == NULL) return FUJIIOS_ERR_OUT_OF_MEM;
	for (uint32_t i = 0; i < a->length && i < max && out != NULL; i++) out[i] = a->data[i];
	*count = a->length;
	free(a);
	return 0;
}

int32_t fujiios_get_object_info(fujiios_session *s, uint32_t handle, fujiios_object_info *out) {
	REQUIRE_CONNECTED(s);
	if (out == NULL) return FUJIIOS_ERR_INVALID_ARG;
	struct PtpRuntime *r = s->r;
	ptp_mutex_lock(r);
	struct PtpObjectInfo oi;
	memset(&oi, 0, sizeof(oi));
	int rc = ptp_get_object_info(r, handle, &oi);
	if (rc == 0) convert_object_info(&oi, out);
	rc = finish(s, rc);
	ptp_mutex_unlock(r);
	return rc;
}

int32_t fujiios_get_thumbnail(fujiios_session *s, uint32_t handle, fujiios_buffer *out, fujiios_object_info *info_out, int32_t *info_valid) {
	REQUIRE_CONNECTED(s);
	if (out == NULL) return FUJIIOS_ERR_INVALID_ARG;
	out->data = NULL;
	out->length = 0;
	if (info_valid) *info_valid = 0;
	struct PtpRuntime *r = s->r;
	ptp_mutex_lock(r);
	s->last_oi_valid = 0;
	unsigned int offset = 0, length = 0;
	int rc = fuji_get_thumb(r, (int)handle, &offset, &length);
	if (rc == 0 && length > 0) {
		if ((uint64_t)offset + length > ptp_get_payload_length(r)) {
			rc = PTP_RUNTIME_ERR;
		} else {
			out->data = malloc(length);
			if (out->data == NULL) {
				rc = PTP_OUT_OF_MEM;
			} else {
				memcpy(out->data, ptp_get_payload(r) + offset, length);
				out->length = length;
			}
		}
	}
	if (info_out != NULL && s->last_oi_valid && s->last_oi_handle == handle) {
		convert_object_info(&s->last_oi, info_out);
		if (info_valid) *info_valid = 1;
	}
	rc = finish(s, rc);
	ptp_mutex_unlock(r);
	return rc;
}

int32_t fujiios_download(fujiios_session *s, uint32_t handle, uint64_t start_offset, int32_t audit_properties,
	const fujiios_download_callbacks *cb, fujiios_download_report *report) {
	REQUIRE_CONNECTED(s);
	if (cb == NULL || cb->on_data == NULL) return FUJIIOS_ERR_INVALID_ARG;
	struct PtpRuntime *r = s->r;
	fujipriv_t *fuji = fuji_get(r);
	fujiios_download_report local = {
		.compress_small_before = -2, .compress_small_after = -2,
		.correct_size_before = -2, .correct_size_after = -2,
	};
	int rc = 0;

	ptp_mutex_lock(r);

	if (audit_properties) {
		local.compress_small_before = audit_prop(r, PTP_DPC_FUJI_CompressSmall_D226);
		local.correct_size_before = audit_prop(r, PTP_DPC_FUJI_EnableCorrectFileSize_D227);
	}

	// --- Same preparation as fuji_download_file_ex() (lib/fuji.c) ---
	if (fuji->transport == FUJI_FEATURE_WIRELESS_COMM) {
		rc = fuji_get_events(r);
		if (rc) goto unlock;
		r->wait_for_response = 3;
		rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_EnableCorrectFileSize_D227, 1);
		if (rc == PTP_CHECK_CODE) {
			// Same as upstream: carry on; the reported size may then be wrong,
			// which the Swift side detects when validating the file.
			shim_log(s, FUJIIOS_LOG_ERROR, "D227=1 rejected (0x%X)", ptp_get_return_code(r));
			rc = 0;
		} else if (rc) {
			goto unlock;
		}
	} else if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
		ptp_set_prop_value16(r, PTP_DPC_FUJI_CompressSmall_D226, fuji->compress_images_for_smartphone ? 1 : 2);
	}

	struct PtpObjectInfo oi;
	memset(&oi, 0, sizeof(oi));
	rc = ptp_get_object_info(r, handle, &oi);
	if (rc) goto end;
	plat_update_object_info(r, (int)handle, &oi);
	local.expected_size = oi.compressed_size;

	if (cb->on_info != NULL) {
		fujiios_object_info info;
		convert_object_info(&oi, &info);
		if (cb->on_info(cb->user, &info)) {
			rc = PTP_CANCELED;
			goto end;
		}
	}

	if (oi.compressed_size == 0) {
		shim_log(s, FUJIIOS_LOG_ERROR, "camera reported size 0 for handle %u", handle);
		rc = PTP_RUNTIME_ERR;
		goto end;
	}
	if (start_offset > oi.compressed_size) {
		rc = FUJIIOS_ERR_INVALID_ARG;
		goto end;
	}

	uint64_t read = start_offset;
	while (read < oi.compressed_size) {
		if (atomic_load(&s->cancel)) {
			rc = PTP_CANCELED;
			goto end;
		}
		uint64_t remaining = oi.compressed_size - read;
		unsigned int cur = remaining > FUJI_MAX_PARTIAL_OBJECT ? FUJI_MAX_PARTIAL_OBJECT : (unsigned int)remaining;
		rc = ptp_get_partial_object(r, handle, (unsigned int)read, cur);
		if (rc) goto end; // CHECK_CODE: restore below. IO error: restore attempts fail fast.

		unsigned int payload_size = ptp_get_payload_length(r);
		if (payload_size == 0 || payload_size > cur) {
			shim_log(s, FUJIIOS_LOG_ERROR, "GetPartialObject returned %u bytes for a %u byte request", payload_size, cur);
			rc = PTP_RUNTIME_ERR;
			goto end;
		}
		if (cb->on_data(cb->user, ptp_get_payload(r), payload_size, read, oi.compressed_size)) {
			rc = PTP_CANCELED;
			goto end;
		}
		read += payload_size;
		local.bytes_received = read;
	}
	local.bytes_received = read;
	rc = 0;

	end:;
	// --- Same restore steps as fuji_download_file_ex(), without overwriting rc ---
	{
		int cleanup_rc = 0;
		if (rc == PTP_CHECK_CODE) local.last_response_code = (uint32_t)ptp_get_return_code(r);
		if (fuji->transport == FUJI_FEATURE_WIRELESS_COMM) {
			cleanup_rc = fuji_get_events(r);
			if (cleanup_rc == 0) cleanup_rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_EnableCorrectFileSize_D227, 0);
		} else if (fuji->transport == FUJI_FEATURE_XAPP_WIRELESS_COMM) {
			cleanup_rc = ptp_set_prop_value16(r, PTP_DPC_FUJI_CompressSmall_D226, 2);
		}
		local.cleanup_rc = cleanup_rc;
		if (audit_properties && !r->operation_kill_switch) {
			local.compress_small_after = audit_prop(r, PTP_DPC_FUJI_CompressSmall_D226);
			local.correct_size_after = audit_prop(r, PTP_DPC_FUJI_EnableCorrectFileSize_D227);
		}
		if (rc == 0 && cleanup_rc == PTP_IO_ERR) rc = cleanup_rc;
	}

	unlock:;
	rc = finish(s, rc);
	if (rc == FUJIIOS_ERR_CHECK_CODE && local.last_response_code == 0) local.last_response_code = s->last_response_code;
	ptp_mutex_unlock(r);
	if (report != NULL) *report = local;
	return rc;
}

int32_t fujiios_read_partial(fujiios_session *s, uint32_t handle, uint32_t offset, uint32_t max, fujiios_buffer *out) {
	REQUIRE_CONNECTED(s);
	if (out == NULL || max == 0 || max > FUJI_MAX_PARTIAL_OBJECT) return FUJIIOS_ERR_INVALID_ARG;
	struct PtpRuntime *r = s->r;
	ptp_mutex_lock(r);
	int rc = ptp_get_partial_object(r, handle, offset, max);
	if (rc == 0) rc = copy_payload(r, out);
	rc = finish(s, rc);
	ptp_mutex_unlock(r);
	return rc;
}

void fujiios_set_compress_small(fujiios_session *s, int32_t on) {
	if (s == NULL || s->r == NULL) return;
	fuji_get(s->r)->compress_images_for_smartphone = on ? 1 : 0;
}

int32_t fujiios_liveview_start(fujiios_session *s) {
	REQUIRE_CONNECTED(s);
	ptp_mutex_lock(s->r);
	int rc = fuji_config_liveview(s->r);
	rc = finish(s, rc);
	ptp_mutex_unlock(s->r);
	return rc;
}

int32_t fujiios_liveview_stop(fujiios_session *s) {
	REQUIRE_CONNECTED(s);
	// Same order as libfuji's module.c on_switch_screen(): end liveview, then
	// re-enter the image gallery.
	ptp_mutex_lock(s->r);
	int rc = fuji_end_liveview(s->r);
	int rc2 = fuji_config_image_gallery(s->r);
	rc = finish(s, rc ? rc : rc2);
	ptp_mutex_unlock(s->r);
	return rc;
}

int32_t fujiios_liveview_frame(fujiios_session *s, fujiios_buffer *out) {
	REQUIRE_CONNECTED(s);
	if (out == NULL) return FUJIIOS_ERR_INVALID_ARG;
	out->data = NULL;
	out->length = 0;
	struct PtpRuntime *r = s->r;
	ptp_mutex_lock(r);
	unsigned int size = 0;
	int rc = ptp_fuji_read_liveview_frame(r, &size);
	if (rc == 0 && size > 0) {
		out->data = malloc(size);
		if (out->data == NULL) {
			rc = PTP_OUT_OF_MEM;
		} else {
			memcpy(out->data, r->data, size);
			out->length = size;
		}
	}
	rc = finish(s, rc);
	ptp_mutex_unlock(r);
	return rc;
}

static int32_t send_and_copy(fujiios_session *s, uint16_t code, int nparams, uint32_t p0, fujiios_buffer *out) {
	REQUIRE_CONNECTED(s);
	if (out == NULL) return FUJIIOS_ERR_INVALID_ARG;
	out->data = NULL;
	out->length = 0;
	struct PtpRuntime *r = s->r;
	struct PtpCommand cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.code = code;
	cmd.param_length = (unsigned int)nparams;
	cmd.params[0] = p0;
	ptp_mutex_lock(r);
	int rc = ptp_send(r, &cmd);
	if (rc == 0) rc = copy_payload(r, out);
	rc = finish(s, rc);
	ptp_mutex_unlock(r);
	return rc;
}

int32_t fujiios_get_prop_raw(fujiios_session *s, uint16_t code, fujiios_buffer *out) {
	return send_and_copy(s, PTP_OC_GetDevicePropValue, 1, code, out);
}

int32_t fujiios_get_prop_desc_raw(fujiios_session *s, uint16_t code, fujiios_buffer *out) {
	return send_and_copy(s, PTP_OC_GetDevicePropDesc, 1, code, out);
}

int32_t fujiios_set_prop_raw(fujiios_session *s, uint16_t code, const uint8_t *data, uint32_t length) {
	REQUIRE_CONNECTED(s);
	if (data == NULL || length == 0 || length > 4096) return FUJIIOS_ERR_INVALID_ARG;
	uint8_t copy[4096];
	memcpy(copy, data, length);
	ptp_mutex_lock(s->r);
	int rc = ptp_set_prop_value_data(s->r, code, copy, length);
	rc = finish(s, rc);
	ptp_mutex_unlock(s->r);
	return rc;
}

int32_t fujiios_device_info_raw(fujiios_session *s, fujiios_buffer *out) {
	return send_and_copy(s, PTP_OC_GetDeviceInfo, 0, 0, out);
}

int32_t fujiios_vendor_device_info_raw(fujiios_session *s, fujiios_buffer *out) {
	return send_and_copy(s, PTP_OC_FUJI_GetDeviceInfo, 0, 0, out);
}

int32_t fujiios_storage_ids_raw(fujiios_session *s, fujiios_buffer *out) {
	return send_and_copy(s, PTP_OC_GetStorageIDs, 0, 0, out);
}

int32_t fujiios_storage_info_raw(fujiios_session *s, uint32_t storage_id, fujiios_buffer *out) {
	return send_and_copy(s, PTP_OC_GetStorageInfo, 1, storage_id, out);
}

int32_t fujiios_take_pushed_handles(fujiios_session *s, uint32_t *out, uint32_t max, uint32_t *count) {
	REQUIRE_SESSION(s);
	if (count == NULL) return FUJIIOS_ERR_INVALID_ARG;
	uint32_t n = s->pushed_count < max ? s->pushed_count : max;
	for (uint32_t i = 0; i < n && out != NULL; i++) out[i] = s->pushed[i];
	// Keep anything that did not fit for the next call
	memmove(s->pushed, s->pushed + n, (s->pushed_count - n) * sizeof(uint32_t));
	s->pushed_count -= n;
	*count = n;
	return 0;
}

void fujiios_buffer_free(fujiios_buffer *buffer) {
	if (buffer == NULL) return;
	free(buffer->data);
	buffer->data = NULL;
	buffer->length = 0;
}

const char *fujiios_error_name(int32_t rc) {
	switch (rc) {
	case FUJIIOS_OK: return "OK";
	case FUJIIOS_ERR_NO_DEVICE: return "NO_DEVICE";
	case FUJIIOS_ERR_NO_PERM: return "NO_PERM";
	case FUJIIOS_ERR_OPEN_FAIL: return "OPEN_FAIL";
	case FUJIIOS_ERR_OUT_OF_MEM: return "OUT_OF_MEM";
	case FUJIIOS_ERR_IO: return "IO_ERR";
	case FUJIIOS_ERR_RUNTIME: return "RUNTIME_ERR";
	case FUJIIOS_ERR_UNSUPPORTED: return "UNSUPPORTED";
	case FUJIIOS_ERR_CHECK_CODE: return "CHECK_CODE";
	case FUJIIOS_ERR_CANCELED: return "CANCELED";
	case FUJIIOS_ERR_IGNORED: return "COMMAND_IGNORED";
	case FUJIIOS_ERR_INVALID_ARG: return "INVALID_ARG";
	case FUJIIOS_ERR_NOT_CONNECTED: return "NOT_CONNECTED";
	case FUJIIOS_ERR_ABORTED: return "ABORTED";
	default: return "UNKNOWN";
	}
}

int32_t fujiios_find_interface_for_ipv4(const char *ip, char *ifname, size_t ifname_max, char *local_ip, size_t local_ip_max) {
	struct in_addr target;
	if (ip == NULL || inet_pton(AF_INET, ip, &target) != 1) return -1;
	struct ifaddrs *list = NULL;
	if (getifaddrs(&list) != 0) return -1;
	int32_t found = -1;
	for (struct ifaddrs *it = list; it != NULL; it = it->ifa_next) {
		if (it->ifa_addr == NULL || it->ifa_netmask == NULL) continue;
		if (it->ifa_addr->sa_family != AF_INET) continue;
		if (!(it->ifa_flags & IFF_UP) || (it->ifa_flags & IFF_LOOPBACK)) continue;
		struct in_addr addr = ((struct sockaddr_in *)(void *)it->ifa_addr)->sin_addr;
		struct in_addr mask = ((struct sockaddr_in *)(void *)it->ifa_netmask)->sin_addr;
		if (mask.s_addr == 0) continue;
		if ((addr.s_addr & mask.s_addr) == (target.s_addr & mask.s_addr)) {
			if (ifname != NULL && ifname_max > 0) snprintf(ifname, ifname_max, "%s", it->ifa_name);
			if (local_ip != NULL && local_ip_max > 0) inet_ntop(AF_INET, &addr, local_ip, (socklen_t)local_ip_max);
			found = 0;
			break;
		}
	}
	freeifaddrs(list);
	return found;
}

int32_t fujiios_recipe_parse_file(const char *path, char *out, size_t max) {
	if (path == NULL || out == NULL || max == 0) return FUJIIOS_ERR_INVALID_ARG;
	out[0] = '\0';
	struct FujiProfile fp;
	int rc = fp_parse_fp1(path, &fp);
	if (rc) {
		snprintf(out, max, "%s", fp_get_error());
		return FUJIIOS_ERR_RUNTIME;
	}
	char *text = NULL;
	size_t text_len = 0;
	FILE *f = open_memstream(&text, &text_len);
	if (f == NULL) return FUJIIOS_ERR_OUT_OF_MEM;
	fprintf(f, "  <Device>%s</Device>\n", fp.prop_group_device);
	fprintf(f, "  <DeviceVersion>%s</DeviceVersion>\n", fp.prop_group_version);
	fprintf(f, "  <ProfileVersion>%d</ProfileVersion>\n", fp.profile_version);
	int dump_rc = fp_dump_struct(f, &fp);
	fclose(f);
	snprintf(out, max, "%s", text ? text : "");
	free(text);
	if (dump_rc) {
		// Some values had no symbolic name; the lines that could be rendered are kept.
		return FUJIIOS_ERR_RUNTIME;
	}
	return 0;
}
