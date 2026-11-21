// Proprietary vendor property data conversion to standard simple formats
// Copyright 2022 by Daniel C (https://github.com/petabyt/libpict)

#include <string.h>

#include <libpict.h>

struct CanonShutterSpeed {
	int value; // value times 100000
	int data; // data from camera
}canon_shutter[] = {
	{0, 0xc}, // BULB 1300D
	{0, 0x4}, // BULB 5dmk3
	{300000,0x10},
	{250000,0x13},
	{200000,0x15},
	{150000,0x18},
	{130000,0x1b},
	{100000,0x1d},
	{80000,0x20},
	{60000,0x23},
	{50000,0x25},
	{40000,0x28},
	{32000,0x2b},
	{25000,0x2d},
	{20000,0x30},
	{16000,0x33},
	{13000,0x35},
	{10000,0x38},
	{8000,0x3b},
	{6000,0x3d},
	{5000,0x40},
	{4000,0x43},
	{3000,0x45},
	{2500,0x48},
	{2000,0x4b},
	{1666,0x4d},
	{1250,0x50},
	{10000 / 10,0x53},
	{10000 / 13,0x55},
	{10000 / 15,0x58},
	{10000 / 20,0x5b},
	{10000 / 25,0x5d},
	{10000 / 30,0x60},
	{10000 / 40,0x63},
	{10000 / 50,0x65},
	{10000 / 60,0x68},
	{10000 / 80,0x6b},
	{10000 / 100,0x6d},
	{10000 / 125,0x70},
	{10000 / 160,0x73},
	{10000 / 200,0x75},
	{10000 / 250,0x78},
	{10000 / 320,0x7b},
	{10000 / 400,0x7d},
	{10000 / 500,0x80},
	{10000 / 640,0x83},
	{10000 / 800,0x85},
	{10000 / 1000,0x88},
	{10000 / 1250,0x8b},
	{10000 / 1600,0x8d},
	{10000 / 2000,0x90},
	{10000 / 2500,0x93},
	{10000 / 3200,0x95},
	{10000 / 4000,0x98},
	{10000 / 5000,0x9a},
	{10000 / 6400,0x9d},
	{10000 / 8000,0xa0},
};

int ptp_eos_get_shutter(int data, int *out, int dir) {
	for (int i = 0; i < (int)(sizeof(canon_shutter) / sizeof(struct CanonShutterSpeed)); i++) {
		if (dir) {
			if (canon_shutter[i].value == data) {
				(*out) = canon_shutter[i].data;
				return 0;
			}
		} else {
			if (canon_shutter[i].data == data) {
				(*out) = canon_shutter[i].value;
				return 0;
			}
		}
	}
	return -1;
}

struct CanonISO {
	int value;
	int data;
}canon_iso[] = {
	{0, 0}, // AUTO
	{50, 0x40},
	{100, 0x48},
	{125, 0x4b},
	{160, 0x4d},
	{200, 0x50},
	{250, 0x53},
	{320, 0x55},
	{400, 0x58},
	{500, 0x5b},
	{640, 0x5b},
	{800, 0x60},
	{1000, 0x63},
	{1250, 0x65},
	{1600, 0x68},
	{3200, 0x70},
	{6400, 0x78},
	{12800, 0x78+8},
};

int ptp_eos_get_iso(int data, int *out, int dir) {
	for (int i = 0; i < (int)(sizeof(canon_iso) / sizeof(struct CanonISO)); i++) {
		if (dir) {
			if (canon_iso[i].value == data) {
				(*out) = canon_iso[i].data;
				return 0;
			}
		} else {
			if (canon_iso[i].data == data) {
				(*out) = canon_iso[i].value;
				return 0;
			}
		}
	}
	return -1;
}

#if 0
Notes:
{0, 0} // Undefined
{1, 0} // Manual (RGB gain)
{2, 0} // Automatic
{3, 0} // One push automatic
{4, 1} // Daylight
{5, 4} // Florescent
{6, 3} // Tungsten
{7, 8} // Flash
#endif

struct CanonWhiteBalance {
	int value;
	int data;
}canon_white_balance[] = {
	{PTP_WB_Automatic, 0}, // AUTO
	{PTP_WB_Daylight, 1}, // Daylight
//	{2, 8}, // Shade
	{PTP_WB_Tungsten, 3}, // Tungsten/Incandescent
	{PTP_WB_Florescent, 4}, // White florescent
};

int ptp_eos_get_white_balance(int data, int *out, int dir) {
	for (int i = 0; i < (int)(sizeof(canon_white_balance) / sizeof(canon_white_balance[0])); i++) {
		if (dir) {
			if (canon_white_balance[i].value == data) {
				(*out) = canon_white_balance[i].data;
				return 0;
			}
		} else {
			if (canon_white_balance[i].data == data) {
				(*out) = canon_white_balance[i].value;
				return 0;
			}
		}
	}
	return -1;
}

struct CanonAperture {
	int value;
	int data;
}canon_aperture[] = {
	{120, 0xd},
	{140, 0x10},
	{160, 0x13},
	{180, 0x15},
	{200, 0x18},
	{220, 0x1b},
	{250, 0x1d},
	{280, 0x20},
	{320, 0x23},
	{350, 0x25},
	{400, 0x28},
	{450, 0x2b},
	{500, 0x2d},
	{560, 0x30},
	{630, 0x33},
	{710, 0x35},
	{800, 0x38},
	{900, 0x3b},
	{1000, 0x3d},
	{1100, 0x40},
	{1300, 0x43},
	{1400, 0x45},
	{1600, 0x48},
	{1800, 0x4b},
	{2000, 0x4d},
	{2200, 0x50},
	{2500, 0x53},
	{2900, 0x55},
	{3200, 0x58},
};

int ptp_eos_get_aperture(int data, int *out, int dir) {
	for (int i = 0; i < (int)(sizeof(canon_aperture) / sizeof(struct CanonAperture)); i++) {
		if (dir) {
			if (canon_aperture[i].value == data) {
				(*out) = canon_aperture[i].data;
				return 0;
			}
		} else {
			if (canon_aperture[i].data == data) {
				(*out) = canon_aperture[i].value;
				return 0;
			}
		}
	}
	return -1;
}

// Lots of confusing types (resolutions, raw+jpeg, superfine, etc)
// Converts to libpict wrapper types (enum ImageFormats)
struct CanonImageFormats {
	int value;
	uint32_t data[9];
}canon_imgformats[] = {
	{IMG_FORMAT_RAW, {1, 16, 6, 0, 4}}, // RAW
	{IMG_FORMAT_STD, {1, 16, 1, 0, 2}}, // STD
	{IMG_FORMAT_HIGH, {1, 16, 1, 0, 3}}, // HIGH
	{IMG_FORMAT_RAW_JPEG, {2, 16, 6, 0, 4, 16, 1, 0, 3}}, // RAW + HIGH JPG
};

int ptp_eos_get_imgformat_value(uint32_t data[5], int *out) {
	for (int i = 0; i < (int)(sizeof(canon_imgformats) / sizeof(struct CanonImageFormats)); i++) {
		if (!memcmp(canon_imgformats[i].data, data, sizeof(int) * 5)) {
			(*out) = canon_imgformats[i].value;
			return 0;
		}
	}
	return -1;
}
