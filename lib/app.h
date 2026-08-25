/// @file
/// Layer for Fudge frontend

#ifndef APP_H
#define APP_H

#include <libpict.h>

/// @brief Send current camera name to UI
void app_send_cam_name(struct PtpRuntime *r, const char *name);
void app_update_storage_info(struct PtpRuntime *r);

/// @brief OS level debug log
void plat_dbg(char *fmt, ...);

/// @brief Ping UI with update
void app_print(struct PtpRuntime *r, char *fmt, ...);

// Test suite verbose logging
void tester_log(struct PtpRuntime *r, char *fmt, ...);
void tester_fail(struct PtpRuntime *r, char *fmt, ...);

void app_report_download_speed(struct PtpRuntime *r, long time, size_t size);

/// @brief Check if the current downloader thread has been marked as canceled
int app_check_thread_cancel(struct PtpRuntime *r);

int plat_update_object_info(struct PtpRuntime *r, int handle, const struct PtpObjectInfo *oi);

int app_ptp_download_file(struct PtpRuntime *r, struct PtpObjectInfo *oi, int object_id, unsigned int max_chunk_size, int index);

int app_queue_file_for_download(struct PtpRuntime *r, int object_id);

#endif
