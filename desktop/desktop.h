// Any functionality specific to the desktop/cli utility
#ifndef DESKTOP_H
#define DESKTOP_H
#include <stdio.h>

int fudge_test_all_cameras(void);

/// @brief Setup a thread for networking
void network_init(void);

int fudge_main_ui(void);

void app_log_clear(void);
void app_update_connected_status(int connected);
void fudge_disconnect_all(void);

void *fudge_usb_connect_thread(void *arg);

//void *fudge_backup_settings(void *arg);
int fudge_dump_usb(int devnum);
int fuji_connect_run_script(int devnum, const char *filename);
int fudge_cli_backup(int devnum, const char *filename);

// Tests
int fuji_test_discovery(struct PtpRuntime *r);
//int fuji_test_filesystem(struct PtpRuntime *r);
//int fuji_test_setup(struct PtpRuntime *r);

/// @brief CLI function to do quick conversion
int fudge_process_raf(int devnum, const char *input, const char *output, const char *profile);

int fudge_download_backup(int devnum, const char *output);
int fudge_restore_backup(int devnum, const char *output);

int fujiusb_restore_backup(struct PtpRuntime *r, FILE *input);

#endif
