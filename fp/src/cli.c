#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "fp.h"

int parse_fp_files(const char *folder_path, int expect) {
	DIR *dir = opendir(folder_path);
	if (!dir) {
		printf("Can't open %s\n", folder_path);
		return -1;
	}

	struct FujiProfile fp;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		char file_path[1024];
		if (entry->d_type == DT_REG) {
			snprintf(file_path, sizeof(file_path), "%s/%s", folder_path, entry->d_name);
			printf("Parsing %s\n", file_path);
			int rc = fp_parse_fp1(file_path, &fp);
			if (rc != expect) return -1;
		}
	}
	closedir(dir);
	return 0;
}

int parse_raw_files(void) {
	const char *folder_path = "d185";
	DIR *dir = opendir(folder_path);
	if (!dir) return -1;

	struct FujiProfile fp = {0};

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		char file_path[1024];
		if (entry->d_type == DT_REG) {
			snprintf(file_path, sizeof(file_path), "%s/%s", folder_path, entry->d_name);
			printf("Parsing %s\n", file_path);

			FILE *f = fopen(file_path, "rb");
			if (f == NULL) return -1;

			fseek(f, 0, SEEK_END);
			long file_size = ftell(f);
			fseek(f, 0, SEEK_SET);

			uint8_t *buffer = malloc(file_size);
			fread(buffer, 1, file_size, f);

			// 1. d185 -> struct
			int rc = fp_parse_d185(buffer, (int)file_size, &fp);
			if (rc) return rc;

			// 2. struct -> d185
			uint8_t d185_buf[1024] = {0};
			rc = fp_create_d185(&fp, d185_buf, 1024);
			if (rc < 0) return rc;

			{
				for (int i = 0; i < rc; i++) {
					printf("%02x ", d185_buf[i]);
					if ((i % 10) == 0 && i != 0) printf("\n");
				}
				printf("\n--------\n");
				#if 0
				char new_path[1024];
				sprintf(new_path, "%s_m", file_path);
				FILE *out = fopen(new_path, "wb");
				fwrite(d185_buf, 1, rc, out);
				fclose(out);
				#endif
			}

			rc = fp_parse_d185(d185_buf, (int)file_size, &fp);
			if (rc) return rc;

			rc = fp_dump_struct(stdout, &fp);
			if (rc) return rc;

			free(buffer);
			fclose(f);
		}
	}
	closedir(dir);
	return 0;	
}

int test_d185(void) {
	return 0;
}

int test(void) {
	int rc;
	rc = parse_fp_files("fp1", 0);
	if (rc) return rc;
	rc = parse_fp_files("fp2", 0);
	if (rc) return rc;
	rc = parse_fp_files("fp1_fail", -1);
	if (rc) return rc;
	rc = test_d185();
	if (rc) return rc;
	rc = parse_raw_files();
	return rc;
}

int main(int argc, char **argv) {
	int rc = test();
	printf("rc: %d '%s'\n", rc, fp_get_error());
	return rc;
}
