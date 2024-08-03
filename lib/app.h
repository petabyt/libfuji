#ifndef APP_H
#define APP_H

/// Send current camera name to UI
void app_send_cam_name(const char *name);

/// OS level debug log
void plat_dbg(char *fmt, ...);

/// printf to UI
void app_print(char *fmt, ...);

// Test suite verbose logging
void tester_log(char *fmt, ...);
void tester_fail(char *fmt, ...);

/// Bind to default or user-selected wifi device
int app_bind_socket_wifi(int sockfd);

// Eventually: Get PTP runtime, (only one connection allowed at once)
struct PtpRuntime *ptp_get();

/// Call for every chunk/packet read
void app_increment_progress_bar(int read);

void app_print_id(int resid);
int app_get_string(const char *key);
void app_get_file_path(char buffer[256], const char *filename);

int app_check_thread_cancel();

#endif
