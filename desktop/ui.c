#include <stdlib.h>
#include <string.h>
#include <fp.h>
#include <camlib.h>
#include <stdarg.h>
#include "im.h"

int fudge_ui_backend(void (*renderer)());

struct State {
	int window_open;
	pthread_mutex_t *mutex;
	int is_camera_connected;
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
	printf("Now connected to '%s'\n", name);
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

void fudge_render_gui(void) {
	static int x = 0;
	struct State *state = &fudge_state;
	pthread_mutex_lock(state->mutex);

	if (state->is_camera_connected) {
		char fmt[64];
		snprintf(fmt, sizeof(fmt), "Connected to %s", fudge_state.connected_camera_name);
	} else {
		im_label("No camera connected.");
	}

	if (im_button("Hello, this is a UI test currently. Nothing to see here.")) {
		app_print("Testing %d", ++x);
	}

	option("Film Simulation", fp_film_sim, &state->filmsim, &state->fp.FilmSimulation);
	option("Exposure Bias", fp_exposure_bias, &state->expobias, &state->fp.ExposureBias);
	option("Color", fp_range, &state->color, &state->fp.Color);
	option("Sharpness", fp_range, &state->sharpness, &state->fp.Sharpness);
	option("Grain Effect", fp_range, &state->graineffect, &state->fp.GrainEffect);
	option("White Balance", fp_range, &state->whitebalance, &state->fp.WhiteBalance);

	im_multiline_entry(fudge_state.ui_log_buffer, fudge_state.ui_log_length);

	pthread_mutex_unlock(state->mutex);
}

int fudge_ui(void) {
	fudge_state.ui_log_buffer = malloc(200);
	fudge_state.ui_log_length = 200;
	fudge_state.ui_log_pos = 0;
	fudge_state.is_camera_connected = 0;
	fudge_state.mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	fudge_state.window_open = 1;
	struct PtpDeviceEntry *fake_entry1 = (struct PtpDeviceEntry *)malloc(sizeof(struct PtpDeviceEntry));
	fake_entry1->endpoint_in = 0x81;
	fake_entry1->endpoint_int = 0x81;
	fake_entry1->endpoint_out = 0x1;
	strcpy(fake_entry1->manufacturer, "Fuji Photo Film");
	strcpy(fake_entry1->name, "X-T30 II");
	fake_entry1->next = NULL;

	fudge_state.camlib_entries = fake_entry1;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(fudge_state.mutex, &attr);
	fudge_ui_backend(fudge_render_gui);
	return 0;
}
