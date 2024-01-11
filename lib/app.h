#ifndef APP_H
#define APP_H

// printf to UI
void app_print(char *fmt, ...);

// Test suite verbose logging
void tester_log(char *fmt, ...);
void tester_fail(char *fmt, ...);

#endif
