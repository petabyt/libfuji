#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <discovery.h>
#include <fuji.h>

static int failures;
static int mutex_locks;
static int mutex_unlocks;
static int partial_object_calls;
static int object_info_result;
static char last_verbose_log[1024];

#define CHECK(condition) do { \
	if (!(condition)) { \
		fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
		failures++; \
	} \
} while (0)

void ptp_verbose_log(struct PtpRuntime *runtime, char *format, ...) {
	(void)runtime;
	va_list args;
	va_start(args, format);
	vsnprintf(last_verbose_log, sizeof(last_verbose_log), format, args);
	va_end(args);
}

void ptp_error_log(struct PtpRuntime *runtime, char *format, ...) {
	(void)runtime;
	(void)format;
}

__attribute__((noreturn)) void ptp_panic(char *format, ...) {
	(void)format;
	abort();
}

int fuji_discovery_check_cancel(struct PtpRuntime *runtime) {
	(void)runtime;
	return 0;
}

int app_check_thread_cancel(struct PtpRuntime *runtime) {
	(void)runtime;
	return 0;
}

void app_report_download_speed(struct PtpRuntime *runtime, long elapsed, size_t size) {
	(void)runtime;
	(void)elapsed;
	(void)size;
}

int plat_update_object_info(
	struct PtpRuntime *runtime,
	int handle,
	const struct PtpObjectInfo *object
) {
	(void)runtime;
	(void)handle;
	(void)object;
	return 0;
}

void __wrap_ptp_mutex_lock(struct PtpRuntime *runtime) {
	(void)runtime;
	mutex_locks++;
}

void __wrap_ptp_mutex_unlock(struct PtpRuntime *runtime) {
	(void)runtime;
	mutex_unlocks++;
}

int __wrap_ptp_get_object_info(
	struct PtpRuntime *runtime,
	uint32_t handle,
	struct PtpObjectInfo *object
) {
	(void)runtime;
	(void)handle;
	if (object_info_result != 0) return object_info_result;
	memset(object, 0, sizeof(*object));
	object->compressed_size = 4;
	snprintf(object->filename, sizeof(object->filename), "TEST.JPG");
	return 0;
}

int __wrap_ptp_get_partial_object(
	struct PtpRuntime *runtime,
	uint32_t handle,
	unsigned int offset,
	unsigned int max
) {
	(void)runtime;
	(void)handle;
	(void)offset;
	(void)max;
	partial_object_calls++;
	return PTP_RUNTIME_ERR;
}

static int decline_download(void *argument, struct PtpObjectInfo *object) {
	(void)argument;
	(void)object;
	return -77;
}

static int add_download_data(
	void *argument,
	void *data,
	unsigned int size,
	unsigned int offset,
	unsigned int total_size
) {
	(void)argument;
	(void)data;
	(void)size;
	(void)offset;
	(void)total_size;
	return 0;
}

static void test_discovery_datagram(void) {
	struct PtpRuntime runtime = {0};
	struct DiscoverInfo info = {0};
	snprintf(info.client_name, sizeof(info.client_name), "Fudge");
	char wildcard[] =
		"DISCOVER * HTTP/1.1\r\n"
		"DSCADDR:192.168.0.129\r\n";
	CHECK(fuji_discovery_parse_datagram(&runtime, wildcard, &info) == 0);
	CHECK(strcmp(info.client_name, "Fudge") == 0);
	CHECK(strcmp(info.camera_ip, "192.168.0.129") == 0);

	char explicit_name[] =
		"DISCOVER Workstation HTTP/1.1\r\n"
		"DSCADDR:127.0.0.1\r\n";
	CHECK(fuji_discovery_parse_datagram(&runtime, explicit_name, &info) == 0);
	CHECK(strcmp(info.client_name, "Workstation") == 0);
	CHECK(strcmp(info.camera_ip, "127.0.0.1") == 0);

	char long_name[100];
	memset(long_name, 'N', sizeof(long_name));
	long_name[sizeof(long_name) - 1] = '\0';
	char long_greeting[512];
	snprintf(
		long_greeting,
		sizeof(long_greeting),
		"DISCOVER %s HTTP/1.1\r\nDSCADDR:%s\r\n",
		long_name,
		long_name
	);
	CHECK(fuji_discovery_parse_datagram(&runtime, long_greeting, &info) == 0);
	CHECK(info.client_name[sizeof(info.client_name) - 1] == '\0');
	CHECK(info.camera_ip[sizeof(info.camera_ip) - 1] == '\0');
}

static void test_discovery_ack(void) {
	int sockets[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) {
		CHECK(false);
		return;
	}

	char ack[512];
	memset(ack, 'A', sizeof(ack));
	CHECK(write(sockets[0], ack, sizeof(ack)) == (ssize_t)sizeof(ack));
	struct PtpRuntime runtime = {0};
	CHECK(fuji_discovery_read_ack(&runtime, sockets[1]) == 0);
	CHECK(last_verbose_log[0] == 'A');
	CHECK(close(sockets[0]) == 0);
	CHECK(close(sockets[1]) == 0);
}

static void test_download_info_can_decline(void) {
	struct PtpUserPriv private = {0};
	private.transport = FUJI_FEATURE_AUTOSAVE;
	struct PtpRuntime runtime = {0};
	runtime.priv = &private;

	mutex_locks = 0;
	mutex_unlocks = 0;
	partial_object_calls = 0;
	object_info_result = 0;
	int result = fuji_download_file_ex(
		&runtime,
		1,
		decline_download,
		add_download_data,
		NULL
	);
	CHECK(result == -77);
	CHECK(partial_object_calls == 0);
	CHECK(mutex_locks == 1);
	CHECK(mutex_unlocks == 1);
}

static void test_object_info_error_unlocks(void) {
	struct PtpUserPriv private = {0};
	private.transport = FUJI_FEATURE_AUTOSAVE;
	struct PtpRuntime runtime = {0};
	runtime.priv = &private;

	mutex_locks = 0;
	mutex_unlocks = 0;
	partial_object_calls = 0;
	object_info_result = -66;
	int result = fuji_download_file_ex(
		&runtime,
		1,
		decline_download,
		add_download_data,
		NULL
	);
	CHECK(result == -66);
	CHECK(partial_object_calls == 0);
	CHECK(mutex_locks == 1);
	CHECK(mutex_unlocks == 1);
}

int main(void) {
	test_discovery_datagram();
	test_discovery_ack();
	test_download_info_can_decline();
	test_object_info_error_unlocks();
	if (failures != 0) {
		fprintf(stderr, "%d assertion(s) failed\n", failures);
		return 1;
	}
	puts("AutoSave regression tests passed");
	return 0;
}
