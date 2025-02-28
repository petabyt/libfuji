#include <stdlib.h>
#include <string.h>
#include <fp.h>
#include <camlib.h>
#include "im.h"

struct State {
	pthread_mutex_t *mutex;
	int is_camera_connected;

	struct PtpDeviceEntry *camlib_entries;

	struct FujiProfile fp;
};
struct State fuji_state = {0};

int fudge_ui_backend(void (*renderer)());

void fudge_render_gui(void) {
	struct State *state = &fuji_state;
	pthread_mutex_lock(state->mutex);

	im_button("Hello");

#if 0
	static int sel = 0;

	im_combo_box("Hello", &sel);
	im_add_combo_box_item("Item 1");
	im_end_combo_box();
#endif

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
