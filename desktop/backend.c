#include <stdio.h>
#include <stdlib.h>
#include <app.h>

void app_get_file_path(char buffer[256], const char *filename) {
	snprintf(buffer, 256, "%s", filename);
}

void app_get_tether_file_path(char buffer[256]) {
	snprintf(buffer, 256, "TETHER.JPG");
}
