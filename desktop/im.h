// Dear-ImGUI wrapper
// I have some future plans with this API I want to try.
#ifndef IM_H
#define IM_H
int im_push_disabled(void);
int im_pop_disabled(void);

int im_tab(void);
int im_add_tab_item(const char *title);
void im_end_tab_item();
void im_end_tab();

int im_combo_box(const char *label, const char *preview);
int im_add_combo_box_item(const char *label, int *selected);
void im_end_combo_box(void);

int im_checkbox_label(const char *label, int *checked);

int im_button(const char *label);
int im_label(const char *label);

int im_window(const char *name, int width, int height, int flags);
int im_window_end(void);

/// @param buffer Buffer that the text will be read from, and where characters will be written to
/// @param size Size of buffer
void im_multiline_entry(char *buffer, size_t size, int flags);

/// @param buffer Buffer that the text will be read from, and where characters will be written to
/// @param size Size of buffer
void im_entry(const char *label, char *buffer, size_t size, int flags);

#endif
