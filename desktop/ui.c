#include "desktop.h"

#include <stdlib.h>
#include <string.h>
#include <fp.h>
#include <pthread.h>
#include <libpict.h>
#include <stdarg.h>
#include "im.h"

int fudge_ui_backend(void (*renderer)());

struct State {
	int window_open;
	pthread_mutex_t *mutex;
	int is_camera_connected;
	// TODO: Make this the main window title
	char connected_camera_name[32];

	struct PtpDeviceEntry *camlib_entries;

	char *ui_log_buffer;
	size_t ui_log_pos;
	size_t ui_log_length;

	int filmsim;
	int expobias;
	int color;
	int sharpness;
	int graineffect;
	int whitebalance;

	char backup_file_path[512];
	char raf_path[512];
	char output_jpg_path[512];
	char fp_xml_path[512];

	struct FujiProfile fp;
};
struct State fudge_state = {0};

static void ui_log(const char *line) {
	pthread_mutex_lock(fudge_state.mutex);
	size_t len = strlen(line);
	while ((fudge_state.ui_log_pos + len + 2) >= fudge_state.ui_log_length) {
		size_t first_newline = 0;
		while (fudge_state.ui_log_buffer[first_newline] != '\n') {
			first_newline++;
			if (first_newline > fudge_state.ui_log_pos) {
				abort();
			}
		}
		first_newline++;
		for (size_t i = first_newline; i < fudge_state.ui_log_pos; i++) {
			fudge_state.ui_log_buffer[i - first_newline] = fudge_state.ui_log_buffer[i];
		}
		fudge_state.ui_log_pos -= first_newline;
	}
	memcpy(fudge_state.ui_log_buffer + fudge_state.ui_log_pos, line, len);
	fudge_state.ui_log_pos += len;
	fudge_state.ui_log_buffer[fudge_state.ui_log_pos++] = '\n';
	fudge_state.ui_log_buffer[fudge_state.ui_log_pos] = '\0';
	pthread_mutex_unlock(fudge_state.mutex);
}

void app_print(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	puts(buffer);
	if (fudge_state.window_open) {
		ui_log(buffer);
	}
}

void ptp_error_log(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	size_t len = strlen(buffer);
	if (len > 1 && buffer[len - 1] == '\n') {
		buffer[len - 1] = '\0';
	}
	printf("PTP: %s\n", buffer);
	if (fudge_state.window_open) {
		ui_log(buffer);
	}
}

void plat_dbg(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	printf("DBG: %s\n", buffer);
	if (fudge_state.window_open) {
		ui_log(buffer);
	}
}

void app_log_clear(void) {
	if (fudge_state.window_open) {
		pthread_mutex_lock(fudge_state.mutex);
		fudge_state.ui_log_buffer[0] = '\0';
		fudge_state.ui_log_pos = 0;
		pthread_mutex_unlock(fudge_state.mutex);
	}
}

void app_downloading_file(const struct PtpObjectInfo *oi) {
	app_print("Downloading file");
}

void app_downloaded_file(const struct PtpObjectInfo *oi, const char *path) {
	app_print("Downloaded file");
}

void app_send_cam_name(const char *name) {
	app_print("Now connected to '%s'", name);
	if (fudge_state.window_open) {
		pthread_mutex_lock(fudge_state.mutex);
		strncpy(fudge_state.connected_camera_name, name, sizeof(fudge_state.connected_camera_name));
		pthread_mutex_unlock(fudge_state.mutex);
	}
}

static void option(const char *name, struct FujiLookup *tbl, int *option, uint32_t *data) {
	im_combo_box(name, tbl[*option].key);
	for (int i = 0; tbl[i].key != NULL; i++) {
		im_add_combo_box_item(tbl[i].key, option);
		(*data) = tbl[*option].value;
	}
	im_end_combo_box();
}

static void *thread_raw_conversion(void *arg) {
	struct State *state = (struct State *)arg;
	int rc = fudge_process_raf(0, state->raf_path, state->output_jpg_path, state->fp_xml_path);
	if (rc) {
		app_print("Failed to convert RAW");
	}
	return NULL;
}
static void *thread_backup(void *arg) {
	struct State *state = (struct State *)arg;
	int rc = fudge_download_backup(0, state->backup_file_path);
	if (rc) {
		app_print("Failed to backup");
	}
	return NULL;
}
static void *thread_restore(void *arg) {
	struct State *state = (struct State *)arg;
	int rc = fudge_restore_backup(0, state->backup_file_path);
	if (rc) {
		app_print("Failed to restore");
	}
	return NULL;
}

void fudge_render_gui(void) {
	static int x = 0;
	struct State *state = &fudge_state;
	pthread_mutex_lock(state->mutex);
#if 0
	if (state->is_camera_connected) {
		char fmt[64];
		snprintf(fmt, sizeof(fmt), "Connected to %s", fudge_state.connected_camera_name);
	} else {
		im_label("No camera connected.");
	}
#endif
	if (im_tab()) {
		if (im_add_tab_item("Backup/Restore")) {
			im_entry("Backup file path", state->backup_file_path, sizeof(state->backup_file_path), 0);
			if (im_button("Download backup to file")) {
				static pthread_t t;
				pthread_create(&t, NULL, thread_backup, state);
			}
			if (im_button("Restore backup from file")) {
				static pthread_t t;
				pthread_create(&t, NULL, thread_restore, state);
			}
			im_end_tab_item();
		}
		if (im_add_tab_item("Raw Conversion")) {
			im_entry("Input RAF path", state->raf_path, sizeof(state->raf_path), 0);
			im_entry("Output JPG path", state->output_jpg_path, sizeof(state->output_jpg_path), 0);
			im_entry("FP1/FP2/FP3 path", state->fp_xml_path, sizeof(state->fp_xml_path), 0);
			if (im_button("Connect to a camera and convert RAW")) {
				static pthread_t t;
				pthread_create(&t, NULL, thread_raw_conversion, state);
			}
			im_end_tab_item();
		}
		if (im_add_tab_item("About")) {
			im_label("Licenses:");
			im_label("libusb-1.0 (LGPL v2.1)");
			im_label("dear imgui (MIT)");
			im_label("hello_imgui (MIT)");
			im_label("Copyright (C) 2023 Fudge by Daniel C");
			im_label("Compile date: " __DATE__);
			im_end_tab_item();
		}
		im_end_tab();
	}

#if 0
	option("Film Simulation", fp_film_sim, &state->filmsim, &state->fp.FilmSimulation);
	option("Exposure Bias", fp_exposure_bias, &state->expobias, &state->fp.ExposureBias);
	option("Color", fp_range, &state->color, &state->fp.Color);
	option("Sharpness", fp_range, &state->sharpness, &state->fp.Sharpness);
	option("Grain Effect", fp_range, &state->graineffect, &state->fp.GrainEffect);
	option("White Balance", fp_range, &state->whitebalance, &state->fp.WhiteBalance);
#endif

	// if (im_button("Clear logs")) {
	// 	app_log_clear();
	// }
	im_multiline_entry(fudge_state.ui_log_buffer, fudge_state.ui_log_length, 0);

	pthread_mutex_unlock(state->mutex);
}

int fudge_ui(void) {
	fudge_state.ui_log_buffer = malloc(1024);
	fudge_state.ui_log_length = 1024;
	fudge_state.ui_log_pos = 0;
	fudge_state.is_camera_connected = 0;
	fudge_state.mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	fudge_state.window_open = 1;

	{ // Testing camera entries in UI
		struct PtpDeviceEntry *fake_entry1 = (struct PtpDeviceEntry *)malloc(sizeof(struct PtpDeviceEntry));
		fake_entry1->endpoint_in = 0x81;
		fake_entry1->endpoint_int = 0x81;
		fake_entry1->endpoint_out = 0x1;
		strcpy(fake_entry1->manufacturer, "Fuji Photo Film");
		strcpy(fake_entry1->name, "X-T30 II");
		fake_entry1->next = NULL;
		fudge_state.camlib_entries = fake_entry1;
	}

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(fudge_state.mutex, &attr);

	char pwd[256];
	if (getcwd(pwd, sizeof(pwd)) == NULL) abort();
	snprintf(fudge_state.raf_path, sizeof(fudge_state.raf_path), "%s/DSC0000.RAF", pwd);
	snprintf(fudge_state.output_jpg_path, sizeof(fudge_state.output_jpg_path), "%s/output.jpg", pwd);
	snprintf(fudge_state.fp_xml_path, sizeof(fudge_state.fp_xml_path), "%s/MyProfile.FP1", pwd);
	snprintf(fudge_state.backup_file_path, sizeof(fudge_state.backup_file_path), "%s/MY_CAM.DAT", pwd);

	app_print("You are trying out the very much unfinished Fudge desktop utility.");
	app_print("This is not finished yet and is intended to be used through CLI.");
	app_print("This frontend is not very well setup and is only a simple layer over CLI functionality.");
	fudge_ui_backend(fudge_render_gui);
	return 0;
}
