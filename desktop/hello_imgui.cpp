#include <hello_imgui/hello_imgui.h>

typedef void win0_renderer_t(void *);

static win0_renderer_t *win0_renderer;
static void *win0_state;

void render_ctx(void) {
	win0_renderer(win0_state);
}

extern "C" int fudge_ui_backend(void *(*init_state)(), void (*renderer)(void *)) {
	win0_renderer = renderer;
	win0_state = init_state();
	HelloImGui::Run(
		render_ctx,
		"Fudge desktop",
		false
	);
	return 0;
}