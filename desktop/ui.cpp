#include <imgui.h>
#include <stdlib.h>

extern "C" {
	#include <fp.h>

	struct State {
		int state;
		int selected_film_sim;
	};

	int fudge_ui_backend(void *(*init_state)(), void (*renderer)(void *));
}

extern "C" void *fudge_init_state(void) {
	struct State *state = (struct State *)calloc(1, sizeof(struct State));
	state->selected_film_sim = 0;
	return state;
}

static void option(const char *name, struct FujiLookup *tbl, int *index) {
	if (ImGui::BeginCombo(name, tbl[(*index)].key, 0)) {
		for (int i = 0; tbl[i].key != NULL; i++) {
			const bool is_selected = ((*index) == i);
			if (ImGui::Selectable(tbl[i].key, is_selected)) {
				(*index) = i;
			}

			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

extern "C" void fudge_render_gui(void *arg) {
	struct State *state = (struct State *)arg;
	if (ImGui::BeginTabBar("Hello")) {
		if (ImGui::BeginTabItem("Raw Conversion")) {
			if (ImGui::BeginTable("split", 2))
			{
				if (ImGui::TableNextColumn()) {
					option("Film Simulation", fp_film_sim, &state->selected_film_sim);
					option("Exposure Bias", fp_exposure_bias, &state->selected_film_sim);
					option("Color", fp_range, &state->selected_film_sim);
					option("Sharpness", fp_range, &state->selected_film_sim);
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

extern "C" int fudge_ui(void) {
	fudge_ui_backend(fudge_init_state, fudge_render_gui);
	return 0;
}