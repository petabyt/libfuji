// Dear-ImGUI wrapper
// Last edited 2025/5/1
#ifndef IM_H
#define IM_H

#ifdef __cplusplus
extern "C" {
#endif

/* API Design Rules
Widget function naming:
- im_begin_<widget> / im_<widget>
  - Create/start a widget with minimal arguments
- im_begin_<widget>_ex / im_<widget>_ex
  - Create/start a widget with complete arguments
  - This API is less stable
- im_end_<widget>
  - Ends a widget

- Widgets that accept children must use `im_begin_` naming
- Must only accept units of length/width/size in dp, not pixels.

*/

enum ImReturnCode {
	IM_NONE = 0,
	IM_CLICKED = 1,
	IM_SELECTED = 2,
	IM_CHILDREN_VISIBLE = 3,
};

struct ImModifier {
	unsigned int color;
	unsigned int width_x_type;
	unsigned int width_x;
	unsigned int width_y_type;
	unsigned int width_y;
	unsigned int margin[4];
	unsigned int padding[4];
};

/// @returns IM_NONE, IM_CHILDREN_VISIBLE
int im_begin_vertical_box(void);
void im_end_vertical_box(void);

/// @returns IM_NONE, IM_CHILDREN_VISIBLE
int im_begin_horizontal_box(void);
void im_end_horizontal_box(void);

void im_begin_disabled(void);
void im_end_disabled(void);

/// @brief Set the on-hover tooltip for the next widget
void im_set_next_tooltip(const char *label);
/// @brief Disable the next widget
void im_set_next_disabled(int opt);
/// @brief Force the next widget to expand and take up 100% of parents space
void im_set_next_expand();

int im_begin_tab_bar(int *selected);
int im_begin_tab(const char *title);
void im_end_tab(void);
void im_end_tab_bar(void);

/// @brief Begin a menu bar, this should only ever be called directly after im_begin_window_*
int im_begin_menu_bar(void);
/// @brief Begin a menu item within a menu
int im_begin_menu(const char *name);
// @brief A clickable menu item
int im_menu_item(const char *name);
void im_end_menu(void);
void im_end_menu_bar(void);

/// @returns IM_NONE, IM_CHILDREN_VISIBLE
int im_begin_combo_box(const char *label, int *selected);
/// @returns IM_NONE, IM_CHILDREN_VISIBLE
int im_begin_combo_box_ex(const char *label, int *selected, const char *preview_text);
void im_combo_box_item(const char *label);
void im_end_combo_box(void);

int im_checkbox_label(const char *label, int *checked);

/// @returns IM_NONE, IM_CLICKED
int im_button(const char *label);
/// @returns IM_NONE, IM_CLICKED
int im_button_ex(const char *label, struct ImModifier *mod);
/// @returns IM_NONE
int im_label(const char *label);

/// @returns IM_NONE, IM_CHILDREN_VISIBLE
int im_begin_window(const char *name, int width_dp, int height_dp);
/// @returns IM_NONE, IM_CHILDREN_VISIBLE
int im_begin_window_ex(const char *name, int width_dp, int height_dp, int *is_open);
void im_end_window(void);

/// @param buffer Buffer that the text will be read from, and where characters will be written to
/// @param size Size of buffer
void im_multiline_entry(char *buffer, unsigned int size);

/// @param buffer Buffer that the text will be read from, and where characters will be written to
/// @param size Size of buffer
void im_entry(const char *label, char *buffer, unsigned int size);

/// @brief Horizontal integer slider
void im_slider(int min, int max, int *value);

/// @brief Horizontal progress bar that goes from 0-100
void im_progress_bar(int progress);

// For internal/extension use only
//void im_apply_prop(struct RimTree *tree);

#ifdef __cplusplus
}
#endif

#endif
