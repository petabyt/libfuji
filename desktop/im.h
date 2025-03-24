#ifndef IM_H
#define IM_H

int im_push_disabled(void);
int im_pop_disabled(void);

int im_combo_box(const char *label, const char *preview);
int im_add_combo_box_item(const char *label, int *selected);
void im_end_combo_box(void);

int im_checkbox_label(const char *label, int *checked);

int im_entry(char *buffer, unsigned int buf_len);

int im_button(const char *label);
int im_label(const char *label);

int im_window(const char *name, int width, int height, int flags);
int im_window_end(void);

#endif
