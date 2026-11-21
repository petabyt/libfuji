#include <string.h>
#include <libpict.h>

char *enum_null = "(null)";

int ptp_enum_index(char *string, int *value, int i) {
	if (i >= ptp_enums_length) {
		return 1;
	}

	strcpy(string, ptp_enums[i].name);
	value[0] = ptp_enums[i].value;
	return 0;
}

int ptp_enum_all(char *string) {
	for (int i = 0; i < ptp_enums_length; i++) {
		if (!strcmp(string, ptp_enums[i].name)) {
			return ptp_enums[i].value;
		}
	}

	return -1;
}

int ptp_enum(int type, char *string) {
	for (int i = 0; i < ptp_enums_length; i++) {
		if (!strcmp(string, ptp_enums[i].name) && ptp_enums[i].type == type) {
			return ptp_enums[i].value;
		}
	}

	return -1;
}

char *ptp_get_enum_all(int id) {
	for (int i = 0; i < ptp_enums_length; i++) {
		if (id == ptp_enums[i].value) {
			return ptp_enums[i].name;
		}
	}

	return enum_null;
}

char *ptp_get_enum(int type, int vendor, int id) {
	for (int i = 0; i < ptp_enums_length; i++) {
		if (ptp_enums[i].value == id && ptp_enums[i].type == type
				&& (ptp_enums[i].vendor == vendor || ptp_enums[i].vendor == PTP_DEV_EMPTY)) {
			return ptp_enums[i].name;
		}
	}

	return enum_null;
}
