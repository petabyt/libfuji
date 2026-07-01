// Copyright 2025 Daniel C
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "fp.h"

inline static int read_u8(const void *buf, uint8_t *out) {
	const uint8_t *b = buf;
	*out = b[0];
	return 1;
}
inline static int read_u16(const void *buf, uint16_t *out) {
	const uint8_t *b = buf;
	*out = (uint16_t)b[0] | ((uint16_t)b[1] << 8);
	return 2;
}
inline static int read_u32(const void *buf, uint32_t *out) {
	const uint8_t *b = buf;
	*out = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
	return 4;
}
inline static int write_u8(void *buf, uint8_t out) {
	((uint8_t *)buf)[0] = out;
	return 1;
}
inline static int write_u16(void *buf, uint16_t out) {
	uint8_t *b = buf;
	b[0] = out & 0xFF;
	b[1] = (out >> 8) & 0xFF;
	return 2;
}
inline static int write_u32(void *buf, uint32_t out) {
	uint8_t *b = buf;
	b[0] = out & 0xFF;
	b[1] = (out >> 8) & 0xFF;
	b[2] = (out >> 16) & 0xFF;
	b[3] = (out >> 24) & 0xFF;
	return 4;
}

int validate_prop(uint32_t value, uint32_t *output, struct FujiLookup *tbl) {
	// TODO: output should be uint32_t
	for (int i = 0; tbl[i].key != NULL; i++) {
		if (value == tbl[i].value) {
			output[0] = value;
			return 0;
		}
	}
	return -1;
}

static int parse_prop(struct FujiProfile *fp, int idx, uint32_t value) {
	switch (idx) {
	case 0:
		// 2
		fp->ShootingCondition = value; 
		return 0;
	case 1:
		// 7
		fp->FileType = value;
		return 0;
	case 2:
		return validate_prop(value, &fp->ImageSize, fp_image_size);
	case 3:
		return validate_prop(value, &fp->ImageQuality, fp_image_quality);
	case 4:
		return validate_prop(value, &fp->ExposureBias, fp_exposure_bias);
	case 5:
		return validate_prop(value, &fp->DynamicRange, fp_drange);
	case 6:
		return validate_prop(value, &fp->WideDRange, fp_drange_priority);
	case 7:
		return validate_prop(value, &fp->FilmSimulation, fp_film_sim);
	case 8:
		return validate_prop(value, &fp->GrainEffect, fp_grain_effect);
	case 9:
		fp->SmoothSkinEffect = value; // TODO
		return 0;
	case 10:
		fp->WBShootCond = value; // TODO
		return 0;
	case 11:
		return validate_prop(value, &fp->WhiteBalance, fp_white_balance);
	case 12:
		return validate_prop(value, &fp->WBShiftR, fp_range);
	case 13:
		return validate_prop(value, &fp->WBShiftB, fp_range);
	case 14:
		return validate_prop(value, &fp->WBColorTemp, fp_color_temp);
	case 15:
		return validate_prop(value, &fp->HighlightTone, fp_get_highlight_tone(fp));
	case 16:
		return validate_prop(value, &fp->ShadowTone, fp_get_shadow_tone(fp));
	case 17:
		return validate_prop(value, &fp->Color, fp_range);
	case 18:
		return validate_prop(value, &fp->Sharpness, fp_range);
	case 19:
		return validate_prop(value, &fp->NoisReduction, fp_noise_reduction);
	case 20:
		return validate_prop(value, &fp->Clarity, fp_clarity);
	case 21:
		return validate_prop(value, &fp->ColorSpace, fp_color_space);
	case 22:
		// HDR??
	case 23:
		// DigitalTeleConv??
	case 24:
		// PortraitEnhancer
	case 25:
		// Something to do with RejectedValue?
	case 26:
	case 27:
	case 28:
		// ????
		return 0;
	default:
		return -1;
	}
}
static int get_prop(const struct FujiProfile *fp, int idx, uint32_t *value) {
	switch (idx) {
	case 0:
		(*value) = 0x2;
		return 0;
	case 1:
		(*value) = 0x7;
		return 0;
	case 2:
		return validate_prop(fp->ImageSize, value, fp_image_size);
	case 3:
		return validate_prop(fp->ImageQuality, value, fp_image_quality);
	case 4:
		return validate_prop(fp->ExposureBias, value, fp_exposure_bias);
	case 5:
		return validate_prop(fp->DynamicRange, value, fp_drange);
	case 6:
		return validate_prop(fp->WideDRange, value, fp_drange_priority);
	case 7:
		return validate_prop(fp->FilmSimulation, value, fp_film_sim);
	case 8:
		return validate_prop(fp->GrainEffect, value, fp_grain_effect);
	case 9:
		(*value) = fp->SmoothSkinEffect;
		return 0;
	case 10:
		(*value) = fp->WBShootCond;
		return 0;
	case 11:
		return validate_prop(fp->WhiteBalance, value, fp_white_balance);
	case 12:
		return validate_prop(fp->WBShiftR, value, fp_range);
	case 13:
		return validate_prop(fp->WBShiftB, value, fp_range);
	case 14:
		return validate_prop(fp->WBColorTemp, value, fp_color_temp);
	case 15:
		return validate_prop(fp->HighlightTone, value, fp_get_highlight_tone(fp));
	case 16:
		return validate_prop(fp->ShadowTone, value, fp_get_shadow_tone(fp));
	case 17:
		return validate_prop(fp->Color, value, fp_range);
	case 18:
		return validate_prop(fp->Sharpness, value, fp_range);
	case 19:
		return validate_prop(fp->NoisReduction, value, fp_noise_reduction);
	case 20:
		return validate_prop(fp->Clarity, value, fp_clarity);
	case 21:
		return validate_prop(fp->ColorSpace, value, fp_color_space);
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
		(*value) = 0;
		return 0;
	default:
		return -1;
	}
}

int fp_create_d185(const struct FujiProfile *fp, uint8_t *bin, int len) {
	if (!(len >= 0x274)) return -1;
	int of = 0;
	of += write_u16(bin + of, 0x1d);

	char temp_iop_code[64];
	snprintf(temp_iop_code, sizeof(temp_iop_code), "%08X", fp->IOPCode);

	of += write_u8(bin + of, strlen(temp_iop_code) + 1);
	for (int i = 0; temp_iop_code[i] != '\0'; i++) {
		of += write_u16(bin + of, (uint16_t)temp_iop_code[i]);
	}
	of += write_u16(bin + of, 0x0);

	while (of < 0x201) {
		of += write_u8(bin + of, 0x0);
	}

	for (int i = 0; i < 0x1d; i++) {
		uint32_t value;
		int rc = get_prop(fp, i, &value);
		if (rc) {
			fp_set_error("Error packing property %d", i);
			return rc;
		}
		of += write_u32(bin + of, value);
	}
	
	return of;
}

int fp_parse_d185(const uint8_t *bin, int len, struct FujiProfile *fp) {
	if (len < 0x200) return -1;

	memset(fp, 0, sizeof(struct FujiProfile));

	//const struct FujiBinaryProfile *profile = (const struct FujiBinaryProfile *)bin;

	int of = 0;

	uint16_t n_props;
	uint8_t str_len;
	of += read_u16(bin + of, &n_props);
	of += read_u8(bin + of, &str_len);

	uint16_t wchr;
	uint32_t iop_code = 0;
	for (int i = 0; i < (int)str_len; i++) {
		of += read_u16(bin + of, &wchr);
		if (wchr == 0) break; // Skip NULL character (the camera doesn't even include it)
		if (i > 8) return -1; // Don't handle more than 1 iopcode
		iop_code *= 16;
		if (wchr >= '0' && wchr <= '9') {
			iop_code += (char)wchr - '0';
		} else if (wchr >= 'A' && wchr <= 'F') {
			iop_code += (char)wchr - 'A' + 10;
		} else {
			fp_set_error("Invalid char in IOPCode");
			return -1;
		}
	}

	fp->IOPCode = iop_code;

	of = 0x201;
	for (int i = 0; i < (int)n_props; i++) {
		uint32_t value;
		if ((of + (int)sizeof(uint32_t)) > len) {
			// Assume value will be zero if data structure cuts off before props list ends
			value = 0;
		} else {
			of += read_u32(bin + of, &value);
		}
		int rc = parse_prop(fp, i, value);
		if (rc) {
			fp_set_error("Error parsing property of value %x at index %d", value, i);
			return -1;
		}
	}

	return 0;
}
