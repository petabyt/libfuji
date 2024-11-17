#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <camlib.h>
#include <fuji.h>
#include <app.h>
#include <fujiptp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "desktop.h"

static FILE *shell_pipe = NULL;
static int shell_pid = 0;

static int run_vcam(const char *model_name, const char *arg) {
	char shell[256];
	sprintf(shell, "vcam %s --ip 0.0.0.0 %s & echo $!", model_name, arg);
	FILE *p = popen(shell, "r");
	int pid;
	fscanf(p, "%d", &pid);
	shell_pipe = p;
	printf("vcam pid: %d\n", pid);
	usleep(1000 * 100); // 100ms
	return pid;
}

static int discovery_test() {
	int pid = run_vcam("fuji_x_h1", "--discovery");
	return 0;
}

void nothing(int x) {}

pid_t child_pid = -1;

int fudge_test_all_cameras_(const char *name) {
	signal(SIGUSR1, nothing);
	char thispid[16];
	sprintf(thispid, "%d", getpid());
	const char *ip_addr = "0.0.0.0";

	child_pid = fork();
	if (child_pid == -1) {
		perror("fork failed");
		exit(1);
	}

	if (child_pid == 0) {
		int rc = execlp("/usr/bin/vcam", "vcam", name, "--ip", ip_addr, "--sig", thispid, NULL);
		printf("Return value: %d\n", rc);
		exit(0);
	}

	printf("Waiting for vcam...\n");
	pause();

	int rc = 0;

	struct PtpRuntime *r = ptp_new(PTP_IP_USB);
	fuji_reset_ptp(r);
	fuji_get(r)->transport = FUJI_FEATURE_WIRELESS_COMM;
	if (ptpip_connect(r, ip_addr, FUJI_CMD_IP_PORT, 0)) {
		printf("Error connecting to %s:%d\n", ip_addr, FUJI_CMD_IP_PORT);
		return -1;
	}
	
	rc = fuji_test_setup(r);
	if (rc) return rc;
	
	rc = fuji_test_filesystem(r);
	if (rc) return rc;
	
	rc = ptp_close_session(r);
	if (rc) return rc;
	
	ptpip_close(r);

	printf("Going to kill %d\n", child_pid);
	if (kill(child_pid, SIGINT) == -1) {
		perror("kill failed");
		return -1;
	}

	wait(NULL);
	printf("Child process terminated\n");
	return 0;
}

int fudge_test_all_cameras(void) {
	network_init();
	const char *models[] = {"fuji_x_h1", "fuji_x_a2", "fuji_x_t20", "fuji_x_t2", "fuji_x_s10", "fuji_x_f10", "fuji_x30", "fuji_x_dev"};
	const int len = sizeof(models) / sizeof(models[0]);

	for (int i = 0; i < len; i++) {
		int rc = fudge_test_all_cameras_(models[i]);
		if (rc) return rc;
	}

	return 0;
}
