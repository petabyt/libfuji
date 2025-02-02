#include <imgui.h>
#include <stdlib.h>

extern "C" int fudge_ui_backend(void *(*init_state)(), void (*renderer)(void *));

struct State {
	int state;
};

extern "C" void *fudge_init_state(void) {
	struct State *state = (struct State *)calloc(1, sizeof(struct State));
	return state;
}

extern "C" void fudge_render_gui(void *arg) {
	struct State *state = (struct State *)arg;
	if (ImGui::BeginTabBar("Hello")) {
		if (ImGui::BeginTabItem("Raw Conversion")) {
			if (ImGui::BeginTable("split", 1))
			{
				ImGui::TableNextColumn();
				ImGui::Text("Hello1");
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