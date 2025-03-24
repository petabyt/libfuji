#include <stdlib.h>
#include <string.h>
#include <fp.h>
#include <camlib.h>
#include "im.h"

int fudge_ui_backend(void (*renderer)());

struct State {
	pthread_mutex_t *mutex;
	int is_camera_connected;

	struct PtpDeviceEntry *camlib_entries;

	int filmsim;
	int expobias;
	int color;
	int sharpness;
	int graineffect;
	int whitebalance;

	struct FujiProfile fp;
};
struct State fuji_state = {0};

static void option(const char *name, struct FujiLookup *tbl, int *option, uint32_t *data) {
	im_combo_box(name, tbl[*option].key);
	for (int i = 0; tbl[i].key != NULL; i++) {
		im_add_combo_box_item(tbl[i].key, option);
		(*data) = tbl[*option].value;
	}
	im_end_combo_box();
}

void fudge_render_gui(void) {
	struct State *state = &fuji_state;
	pthread_mutex_lock(state->mutex);

	im_button("Hello, this is a UI test currently. Nothing to see here.");

	option("Film Simulation", fp_film_sim, &state->filmsim, &state->fp.FilmSimulation);
	option("Exposure Bias", fp_exposure_bias, &state->expobias, &state->fp.ExposureBias);
	option("Color", fp_range, &state->color, &state->fp.Color);
	option("Sharpness", fp_range, &state->sharpness, &state->fp.Sharpness);
	option("Grain Effect", fp_range, &state->graineffect, &state->fp.GrainEffect);
	option("White Balance", fp_range, &state->whitebalance, &state->fp.WhiteBalance);

	pthread_mutex_unlock(state->mutex);
}

int fudge_ui(void) {
	fuji_state.is_camera_connected = 0;
	fuji_state.mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

	struct PtpDeviceEntry *fake_entry1 = (struct PtpDeviceEntry *)malloc(sizeof(struct PtpDeviceEntry));
	fake_entry1->endpoint_in = 0x81;
	fake_entry1->endpoint_int = 0x81;
	fake_entry1->endpoint_out = 0x1;
	strcpy(fake_entry1->manufacturer, "Fuji Photo Film");
	strcpy(fake_entry1->name, "X-T30 II");
	fake_entry1->next = NULL;

	fuji_state.camlib_entries = fake_entry1;

	pthread_mutex_init(fuji_state.mutex, NULL);
	fudge_ui_backend(fudge_render_gui);
	return 0;
}
