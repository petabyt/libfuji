#include <imgui.h>

extern "C" {
#include "im.h"
}

static __thread struct PrivCache {
	// retarded
	char combo_label[32];
	char combo_items[100][16];
	int combo_length;
	int *combo_selected_ptr;
}priv;

void im_begin_disabled(void) {
	ImGui::BeginDisabled();
}
void im_end_disabled(void) {
	ImGui::EndDisabled();
}

int im_tab() {
	ImGui::BeginTabBar("...");
}
int im_add_tab_item(const char *title) {
	ImGui::BeginTabItem(title);
}
int im_end_tab_item(const char *title) {
	ImGui::BeginTabItem(title);
}
int im_end_tab() {
	ImGui::EndTabBar();
}

int im_combo_box(const char *label, int *selected) {
	strcpy(priv.combo_label, label);
	priv.combo_selected_ptr = selected;
	priv.combo_length = 0;
}
int im_add_combo_box_item(const char *label) {
	strncpy(priv.combo_items[priv.combo_length], label, 16);
	priv.combo_length++;
}
int im_end_combo_box() {
	if (ImGui::BeginCombo(priv.combo_label, priv.combo_items[*priv.combo_selected_ptr], 0)) {
		for (int i = 0; i < priv.combo_length; i++) {
			const bool is_selected = (*priv.combo_selected_ptr) == i;
			if (ImGui::Selectable(priv.combo_items[i], is_selected)) {
				(*priv.combo_selected_ptr) = i;
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}

int im_button(const char *label) {
	return ImGui::Button(label);
}
int im_label(const char *label) {
	ImGui::Text(label);
	return 0;
}
int im_window(const char *name, int width, int height, int flags) {
	ImGui::Begin(name);
}
int im_end_window(void) {
	ImGui::End();
}