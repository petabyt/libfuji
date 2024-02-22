#ifndef APP_H
#define APP_H

// Ping user with technical updates
void ui_send_text(char *key, char *fmt, ...);

// printf to kernel
void plat_dbg(char *fmt, ...);

// printf to UI
void app_print(char *fmt, ...);

// Test suite verbose logging
void tester_log(char *fmt, ...);
void tester_fail(char *fmt, ...);

#endif
