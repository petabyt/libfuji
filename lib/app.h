#ifndef APP_H
#define APP_H

#include <ptp.h>

/// @brief Send current camera name to UI
void app_send_cam_name(const char *name);

/// @brief OS level debug log
void plat_dbg(char *fmt, ...);

/// @brief Ping UI with update
void app_print(char *fmt, ...);
/// @brief Ping UI with update (accepts localized resource string)
void app_print_id(int resid);

// Test suite verbose logging
void tester_log(char *fmt, ...);
void tester_fail(char *fmt, ...);

/// @brief Get default PTP object
/// @note Eventually this will be removed when more than one connection is allowed at once
struct PtpRuntime *ptp_get(void);

/// @brief Called for every chunk/packet read
void app_increment_progress_bar(int read);

void app_set_progress_bar(int status, int size);

void app_report_download_speed(long time, size_t size);

/// @brief Get string ID from key/ID
int app_get_string(const char *key);

/// @brief Get download path for a new file, for fopen()
void app_get_file_path(char buffer[256], const char *filename);

void app_get_tether_file_path(char buffer[256], const char *filename);

/// @brief Check if the current downloader thread has been marked as canceled
int app_check_thread_cancel(void);

/// @brief Pings the frontned when a file is going to be downloaded
void app_downloading_file(const struct PtpObjectInfo *oi);

/// @brief Pings the frontend when a file has been downloaded
void app_downloaded_file(const struct PtpObjectInfo *oi, const char *path);

struct NetworkHandle {
	long long android_fd;
	int ignore;
};

int app_get_os_network_handle(struct NetworkHandle *h);
int app_get_wifi_network_handle(struct NetworkHandle *h);
int app_bind_socket_to_network(int fd, struct NetworkHandle *h);

#endif
