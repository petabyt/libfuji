#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <libpict.h>
#include <fuji.h>
#include <app.h>
#include <fp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

int fuji_test_discovery(struct PtpRuntime *r);

void nothing(int x) {}

pid_t child_pid = -1;

int fudge_usb_connect(struct PtpRuntime *r, int num) {
	static int attempts = 0;
	int rc = fujiusb_try_connect(r, num);
	if (rc) {
		const char *msg = "No Fujifilm device found";
		if (attempts) {
			printf("%s (%d)\n", msg, attempts);
		} else {
			printf("%s\n", msg);
		}
		attempts++;
		return rc;
	}
	attempts = 0;

	return 0;
}

int fudge_dump_usb(int devnum) {
	struct PtpRuntime *r = ptp_new(PTP_USB);
	int rc = fudge_usb_connect(r, devnum);
	if (rc) return rc;

	rc = fujiusb_setup(r);
	if (rc) return rc;

	printf("Camera mode: ");
	switch (fuji_get(r)->transport) {
	case FUJI_FEATURE_RAW_CONV:
		printf("USB RAW CONV./BACKUP RESTORE");
		break;
	case FUJI_FEATURE_USB_CARD_READER:
		printf("USB CARD READER");
		break;
	case FUJI_FEATURE_USB_TETHER_SHOOT:
		printf("USB TETHER SHOOTING");
		break;
	case FUJI_FEATURE_USB:
		printf("MTP");
		break;
	default:
		printf("Invalid %d", fuji_get(r)->transport);
		break;
	}
	printf("\n");

	struct PtpDeviceInfo oi;
	rc = ptp_get_device_info(r, &oi);
	if (rc) return rc;

	char buffer[4096];
	ptp_device_info_json(&oi, buffer, sizeof(buffer));
	printf("%s\n", buffer);

	for (int i = 0; i < oi.props_supported_length; i++) {
		printf("Get prop value %04x\n", oi.props_supported[i]);
		rc = ptp_get_prop_value(r, oi.props_supported[i]);
		if (rc == PTP_CHECK_CODE) continue;
		if (rc) return rc;
		printf("%04x = %x\n", oi.props_supported[i], ptp_parse_prop_value(r));
	}

	ptp_close_session(r);
	ptp_device_close(r);

	return rc;
}

int fudge_test_camera(const char *name) {
	signal(SIGUSR1, nothing);
	int pid = getpid();
	char thispid[16];
	sprintf(thispid, "%d", pid);
	const char *ip_addr = "0.0.0.0";

	child_pid = fork();
	if (child_pid == -1) {
		perror("fork failed");
		exit(1);
	}

	if (child_pid == 0) {
		int rc = execlp("vcam", "vcam", name, "tcp", "--ip", ip_addr, "--sig", thispid, NULL);
		printf("Return value: %d\n", rc);
		if (rc != 0x0) {
			kill(pid, SIGKILL);
		}
		exit(0);
	}

	printf("Waiting for vcam signal...\n");
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
	
	ptpip_device_close(r);

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
	const char *models[] = {"fuji_x_h1", "fuji_x_a2", "fuji_x_t20", "fuji_x_t2", "fuji_x_s10", "fuji_x_f10", "fuji_x30", "fuji_x_dev"};
	const int len = sizeof(models) / sizeof(models[0]);

	for (int i = 0; i < len; i++) {
		int rc = fudge_test_camera(models[i]);
		if (rc) return rc;
	}

	return 0;
}


static int help(void) {
	printf("TODO: help\n");
	return 0;
}

int main(int argc, char **argv) {
	int devnum = -1;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--test-wifi")) {
			int rc = fudge_test_all_cameras();
			plat_dbg("Result: %d\n", rc);
			return rc;
		}
		if (!strcmp(argv[i], "--dump-usb")) {
			return fudge_dump_usb(devnum);
		}
		if (!strcmp(argv[i], "--test-discovery")) {
			fuji_test_discovery(ptp_new(PTP_USB));
			return 1;
		}
		if (!strcmp(argv[i], "--help")) {
			return help();
		}
		printf("Invalid arg '%s'\n", argv[i]);
		return -1;
	}

	return 0;
}
