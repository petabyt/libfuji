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

void nothing(int x) {}

pid_t child_pid = -1;

__attribute__((weak))
void ptp_verbose_log(struct PtpRuntime *r, const char *fmt, ...) {
	printf("PTP: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void ptp_error_log(struct PtpRuntime *r, const char *fmt, ...) {
	printf("ERR: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void ptp_panic(const char *fmt, ...) {
	printf("PTP abort: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	fflush(stdout);
	putchar('\n');
	abort();
}

void plat_dbg(char *fmt, ...) {
	printf("DBG: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
}

void app_print(struct PtpRuntime *r, char *fmt, ...) {
	printf("APP: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
}

int fuji_discovery_check_cancel(struct PtpRuntime *r) {return 0;}
void app_report_download_speed(struct PtpRuntime *r, long time, size_t size) {}
int app_check_thread_cancel(struct PtpRuntime *r) {return 0;}
int plat_update_object_info(struct PtpRuntime *r, int handle, const struct PtpObjectInfo *oi) {return 0;}
int app_ptp_download_file(struct PtpRuntime *r, struct PtpObjectInfo *oi, int object_id, unsigned int max_chunk_size, int index) {
	// TODO: 
	return 0;
}
void app_update_storage_info(struct PtpRuntime *r) {}
int app_queue_file_for_download(struct PtpRuntime *r, int object_id) { return -1; }
void app_send_cam_name(struct PtpRuntime *r, const char *name) {
	printf("Got camera name '%s'\n", name);
}

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

int temp_file_handle(void *arg, void *buffer, unsigned int size, unsigned int offset, unsigned int total_size) {
	struct PtpRuntime *r = arg;
	ptp_verbose_log(r, "Read %d bytes\n", size);
	return 0;
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
	if (!strcmp(name, "fuji_x_t5")) {
		r->priv->transport = FUJI_FEATURE_XAPP_WIRELESS_COMM;
	} else {
		r->priv->transport = FUJI_FEATURE_WIRELESS_COMM;
	}
	strlcpy(r->priv->ip_address, ip_addr, sizeof(r->priv->ip_address));
	if (ptpip_connect(r, ip_addr, FUJI_CMD_IP_PORT, 0)) {
		printf("Error connecting to %s:%d\n", ip_addr, FUJI_CMD_IP_PORT);
		return -1;
	}

	rc = fuji_setup(r, "tester");
	if (rc) return rc;

#if 0
	// not needed, fuji_setup enters gallery itself
	rc = fuji_config_image_gallery(r);
	if (rc) {
		ptp_verbose_log(r, "Failed to config image viewer\n");
		return rc;
	} else {
		ptp_error_log(r, "Configured image viewer\n");
	}
#endif

	if (r->priv->num_objects != 0) {
		srand(382);
		for (int i = 0; i < 10; i++) {
			if ((rand() & 3) == 0) {
				int handle = fuji_to_oh(r, (rand() % r->priv->num_objects + 1) + 1);
				ptp_verbose_log(r, "Downloading thumb (%d)\n", handle);
				rc = ptp_get_thumbnail(r, handle);
				if (rc == PTP_CHECK_CODE) {
					// doh
				} else if (rc) {
					ptp_error_log(r, "Failed to download thumb\n");
					return rc;
				}
			}

			if ((rand() & 7) == 0) {
				int handle = fuji_to_oh(r, (rand() % fuji_get(r)->num_objects + 1) + 1);
				ptp_verbose_log(r, "Downloading an object (%d)\n", handle);

				rc = fuji_download_file(r, handle, temp_file_handle, NULL);
				if (rc) {
					ptp_error_log(r, "fuji_download_file\n");
					return rc;
				}
			}
		}
	} else {
		ptp_verbose_log(r, "No files available, not running gallery simulation.");
	}
	
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
	const char *models[] = {"fuji_x_h1", "fuji_x_a2", "fuji_x_t20", "fuji_x_t2", "fuji_x_s10", "fuji_x_f10", "fuji_x30", "fuji_x_t5", "fuji_x_t4"};
	const int len = sizeof(models) / sizeof(models[0]);

	for (int i = 0; i < len; i++) {
		int rc = fudge_test_camera(models[i]);
		if (rc) return rc;
	}

	return 0;
}

int fuji_test_discovery(struct PtpRuntime *r) {
	struct DiscoverInfo info = {0};

	printf("Listening...\n");
	int rc = fuji_discover_thread(r, &info, "libfuji");
	if (rc == FUJI_D_REGISTERED) {
		plat_dbg("Registered %s %s", info.camera_name, info.camera_model);
	} else if (rc == FUJI_D_GO_PTP) {
		usleep(100000); // TODO: Kind of have to wait before connecting

		printf("connecting to %s:%d\n", info.camera_ip, info.camera_port);
		fuji_reset_ptp(r);
		r->connection_type = PTP_IP_USB;
		fuji_get(r)->transport = info.transport;
		strcpy(fuji_get(r)->ip_address, info.camera_ip);
		if (ptpip_connect(r, info.camera_ip, info.camera_port, 0)) {
			printf("Error connecting to %s:%d\n", info.camera_ip, info.camera_port);
			return 0;
		}

		rc = fuji_setup(r, "fudge");
		if (rc) return rc;

		printf("Connected, ready to do stuff\n");

		ptpip_device_close(r);
	} else {
		printf("Response code: %d\n", rc);
	}

	return -1;
}

static int help(void) {
	printf("--test-wifi\n");
	printf("--dump-usb\n");
	printf("--test-discovery\n");
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

	help();

	return 0;
}
