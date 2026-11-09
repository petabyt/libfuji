// Link/lifecycle smoke test for the C core (no camera needed).
#include <stdio.h>
#include <string.h>
#include "CFujiCore.h"

static void log_cb(void *user, int32_t level, const char *message) {
	(void)user;
	printf("[%d] %s\n", level, message);
}

int main(void) {
	fujiios_global_init();
	fujiios_set_global_log(log_cb, NULL);

	fujiios_session *s = fujiios_session_new(FUJIIOS_TRANSPORT_XAPP_WIRELESS_COMM, log_cb, NULL, FUJIIOS_LOG_VERBOSE);
	if (s == NULL) { printf("FAIL: session_new\n"); return 1; }
	if (fujiios_session_new(5, NULL, NULL, 0) != NULL) { printf("FAIL: invalid transport accepted\n"); return 1; }

	// Operations before connect must fail cleanly
	fujiios_object_info info;
	if (fujiios_get_object_info(s, 1, &info) != FUJIIOS_ERR_NOT_CONNECTED) { printf("FAIL: not-connected check\n"); return 1; }

	// Abort before connect, then connect must report ABORTED
	fujiios_abort(s);
	if (fujiios_connect(s, "127.0.0.1", 1, 0, NULL) != FUJIIOS_ERR_ABORTED) { printf("FAIL: abort before connect\n"); return 1; }
	fujiios_session_free(s);

	// Connect to a closed port fails with IO error and leaves the session usable for free()
	s = fujiios_session_new(FUJIIOS_TRANSPORT_WIRELESS_COMM, log_cb, NULL, FUJIIOS_LOG_INFO);
	int32_t rc = fujiios_connect(s, "127.0.0.1", 1, 0, NULL);
	if (rc != FUJIIOS_ERR_IO) { printf("FAIL: connect closed port rc=%d\n", rc); return 1; }
	fujiios_close(s, 1);
	fujiios_session_free(s);

	char ifname[32], local[64];
	if (fujiios_find_interface_for_ipv4("127.0.0.1", ifname, sizeof(ifname), local, sizeof(local)) == 0) {
		printf("loopback is skipped as expected? got %s\n", ifname);
	}

	printf("smoke OK\n");
	return 0;
}
