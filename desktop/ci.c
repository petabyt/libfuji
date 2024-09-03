#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <camlib.h>
#include <fuji.h>
#include <app.h>
#include <fujiptp.h>
#include <signal.h>
#include "desktop.h"

static int run_vcam(const char *model_name, const char *arg) {
	char shell[256];
	sprintf(shell, "vcam %s --ip 0.0.0.0 %s & echo $!", model_name, arg);
	FILE *p = popen(shell, "r");
	int pid;
	fscanf(p, "%d", &pid);
	pclose(p);
	printf("vcam pid: %d\n", pid);
	usleep(1000 * 100); // 100ms
	return pid;
}

static int discovery_test() {
	int pid = run_vcam("fuji_x_h1", "--discovery");
}

int fudge_test_all_cameras(struct PtpRuntime *r) {
	network_init();
	const char *models[] = {"fuji_x_h1", "fuji_x_a2", "fuji_x_t20", "fuji_x_t2", "fuji_x_s10", "fuji_x_f10", "fuji_x30"};
	const int len = sizeof(models) / sizeof(models[0]);

	for (int i = 0; i < len; i++) {
		int pid = run_vcam("fuji_x_h1", "");

		int rc = 0;
		char *ip_addr = "0.0.0.0";
		
		r->connection_type = PTP_IP_USB;
		if (ptpip_connect(r, ip_addr, FUJI_CMD_IP_PORT)) {
			printf("Error connecting to %s:%d\n", ip_addr, FUJI_CMD_IP_PORT);
			return 0;
		}
		fuji_reset_ptp(r);
		r->connection_type = PTP_IP_USB;
		fuji_get(r)->transport = FUJI_FEATURE_WIRELESS_COMM;
		
		rc = fuji_test_setup(r);
		if (rc) return rc;
		
		rc = fuji_test_filesystem(r);
		if (rc) return rc;
		
		rc = ptp_close_session(r);
		if (rc) return rc;
		
		ptpip_close(r);

		kill(pid, SIGTERM);
	}

	return 0;
}