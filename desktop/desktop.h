#ifndef DESKTOP_H
#define DESKTOP_H

int fudge_test_all_cameras(void);

/// @brief Setup a thread for networking
void network_init(void);

int fudge_main_ui(void);

void app_log_clear(void);
void app_update_connected_status(int connected);
void fudge_disconnect_all(void);

void *fudge_usb_connect_thread(void *arg);

void *fudge_backup_settings(void *arg);

int fuji_connect_run_script(const char *filename);

// Tests
int fuji_test_discovery(struct PtpRuntime *r);
int fuji_test_filesystem(struct PtpRuntime *r);
int fuji_test_setup(struct PtpRuntime *r);


#endif
