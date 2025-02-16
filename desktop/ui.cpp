#include <imgui.h>
#include <stdlib.h>
#include <pthread.h>
#include "backends/imgui_impl_glfw.h"

extern "C" {
	#include <fp.h>
	#include <camlib.h>
	#include "fudge_ui.h"
	struct State fuji_state = {0};

	int fudge_ui_backend(void (*renderer)());
}

static void option(const char *name, struct FujiLookup *tbl, uint32_t *index) {
	if (ImGui::BeginCombo(name, tbl[(*index)].key, 0)) {
		for (int i = 0; tbl[i].key != NULL; i++) {
			const bool is_selected = ((*index) == i);
			if (ImGui::Selectable(tbl[i].key, is_selected)) {
				(*index) = (uint32_t)i;
			}

			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

void fudge_rawconv_screen(struct State *state) {
	if (ImGui::BeginTabBar("Hello")) {
		if (ImGui::BeginTabItem("Raw Conversion")) {
			if (ImGui::BeginTable("split", 2)) {
				if (ImGui::TableNextColumn()) {
					option("Film Simulation", fp_film_sim, &state->fp.FilmSimulation);
					option("Exposure Bias", fp_exposure_bias, &state->fp.ExposureBias);
					option("Color", fp_range, &state->fp.Color);
					option("Sharpness", fp_range, &state->fp.Sharpness);
					option("Grain Effect", fp_range, &state->fp.GrainEffect);
					option("White Balance", fp_range, &state->fp.WhiteBalance);
				}

				if (ImGui::TableNextColumn()) {
					ImGui::Text("Something interesting will be here");
				}

				ImGui::EndTable();
			}

#if 0
			float avail_x = ImGui::GetContentRegionAvail().x;
			float avail_y = ImGui::GetContentRegionAvail().y;

			ImGui::BeginChild("LeftPanel", ImVec2(avail_x * 0.2f, avail_y), true);
			ImGui::Text("Left panel content");
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("MiddlePanel", ImVec2(avail_x * 0.6f, avail_y), true);
			ImGui::Text("Right panel content");
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("RightPanel", ImVec2(avail_x * 0.2f - (ImGui::GetStyle().ItemSpacing.x * 2), avail_y), true);
			ImGui::Text("Right panel content");
			ImGui::EndChild();
#endif
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Connections")) {
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

extern "C" void fudge_render_gui(void) {
	struct State *state = &fuji_state;
	pthread_mutex_lock(state->mutex);

	if (state->is_camera_connected) {
		ImGui::Button("Disconnect");
	} else {
		ImGui::Button("Connect");
		ImGui::BeginDisabled();
	}

	if (ImGui::BeginTable("table_share_lineheight", 1, ImGuiTableFlags_Borders)) {
		ImGui::TableNextColumn();
		ImGui::Text("Camera One");
		ImGui::Text("Line 2");
		ImGui::TableNextColumn();
		ImGui::Text("Line 1");
		ImGui::Text("Line 2");
		ImGui::EndTable();
	}

	ImGui::Begin("JPEG preview");


	ImGui::Text("TODO: Render");

	ImGui::End();


	if (ImGui::Button("Backup camera settings")) {

	}
	ImGui::Button("Backup camera settings");

	if (!state->is_camera_connected) {
		ImGui::EndDisabled();
	}

	pthread_mutex_unlock(state->mutex);
}

extern "C" int fudge_ui(void) {
	fuji_state.is_camera_connected = false;
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
