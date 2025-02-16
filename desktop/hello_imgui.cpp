#include <hello_imgui/hello_imgui.h>

typedef void win0_renderer_t(void);

static win0_renderer_t *win0_renderer;

void render_ctx(void) {
	win0_renderer();
}

extern "C" int fudge_ui_backend(void (*renderer)()) {
	win0_renderer = renderer;
	HelloImGui::Run(
		render_ctx,
		"Fudge desktop",
		false
	);
	return 0;
}
