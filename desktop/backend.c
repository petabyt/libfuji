#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <camlib.h>
#include <fuji.h>
#include <app.h>
#include <fujiptp.h>
#include <lua.h>
#include <camlua.h>
#include <pthread.h>
#include "desktop.h"

static struct PtpRuntime *ptp = NULL;

// TODO: System to manage a list of cameras connected
struct PtpRuntime *ptp_get() {
	return ptp;
}

struct PtpRuntime *luaptp_get_runtime(lua_State *L) {
	return ptp_get();
}

int luaopen_libuilua(lua_State *L);
int cam_lua_setup(lua_State *L) {
	luaL_requiref(L, "ui", luaopen_libuilua, 1);
	return 0;
}

int fudge_usb_connect(struct PtpRuntime *r) {
	static int attempts = 0;
	app_log_clear();
	int rc = fujiusb_try_connect(r);
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

	rc = ptp_open_session(r);
	if (rc != PTP_CHECK_CODE && rc) return rc;

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

void *fudge_backup_settings(void *arg) {
	return NULL;
}

void *fudge_usb_connect_thread(void *arg) {
	struct PtpRuntime *r = ptp_new(PTP_USB);
	int rc = fudge_usb_connect(r);
	if (rc) goto exit;

	app_update_connected_status(1);
	rc = fudge_main_app_thread(r);
	app_update_connected_status(0);

	ptp = NULL;

	exit:;
	ptp_close(r);
	pthread_exit(NULL);
	return NULL;
}

int fudge_run_lua(struct PtpRuntime *r, const char *text) {
	cam_run_lua_script_async(text);
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

int fuji_connect_run_script(const char *filename) {
	const char *text = read_file(filename);
	if (text == NULL) {
		printf("%s not found\n", filename);
		return -1;
	}

	struct PtpRuntime *r = ptp_new(PTP_USB);	
	int rc = fudge_usb_connect(r);
	if (rc) return rc;

	rc = cam_run_lua_script_async(text);
	if (rc) {
		printf("%s\n", cam_lua_get_error());
		printf("Lua err %d\n", rc);
	}
	return rc;
	// leak r
}
