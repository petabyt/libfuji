#ifndef IM_H
#define IM_H

#if 0

im_combo_box("Choose an item");
im_combo_box_item("1", &selected);
im_combo_box_item("2", &selected);
im_combo_box_item("3", &selected);
im_end_combo_box();

#endif

int im_push_disabled(void);
int im_pop_disabled(void);

int im_combo_box(const char *label, int *selected);
int im_combo_box_item(const char *label);
int im_combo_box_end(void);

int im_checkbox_label(const char *label, int *checked);

int im_entry(char *buffer, unsigned int buf_len);

int im_button(const char *label);
int im_label(const char *label);

int im_window(const char *name, int width, int height, int flags);
int im_window_end(void);

#endif
