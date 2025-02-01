#include <hello_imgui/hello_imgui.h>

struct State {
	int x;
};

void render_gui(struct State *state) {
	if (ImGui::BeginTabBar("Hello")) {
		if (ImGui::BeginTabItem("Raw Conversion")) {
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

			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Connections")) {
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void render_ctx(void) {
	static struct State state = {0};
	render_gui(&state);
}

extern "C" int fudge_ui_backend(void) {
	HelloImGui::Run(
		render_ctx,
		"Fudge desktop",
		false
	);
	return 0;
}