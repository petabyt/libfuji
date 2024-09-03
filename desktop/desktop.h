#ifndef DESKTOP_H
#define DESKTOP_H

#include <camlib.h>

int fudge_test_all_cameras(struct PtpRuntime *r);

/// @brief Setup a thread for networking
void network_init(void);

int fudge_main_ui(void);

int fuji_test_discovery(struct PtpRuntime *r);

int fuji_test_filesystem(struct PtpRuntime *r);
int fuji_test_setup(struct PtpRuntime *r);

#endif