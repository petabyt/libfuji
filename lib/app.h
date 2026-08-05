/// @file
/// Layer for Fudge frontend

#ifndef APP_H
#define APP_H

#include <libpict.h>

/// @brief Send current camera name to UI
void app_send_cam_name(struct PtpRuntime *r, const char *name);

/// @brief OS level debug log
void plat_dbg(char *fmt, ...);

/// @brief Ping UI with update
void app_print(struct PtpRuntime *r, char *fmt, ...);

// Test suite verbose logging
void tester_log(struct PtpRuntime *r, char *fmt, ...);
void tester_fail(struct PtpRuntime *r, char *fmt, ...);

void app_report_download_speed(struct PtpRuntime *r, long time, size_t size);

/// @brief Get download path for a new file, for fopen()
void app_get_file_path(struct PtpRuntime *r, char buffer[256], const char *filename);

void app_get_tether_file_path(struct PtpRuntime *r, char buffer[256]);

/// @brief Check if the current downloader thread has been marked as canceled
int app_check_thread_cancel(struct PtpRuntime *r);

/// @brief For backend to notify frontend a file is being downloaded
void app_downloading_file(struct PtpRuntime *r, const struct PtpObjectInfo *oi);

/// @brief For backend to notify frontend a file has been downloaded
void app_downloaded_file(struct PtpRuntime *r, const struct PtpObjectInfo *oi, const char *path);

int plat_update_object_info(struct PtpRuntime *r, int handle, const struct PtpObjectInfo *oi);

#endif
