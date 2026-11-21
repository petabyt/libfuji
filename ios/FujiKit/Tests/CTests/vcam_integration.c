// Integration test of the C core against petabyt/vcam (an emulated Fujifilm
// camera speaking Fuji PTP/IP). This exercises the real protocol code paths;
// it is NOT a substitute for testing on an X-T30 III.
//
// Usage: c-test <phase> <card-dir> [host] [port]
//   phase: main | setup-abort | download-abort | liveview
//   Each phase expects a freshly started vcam (see ios/scripts/vcam-integration.sh).
//   vcam must already be running, e.g.:
//   vcam fuji_x_s10 tcp --ip 127.0.0.1 --fs <card-dir>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include "CFujiCore.h"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); failures++; } else { printf("ok   "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int verbose = 0;
static void log_cb(void *user, int32_t level, const char *message) {
	(void)user;
	if (verbose || level >= FUJIIOS_LOG_ERROR) printf("    [c%d] %s\n", level, message);
}

static uint8_t *read_file(const char *dir, const char *name, size_t *len) {
	// vcam serves files from <dir> recursively; search one level of DCIM subfolders
	char path[1024];
	DIR *d = opendir(dir);
	if (d == NULL) return NULL;
	struct dirent *e;
	uint8_t *result = NULL;
	while ((e = readdir(d)) != NULL && result == NULL) {
		if (e->d_name[0] == '.') continue;
		snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
		if (strcasecmp(e->d_name, name) == 0) {
			FILE *f = fopen(path, "rb");
			if (f == NULL) continue;
			fseek(f, 0, SEEK_END);
			long n = ftell(f);
			fseek(f, 0, SEEK_SET);
			result = malloc((size_t)n);
			if (fread(result, 1, (size_t)n, f) != (size_t)n) { free(result); result = NULL; }
			fclose(f);
			*len = (size_t)n;
		} else {
			DIR *sub = opendir(path);
			if (sub != NULL) {
				closedir(sub);
				result = read_file(path, name, len);
			}
		}
	}
	closedir(d);
	return result;
}

struct sink {
	uint8_t *buf;
	uint64_t cap;
	uint64_t received;
	uint64_t first_offset;
	int chunks;
	int cancel_after_chunks;   // 0 = never
	int sleep_ms_per_chunk;
	int info_calls;
	int refuse_info;
	uint64_t last_end;
	int order_ok;
};

static int32_t on_info(void *user, const fujiios_object_info *info) {
	struct sink *s = user;
	s->info_calls++;
	if (s->buf == NULL) {
		s->cap = info->compressed_size;
		s->buf = calloc(1, (size_t)s->cap + 1);
	}
	return s->refuse_info;
}

static int32_t on_data(void *user, const uint8_t *data, uint32_t length, uint64_t offset, uint64_t total) {
	struct sink *s = user;
	if (s->chunks == 0) {
		s->first_offset = offset;
		s->last_end = offset;
		s->order_ok = 1;
	}
	if (offset != s->last_end) s->order_ok = 0;
	s->last_end = offset + length;
	if (offset + length <= s->cap) memcpy(s->buf + offset, data, length);
	s->received += length;
	s->chunks++;
	(void)total;
	if (s->sleep_ms_per_chunk) usleep((useconds_t)s->sleep_ms_per_chunk * 1000);
	return (s->cancel_after_chunks && s->chunks >= s->cancel_after_chunks) ? 1 : 0;
}

static int32_t test_transport(void) {
	// TRANSPORT=9 exercises libfuji's XApp (Bluetooth handoff) code path
	const char *t = getenv("TRANSPORT");
	return (t != NULL && atoi(t) == FUJIIOS_TRANSPORT_XAPP_WIRELESS_COMM) ? FUJIIOS_TRANSPORT_XAPP_WIRELESS_COMM : FUJIIOS_TRANSPORT_WIRELESS_COMM;
}

static fujiios_session *open_session(const char *host, int port) {
	fujiios_session *s = fujiios_session_new(test_transport(), log_cb, NULL, FUJIIOS_LOG_VERBOSE);
	int32_t rc = -999;
	for (int attempt = 0; attempt < 20; attempt++) {
		rc = fujiios_connect(s, host, port, 1, NULL);
		if (rc == 0) break;
		usleep(250 * 1000);
	}
	CHECK(rc == 0, "connect %s:%d rc=%s", host, port, fujiios_error_name(rc));
	if (rc) { fujiios_session_free(s); return NULL; }
	rc = fujiios_setup(s, "CamBridge-Test");
	CHECK(rc == 0, "fuji_setup rc=%s (resp 0x%X)", fujiios_error_name(rc), fujiios_last_response_code(s));
	if (rc) { fujiios_close(s, 0); fujiios_session_free(s); return NULL; }
	return s;
}

struct abort_arg { fujiios_session *s; int delay_ms; };
static void *abort_thread(void *p) {
	struct abort_arg *a = p;
	usleep((useconds_t)a->delay_ms * 1000);
	fujiios_abort(a->s);
	return NULL;
}

static double now_s(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
	if (argc < 3) { printf("usage: %s <phase> <card-dir> [host] [port]\n", argv[0]); return 2; }
	const char *phase = argv[1];
	const char *card = argv[2];
	const char *host = argc > 3 ? argv[3] : "127.0.0.1";
	int port = argc > 4 ? atoi(argv[4]) : 55740;
	verbose = getenv("VERBOSE") != NULL;
	setvbuf(stdout, NULL, _IONBF, 0);
	fujiios_global_init();
	fujiios_set_global_log(log_cb, NULL);

	fujiios_session *s = NULL;
	fujiios_state st;
	int32_t rc;
	uint32_t biggest = 0;

	if (strcmp(phase, "setup-abort") == 0) {
		// vcam does not resend the initial events on a second connection, so
		// fuji_setup() polls "wait for access" forever -- the same thing happens on
		// a real camera while the user has not pressed OK. Abort must end it.
		s = open_session(host, port); // first client: normal setup
		if (s == NULL) return 1;
		fujiios_close(s, 1);
		fujiios_session_free(s);
		usleep(300 * 1000);
		s = fujiios_session_new(FUJIIOS_TRANSPORT_WIRELESS_COMM, log_cb, NULL, FUJIIOS_LOG_ERROR);
		rc = fujiios_connect(s, host, port, 1, NULL);
		CHECK(rc == 0, "connect for setup-abort");
		struct abort_arg a = { s, 1500 };
		pthread_t t;
		pthread_create(&t, NULL, abort_thread, &a);
		double t0 = now_s();
		rc = fujiios_setup(s, "CamBridge-Test");
		double dt = now_s() - t0;
		pthread_join(t, NULL);
		CHECK(rc == FUJIIOS_ERR_ABORTED, "abort ends fuji_setup wait loop -> %s after %.2fs", fujiios_error_name(rc), dt);
		CHECK(dt < 6.0, "setup returned promptly after abort");
		fujiios_close(s, 1);
		fujiios_session_free(s);
		printf("%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
		return failures ? 1 : 0;
	}

	s = open_session(host, port);
	if (s == NULL) return 1;
	
	fujiios_get_state(s, &st);
	printf("     state: transport=%d camera_state=%d num_objects=%d remote_version=0x%X image_get=0x%X get_obj=0x%X remote_img_view=0x%X oldest_first=%d name='%s' storage='%s'\n",
		st.transport, st.camera_state, st.num_objects, st.remote_version, st.image_get_version, st.get_object_version,
		st.remote_image_view_version, st.sort_by_oldest_first, st.camera_name, st.storage_name);
	CHECK(st.num_objects > 0, "camera reports %d objects", st.num_objects);

	CHECK(fujiios_poll_events(s) == 0, "poll events");
	if (strcmp(phase, "main") != 0) goto other_phases;

	uint32_t handles[256];
	uint32_t count = 0;
	rc = fujiios_object_handles(s, handles, 256, &count);
	CHECK(rc == 0 && count == (uint32_t)st.num_objects, "object handles rc=%s count=%u", fujiios_error_name(rc), count);

	uint32_t biggest_size = 0;
	int jpegs = 0;
	char biggest_name[64] = "";
	for (uint32_t i = 0; i < count; i++) {
		fujiios_object_info info;
		memset(&info, 0, sizeof(info));
		rc = fujiios_get_object_info(s, handles[i], &info);
		// vcam also exposes its root/DCIM folders as handles; a real camera over
		// Wi-Fi lists only media. Failures here are reported, not fatal.
		printf("     object info %u -> rc=%s %s fmt=0x%X size=%u date=%s kw='%s'\n", handles[i], fujiios_error_name(rc), info.filename, info.object_format, info.compressed_size, info.date_created, info.keywords);
		if (rc == 0 && info.object_format == 0x3801) jpegs++;
		if (rc == 0 && info.compressed_size > biggest_size) {
			biggest = handles[i];
			biggest_size = info.compressed_size;
			snprintf(biggest_name, sizeof(biggest_name), "%s", info.filename);
		}
		fujiios_buffer thumb = {0};
		fujiios_object_info tinfo;
		int32_t valid = 0;
		rc = fujiios_get_thumbnail(s, handles[i], &thumb, &tinfo, &valid);
		CHECK(rc == 0, "thumbnail %u -> %u bytes (jpeg=%d) info_from_thumb=%d", handles[i], thumb.length,
			thumb.length > 2 && thumb.data[0] == 0xFF && thumb.data[1] == 0xD8, valid);
		fujiios_buffer_free(&thumb);
	}

	CHECK(jpegs >= 2, "found %d JPEG objects (vcam hides one file: obj_count = objects - 1)", jpegs);
	size_t ref_len = 0;
	uint8_t *ref = read_file(card, biggest_name, &ref_len);
	CHECK(ref != NULL && ref_len == biggest_size, "reference file %s on disk (%zu bytes)", biggest_name, ref_len);

	// 1. Full download with property audit
	{
		struct sink sk = {0};
		fujiios_download_callbacks cb = { .user = &sk, .on_info = on_info, .on_data = on_data };
		fujiios_download_report rep;
		double t0 = now_s();
		rc = fujiios_download(s, biggest, 0, 1, &cb, &rep);
		double dt = now_s() - t0;
		CHECK(rc == 0, "full download rc=%s chunks=%d bytes=%llu in %.2fs", fujiios_error_name(rc), sk.chunks, (unsigned long long)sk.received, dt);
		CHECK(sk.order_ok && sk.received == biggest_size && rep.bytes_received == biggest_size && rep.expected_size == biggest_size, "download sequential and complete (report %llu/%llu)", (unsigned long long)rep.bytes_received, (unsigned long long)rep.expected_size);
		CHECK(ref && memcmp(sk.buf, ref, biggest_size) == 0, "downloaded bytes identical to camera file");
		printf("     audit: D226 %d -> %d, D227 %d -> %d, cleanup_rc=%d\n", rep.compress_small_before, rep.compress_small_after,
			rep.correct_size_before, rep.correct_size_after, rep.cleanup_rc);
		CHECK(sk.chunks >= 2, "multi-chunk download exercised (%d chunks)", sk.chunks);
		free(sk.buf);
	}

	// 2. Cancel after the first chunk: the result must stay CANCELED (upstream bug fixed by patch 0003)
	{
		struct sink sk = { .cancel_after_chunks = 1 };
		fujiios_download_callbacks cb = { .user = &sk, .on_info = on_info, .on_data = on_data };
		fujiios_download_report rep;
		rc = fujiios_download(s, biggest, 0, 0, &cb, &rep);
		CHECK(rc == FUJIIOS_ERR_CANCELED, "cancel during download returns CANCELED (got %s), cleanup_rc=%d", fujiios_error_name(rc), rep.cleanup_rc);
		CHECK(fujiios_poll_events(s) == 0, "session still usable after cancel");
		free(sk.buf);
	}

	// 3. Refusing in on_info stops before any data
	{
		struct sink sk = { .refuse_info = 1 };
		fujiios_download_callbacks cb = { .user = &sk, .on_info = on_info, .on_data = on_data };
		rc = fujiios_download(s, biggest, 0, 0, &cb, NULL);
		CHECK(rc == FUJIIOS_ERR_CANCELED && sk.chunks == 0, "on_info refusal stops before data (rc=%s chunks=%d)", fujiios_error_name(rc), sk.chunks);
		free(sk.buf);
	}

	// 4. Offset read verification + resume from an offset
	{
		uint32_t off = biggest_size / 2;
		fujiios_buffer part = {0};
		rc = fujiios_read_partial(s, biggest, off, 4096, &part);
		CHECK(rc == 0 && part.length == 4096 && ref && memcmp(part.data, ref + off, 4096) == 0, "GetPartialObject at offset %u matches file", off);
		fujiios_buffer_free(&part);

		struct sink sk = {0};
		fujiios_download_callbacks cb = { .user = &sk, .on_info = on_info, .on_data = on_data };
		uint64_t start = 1048576 + 123;
		rc = fujiios_download(s, biggest, start, 0, &cb, NULL);
		CHECK(rc == 0 && sk.first_offset == start && sk.received == biggest_size - start, "resume from %llu rc=%s received=%llu", (unsigned long long)start, fujiios_error_name(rc), (unsigned long long)sk.received);
		CHECK(ref && memcmp(sk.buf + start, ref + start, biggest_size - start) == 0, "resumed bytes identical");
		free(sk.buf);

		rc = fujiios_download(s, biggest, (uint64_t)biggest_size + 1, 0, &cb, NULL);
		CHECK(rc == FUJIIOS_ERR_INVALID_ARG, "offset past end rejected (%s)", fujiios_error_name(rc));
	}

	// 5. Raw property / info queries (read-only)
	{
		fujiios_buffer b = {0};
		rc = fujiios_get_prop_raw(s, 0xDF24, &b);
		CHECK(rc == 0, "GetDevicePropValue 0xDF24 -> %u bytes", b.length);
		fujiios_buffer_free(&b);
		rc = fujiios_get_prop_desc_raw(s, 0xD212, &b);
		printf("     GetDevicePropDesc 0xD212 -> %s (%u bytes, resp 0x%X)\n", fujiios_error_name(rc), b.length, fujiios_last_response_code(s));
		fujiios_buffer_free(&b);
		rc = fujiios_device_info_raw(s, &b);
		printf("     GetDeviceInfo -> %s (%u bytes)\n", fujiios_error_name(rc), b.length);
		fujiios_buffer_free(&b);
		rc = fujiios_get_prop_raw(s, 0x1234, &b);
		CHECK(rc == FUJIIOS_ERR_CHECK_CODE || rc == 0, "unknown property handled without crash (%s, resp 0x%X)", fujiios_error_name(rc), fujiios_last_response_code(s));
		fujiios_buffer_free(&b);
		CHECK(fujiios_poll_events(s) == 0, "session usable after rejected request");
	}

	fujiios_close(s, 1);
	fujiios_session_free(s);
	free(ref);
	printf("%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
	return failures ? 1 : 0;

other_phases:;
	// Find the largest object for the remaining phases
	{
		uint32_t n = 0, hs[256];
		fujiios_object_handles(s, hs, 256, &n);
		uint32_t best = 0;
		for (uint32_t i = 0; i < n; i++) {
			fujiios_object_info info;
			if (fujiios_get_object_info(s, hs[i], &info) == 0 && info.object_format == 0x3801 && info.compressed_size > best) {
				best = info.compressed_size;
				biggest = hs[i];
			}
		}
	}

	if (strcmp(phase, "download-abort") == 0) {
		// Abort from another thread during a slow download returns promptly
		struct sink sk = { .sleep_ms_per_chunk = 300 };
		fujiios_download_callbacks cb = { .user = &sk, .on_info = on_info, .on_data = on_data };
		struct abort_arg a = { s, 100 };
		pthread_t t;
		pthread_create(&t, NULL, abort_thread, &a);
		double t0 = now_s();
		rc = fujiios_download(s, biggest, 0, 0, &cb, NULL);
		double dt = now_s() - t0;
		pthread_join(t, NULL);
		CHECK(rc == FUJIIOS_ERR_ABORTED, "abort during download -> %s after %.2fs", fujiios_error_name(rc), dt);
		CHECK(dt < 5.0, "abort unblocked the worker quickly");
		CHECK(!fujiios_is_connected(s), "session reports disconnected after abort");
		free(sk.buf);
	} else if (strcmp(phase, "liveview") == 0) {
		// fuji_setup() already ran one OpenCapture/Terminate cycle in remote mode.
		// vcam re-binds ports 55741/55742 on every OpenCapture and exits on the
		// second one, so only the frame read is asserted here.
		rc = fujiios_liveview_start(s);
		printf("     liveview start rc=%s\n", fujiios_error_name(rc));
		int frames = 0;
		for (int i = 0; i < 20 && rc == 0 && frames < 3; i++) {
			fujiios_buffer f = {0};
			rc = fujiios_liveview_frame(s, &f);
			if (rc == 0 && f.length > 2 && f.data[0] == 0xFF && f.data[1] == 0xD8) frames++;
			fujiios_buffer_free(&f);
		}
		CHECK(frames >= 1, "received %d JPEG liveview frames from the video socket", frames);
		rc = fujiios_liveview_stop(s);
		printf("     liveview stop rc=%s (IO_ERR expected with vcam, see comment)\n", fujiios_error_name(rc));
	} else {
		printf("unknown phase %s\n", phase);
		failures++;
	}

	fujiios_close(s, 0);
	fujiios_session_free(s);
	printf("%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
	return failures ? 1 : 0;
}
