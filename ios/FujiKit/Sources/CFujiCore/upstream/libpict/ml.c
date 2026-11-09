// Magic Lantern and CHDK extension functionality
// Copyright 2023 by Daniel C (https://github.com/petabyt/libpict)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libpict.h>
#include <ptp.h>

// ptplv v1
#define PTP_ML_LvWidth 360
#define PTP_ML_LvHeight 240

// Destination buffer size
#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 480

// BMP VRAM is a little oversized, will be cropped
#define BMP_VRAM_WIDTH 960
#define BMP_VRAM_HEIGHT 480

static struct PtpMlLvInfo lv_info = {0};

static int yuv2rgb_rv[256];
static int yuv2rgb_gu[256];
static int yuv2rgb_gv[256];
static int yuv2rgb_bu[256];

static void precompute_yuv2rgb(void) {
    for (int u = 0; u < 256; u++) {
        int8_t U = (int8_t)u;
        yuv2rgb_gu[u] = (-352 * U) / 1024;
        yuv2rgb_bu[u] = (1812 * U) / 1024;
    }

    for (int v = 0; v < 256; v++) {
        int8_t V = (int8_t)v;
        yuv2rgb_rv[v] = (1437 * V) / 1024;
        yuv2rgb_gv[v] = (-731 * V) / 1024;
    }
}

static inline int coerce(int x, int lo, int hi) {
    return ((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x));
}

static void yuv2rgb(uint8_t Y, uint8_t U, uint8_t V, uint8_t *R, uint8_t *G, uint8_t *B) {
    int v_and_ff = V & 0xFF;
    int u_and_ff = U & 0xFF;
    int v = Y + yuv2rgb_rv[v_and_ff];
    *R = coerce(v, 0, 255);
    v = Y + yuv2rgb_gu[u_and_ff] + yuv2rgb_gv[v_and_ff];
    *G = coerce(v, 0, 255);
    v = Y + yuv2rgb_bu[u_and_ff];
    *B = coerce(v, 0, 255);
}

int ptp_ml_init_bmp_lv(struct PtpRuntime *r) {
	precompute_yuv2rgb();

	struct PtpCommand cmd;
	cmd.code = PTP_OC_ML_LiveBmpRam;
	cmd.param_length = 1;
	cmd.params[0] = PTP_ML_BMP_LV_GET_SPEC;

	int rc = ptp_send(r, &cmd);
	if (rc) {
		return rc;
	}

	memcpy(&lv_info, ptp_get_payload(r), sizeof(lv_info));

	return 0;
}

int ptp_ml_get_bmp_lv(struct PtpRuntime *r, uint32_t **buffer_ptr) {
	buffer_ptr[0] = NULL;

	// Every 100 calls, get the LUT and bmp spec info
	static int toggle = 0;
	toggle++;
	if (toggle > 100) {
		ptp_ml_init_bmp_lv(r);
		toggle = 0;
	}

	struct PtpCommand cmd;
	cmd.code = PTP_OC_ML_LiveBmpRam;
	cmd.param_length = 1;
	cmd.params[0] = PTP_ML_BMP_LV_GET_FRAME;

	int rc = ptp_send(r, &cmd);
	if (rc) {
		buffer_ptr[0] = NULL;
		return rc;
	}

	uint32_t *frame = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
	if (frame == NULL) return PTP_RUNTIME_ERR;

	// TODO: parse ver info from header
	//struct PtpMlLvHeader *header = (struct PtpMlLvHeader *)(ptp_get_payload(r));

	uint8_t *bmp = (uint8_t *)(ptp_get_payload(r));

	int i = 0;
	for (int y = 0; y < BMP_VRAM_HEIGHT; y++) {
		for (int x = 0; x < BMP_VRAM_WIDTH; x++) {
			if (x > SCREEN_WIDTH) {
				i++;
				continue;
			}

			int i2 = y * SCREEN_WIDTH + x;

			uint8_t color = bmp[i];
			uint32_t pal = lv_info.lcd_palette[color];
			i++;
			if (color == 0 || color == 4) {
				pal = 0x00FF0000;
			}

			uint8_t Y, U, V, A;
			A = (pal >> 24) & 0xFF;
			Y = (pal >> 16) & 0xFF;
			U = (pal >>  8) & 0xFF;
			V = (pal >>  0) & 0xFF;

			// Note: alpha layer is special, and needs more research done (it's fine for now)
			// pal 0x00FF0000 == fully transparent. A == 0 || A == 1 is semi-transparent.

			uint8_t R, G, B;
			yuv2rgb(Y, U, V, &R, &G, &B);

			if (pal == 0x00FF0000) {
				A = 0xFF; // fully transparent
			} else {
				A = 0x0; // not transparent
			} // TODO: semi-transparent

			((uint8_t *)&frame[i2])[0] = B;
			((uint8_t *)&frame[i2])[1] = G;
			((uint8_t *)&frame[i2])[2] = R;
			((uint8_t *)&frame[i2])[3] = A;
		}
	}

	buffer_ptr[0] = frame;

	return 0;
}

int ptp_ml_get_liveview_v1(struct PtpRuntime *r) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_ML_Live360x240;
	cmd.param_length = 0;

	return ptp_send(r, &cmd);
}

int ptp_chdk_upload_file(struct PtpRuntime *r, char *input, char *dest) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_CHDK;
	cmd.param_length = 1;
	cmd.params[0] = PTP_CHDK_UploadFile;

	unsigned int size_all = 0;
	char *data = ptp_pack_chdk_upload_file(r, input, dest, &size_all);
	if (data == NULL) {
		return PTP_RUNTIME_ERR;
	}
	
	int rc = ptp_send_data(r, &cmd, data, size_all);
	free(data);
	return rc;
}

int ptp_chdk_get_version(struct PtpRuntime *r) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_CHDK;
	cmd.param_length = 1;
	cmd.params[0] = PTP_CHDK_Version;
	return ptp_send(r, &cmd);
}
