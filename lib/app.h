#ifndef APP_H
#define APP_H

/// Send current camera name to UI
void app_send_cam_name(const char *name);

/// OS level debug log
void plat_dbg(char *fmt, ...);

// printf to UI
void app_print(char *fmt, ...);

// Test suite verbose logging
void tester_log(char *fmt, ...);
void tester_fail(char *fmt, ...);

int app_bind_socket_wifi(int sockfd);

// TODO: Get PTP runtime, (only one connection allowed at once)
struct PtpRuntime *ptp_get();

void app_increment_progress_bar(int read);

void app_print_id(int resid);
int app_get_string(const char *key);

#endif
