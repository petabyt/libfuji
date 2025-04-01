#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <camlib.h>
#include <fuji.h>
#include <app.h>
#include <fujiptp.h>
#include <lua.h>
#include <fuji_lua.h>
#include <pthread.h>
#include <fp.h>
#include "desktop.h"

// todo: move _thread routines to frontend

static struct PtpRuntime *ptp = NULL;

// TODO: System to manage a list of cameras connected
struct PtpRuntime *ptp_get(void) {
	return ptp;
}

struct PtpRuntime *luaptp_get_runtime(lua_State *L) {
	return ptp_get();
}

int cam_lua_setup(lua_State *L) {
	return 0;
}

int fudge_usb_connect(struct PtpRuntime *r, int num) {
	static int attempts = 0;
	app_log_clear();
	int rc = fujiusb_try_connect(r, num);
	if (rc) {
		const char *msg = "No Fujifilm device found";
		if (attempts) {
			app_print("%s (%d)", msg, attempts);
		} else {
			app_print("%s", msg);
		}
		attempts++;
		return rc;
	}
	attempts = 0;

	ptp = r;

	return 0;
}

void fudge_disconnect_all(void) {
	struct PtpRuntime *r = ptp_get();
	if (r == NULL) {
		// 0 cameras connected
		return;
	}
	printf("Disconnecting\n");
	ptp_report_error(r, "Intentional", 0);
	ptp = NULL;
}

int fudge_main_app_thread(struct PtpRuntime *r) {
	struct PtpDeviceInfo di;
	int rc = ptp_get_device_info(r, &di);
	if (rc) return rc;

	app_print("Connected to %s %s", di.manufacturer, di.model);
	app_send_cam_name(di.model);

	struct PtpArray *arr;
	rc = ptp_fuji_get_object_handles(r, &arr);
	if (rc) return rc;
	app_print("%d files on card", arr->length);

	while (r->io_kill_switch == 0) {
		if (ptpusb_get_status(r)) {
			app_print("USB cable disconnected.");
			break;
		}
		usleep(100000);
	}

	return 0;
}

//void *fudge_backup_settings(void *arg) {
//	return NULL;
//}

void *fudge_usb_connect_thread(void *arg) {
	struct PtpRuntime *r = ptp_new(PTP_USB);
	int rc = fudge_usb_connect(r, -1);
	if (rc) goto exit;

	app_update_connected_status(1);
	rc = fudge_main_app_thread(r);
	app_update_connected_status(0);

	ptp = NULL;

	exit:;
	ptp_close(r);
	pthread_exit(NULL);
	return 0;
}

int fudge_run_lua(struct PtpRuntime *r, const char *text) {
	cam_run_lua_script_async(text);
	return 0;
}

static char *read_file(const char *filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) return NULL;
    
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

int fuji_connect_run_script(int devnum, const char *filename) {
	const char *text = read_file(filename);
	if (text == NULL) {
		printf("%s not found\n", filename);
		return -1;
	}

	struct PtpRuntime *r = ptp_new(PTP_USB);	
	int rc = fudge_usb_connect(r, devnum);
	if (rc) return rc;

	app_print("Connected to a Fuji camera, running %s", filename);

	rc = cam_run_lua_script_async(text);
	if (rc) {
		printf("%s\n", cam_lua_get_error());
		printf("Lua err %d\n", rc);
	}
	return rc;
	// leak r
}

int fudge_cli_backup(int devnum, const char *filename) {
	struct PtpRuntime *r = ptp_new(PTP_USB);
	int rc = fudge_usb_connect(r, devnum);
	if (rc) return rc;

	rc = fujiusb_setup(r);
	if (rc) {
		ptp_device_close(r);
		return rc;
	}

	FILE *f = fopen(filename, "wb");
	rc = fujiusb_download_backup(r, f);
	fclose(f);

	ptp_close_session(r);
	ptp_device_close(r);

	return rc;
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

int fudge_process_raf(int devnum, const char *input, const char *output, const char *profile) {
	struct PtpRuntime *r = ptp_new(PTP_USB);
	int rc = fudge_usb_connect(r, devnum);
	if (rc) return rc;

	rc = fujiusb_setup(r);
	if (rc) return rc;

	rc = fuji_process_raf(r, input, output, profile);

	ptp_close_session(r);
	ptp_device_close(r);

	return rc;
}

int fudge_download_backup(int devnum, const char *output) {
	FILE *f = fopen(output, "wb");
	if (f == NULL) {
		printf("Failed to open %s for writing\n", output);
		return -1;
	}

	struct PtpRuntime *r = ptp_new(PTP_USB);
	int rc = fudge_usb_connect(r, devnum);
	if (rc) return rc;

	rc = fujiusb_setup(r);
	if (rc) return rc;

	rc = fujiusb_download_backup(r, f);
	if (rc) {
		app_print("Failed to download backup: %d", rc);
		goto exit;
	}

	app_print("Backup file saved to %s", output);

	ptp_close_session(r);
	exit:;
	ptp_device_close(r);
	ptp_close(r);

	fclose(f);
	return rc;
}

int fudge_restore_backup(int devnum, const char *output) {
	FILE *f = fopen(output, "rb");
	if (f == NULL) {
		printf("Failed to open %s for reading\n", output);
		return -1;
	}

	struct PtpRuntime *r = ptp_new(PTP_USB);
	int rc = fudge_usb_connect(r, devnum);
	if (rc) return rc;

	rc = fujiusb_setup(r);
	if (rc) return rc;

	rc = fujiusb_restore_backup(r, f);
	if (rc == PTP_CHECK_CODE) {
		printf("Failed to restore backup - camera rejected it (%04x)\n", ptp_get_return_code(r));
	} else if (rc) {
		printf("IO error while restoring backup\n");
		goto exit;
	}

	app_print("Successfully restored from backup file.");

	ptp_close_session(r);
	exit:;
	ptp_device_close(r);
	ptp_close(r);

	fclose(f);
	return rc;
}
