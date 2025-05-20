#include <imgui.h>
#include "im.h"

static __thread struct PrivCache {
	int *combo_selected_ptr;
	int combo_index;
	int *tab_bar_selected_ptr;
	int tab_index;
}priv = {0};

void im_begin_disabled(void) {
	ImGui::BeginDisabled();
}
void im_end_disabled(void) {
	ImGui::EndDisabled();
}

int im_begin_tab_bar(int *selected) {
	priv.tab_bar_selected_ptr = selected;
	priv.tab_index = 0;
	return (int)ImGui::BeginTabBar("##tab", ImGuiTabBarFlags_AutoSelectNewTabs);
	// TODO: No way to set current selected tab
}
int im_begin_tab(const char *title) {
	ImGuiTabItemFlags flag = ImGuiTabItemFlags_None;
	bool rc = ImGui::BeginTabItem(title, nullptr, flag);
	if (rc) {
		(*priv.tab_bar_selected_ptr) = priv.tab_index;
	}
	priv.tab_index++;
	return (int)rc;
}
void im_end_tab() {
	ImGui::EndTabItem();
}
void im_end_tab_bar() {
	ImGui::EndTabBar();
}

void im_entry(const char *label, char *buffer, unsigned int size) {
	ImGui::InputText(label, buffer, size);
}

void im_multiline_entry(char *buffer, unsigned int size) {
	ImGui::InputTextMultiline("##source", buffer, size, ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);
}

int im_begin_combo_box(const char *label, int *selected) {
	priv.combo_selected_ptr = selected;
	priv.combo_index = 0;
	return (int)ImGui::BeginCombo(label, "", 0);
}

int im_begin_combo_box_ex(const char *label, int *selected, const char *preview_text) {
	priv.combo_selected_ptr = selected;
	priv.combo_index = 0;
	return (int)ImGui::BeginCombo(label, preview_text, 0);
}

void im_combo_box_item(const char *label) {
	int *selected = priv.combo_selected_ptr;
	const bool is_selected = ((*selected) == priv.combo_index);
	if (ImGui::Selectable(label, is_selected)) {
		(*selected) = priv.combo_index;
	}

	priv.combo_index++;

	if (is_selected) {
		ImGui::SetItemDefaultFocus();
	}
}
void im_end_combo_box(void) {
	if (priv.combo_index == -1) return;
	ImGui::EndCombo();
}

int im_button(const char *label) {
	return ImGui::Button(label);
}
int im_button_ex(const char *label, int width_dp) {
	return ImGui::Button(label);
}
int im_label(const char *label) {
	ImGui::Text("%s", label);
	return 0;
}
int im_begin_window(const char *name, int width_dp, int height_dp) {
	return ImGui::Begin(name);
}
void im_end_window(void) {
	ImGui::End();
}
