// Copyright 2025 Daniel C
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "fp.h"

//enum D185Props {
//	D185_UNKNOWN_0 = 0,
//	D185_SHOOTING_CONDITION = 1,
//	D185_FILE_TYPE = 2,
//	D185_IMAGE_SIZE = 3,
//	D185_IMAGE_QUALITY = 4,
//	D185_EXPOSURE_BIAS = 5,
//	D185_DYNAMIC_RANGE = 6,
//	D185_WIDE_D_RANGE = 7,
//	D185_FILM_SIMULATION = 8,
//	D185_GRAIN_EFFECT_AND_SIZE = 9,
//	D185_CHROME_EFFECT = 10,
//	D185_WHITE_BALANCE_SHOOT_COND = 11,
//	D185_WHITE_BALANCE_MODE = 12,
//	D185_WHITE_BALANCE_SHIFT_RED_GREEN = 13,
//	D185_WHITE_BALANCE_SHIFT_BLUE_YELLOW = 14,
//	D185_WHITE_BALANCE_COLOR_TEMP = 15,
//	D185_HIGHLIGHT_TONE = 16,
//	D185_SHADOW_TONE = 17,
//	D185_COLOR = 18,
//	D185_SHARPNESS = 19,
//	D185_NOISE_REDUCTION = 20,
//	D185_LENS_MODULATION_OPTIMIZATION = 21,
//	D185_COLOR_SPACE = 22,
//	D185_BLACK_IMAGE_TONE = 23,
//	D185_UNKNOWN_24 = 24,
//	D185_COLOR_CHROME_BLUE = 25,
//	D185_MONOCHROMATIC_COLOR_RG = 26,
//	D185_CLARITY = 27,
//	D185_UNKNOWN_28 = 28,
//	D185_UNKNOWN_29 = 29,
//	NR_OF_D185_PROPERTIES,
//};

enum D185Props {
	D185_SHOOTING_CONDITION = 0,
	D185_FILE_TYPE = 1,
	D185_IMAGE_SIZE = 2,
	D185_IMAGE_QUALITY = 3,
	D185_EXPOSURE_BIAS = 4,
	D185_DYNAMIC_RANGE = 5,
	D185_WIDE_D_RANGE = 6,
	D185_FILM_SIMULATION = 7,
	D185_GRAIN_EFFECT_AND_SIZE = 8,
	D185_CHROME_EFFECT = 9,
	D185_WHITE_BALANCE_SHOOT_COND = 10,
	D185_WHITE_BALANCE_MODE = 11,
	D185_WHITE_BALANCE_SHIFT_RED_GREEN = 12,
	D185_WHITE_BALANCE_SHIFT_BLUE_YELLOW = 13,
	D185_WHITE_BALANCE_COLOR_TEMP = 14,
	D185_HIGHLIGHT_TONE = 15,
	D185_SHADOW_TONE = 16,
	D185_COLOR = 17,
	D185_SHARPNESS = 18,
	D185_NOISE_REDUCTION = 19,
	D185_LENS_MODULATION_OPTIMIZATION = 20,
	D185_COLOR_SPACE = 21,
	D185_BLACK_IMAGE_TONE = 22,
	D185_UNKNOWN_24 = 23,
	D185_COLOR_CHROME_BLUE = 24,
	D185_MONOCHROMATIC_COLOR_RG = 25,
	D185_CLARITY = 26,
	D185_UNKNOWN_28 = 27,
	D185_UNKNOWN_29 = 28,
};

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

int copy_value_if_valid(uint32_t value, uint32_t *output, struct FujiLookup *tbl) {
	for (int i = 0; tbl[i].key != NULL; i++) {
		if (value == tbl[i].value) {
			*output = value;
			return 0;
		}
	}
	if (value == 0) return 0;
	return -1;
}

static int parse_prop(struct FujiProfile *fp, int idx, uint32_t value) {
    switch (idx) {
    case D185_SHOOTING_CONDITION:
        return copy_value_if_valid(value, &fp->ShootingCondition, fp_shooting_condition);
    case D185_FILE_TYPE:
        return copy_value_if_valid(value, &fp->FileType, fp_file_type);
    case D185_IMAGE_SIZE:
        return copy_value_if_valid(value, &fp->ImageSize, fp_image_size);
    case D185_IMAGE_QUALITY:
        return copy_value_if_valid(value, &fp->ImageQuality, fp_image_quality);
    case D185_EXPOSURE_BIAS:
        return copy_value_if_valid(value, &fp->ExposureBias, fp_exposure_bias);
    case D185_DYNAMIC_RANGE:
        // can only select equal or lower than in RAW
        // Note: shared with HDR field. HDR rejects drange. can go up to HDR800 (800)
        // HDR_PLUS = 800 + drange_prio = 3
        return copy_value_if_valid(value, &fp->DynamicRange, fp_drange);
    case D185_WIDE_D_RANGE:
        // Note: inhibits: DynamicRange (keeps original)
        // 		 rejects: HighlightTone (sets 0), ShadowTone (sets 0)
        return copy_value_if_valid(value, &fp->WideDRange, fp_drange_priority);
    case D185_FILM_SIMULATION:
        return copy_value_if_valid(value, &fp->FilmSimulation, fp_film_sim);
    case D185_GRAIN_EFFECT_AND_SIZE: {
        // Note: FP_GRAIN_OFF inhibits FujiGrainEffectSize
        uint32_t value_no_grain_size = value;
        if (value_no_grain_size > FP_GRAIN_STRONG) {
            value_no_grain_size -= FP_GRAIN_SIZE_LARGE;
        }
        if (copy_value_if_valid(value_no_grain_size, &fp->GrainEffect, fp_grain_effect)) return -1;
        if (copy_value_if_valid((value - value_no_grain_size), &fp->GrainEffectSize, fp_grain_effect_size)) return -1;
        return 0;
    };
    case D185_CHROME_EFFECT:
        // COLOR CHROME EFFECT
        return copy_value_if_valid(value, &fp->ChromeEffect, fp_chrome_effect);
    case D185_WHITE_BALANCE_SHOOT_COND:
        // use as shot (wb shoot condition)
        fp->WBShootCond = value; // TODO
        // precedence: WBShootCond, WhiteBalance, WBColorTemp
        return copy_value_if_valid(value, &fp->WBShootCond, fp_white_balance_shoot_cond);
    case D185_WHITE_BALANCE_MODE:
        return copy_value_if_valid(value, &fp->WhiteBalance, fp_white_balance_mode);
        // Note: inhibits prop 14, unless WB_TEMPERATURE
    case D185_WHITE_BALANCE_SHIFT_RED_GREEN:
        return copy_value_if_valid(value, &fp->WBShiftR, fp_range_wb_shift);
    case D185_WHITE_BALANCE_SHIFT_BLUE_YELLOW:
        return copy_value_if_valid(value, &fp->WBShiftB, fp_range_wb_shift);
    case D185_WHITE_BALANCE_COLOR_TEMP:
        return copy_value_if_valid(value, &fp->WBColorTemp, fp_color_temp);
    case D185_HIGHLIGHT_TONE:
        return copy_value_if_valid(value, &fp->HighlightTone, fp_get_highlight_tone(fp));
    case D185_SHADOW_TONE:
        return copy_value_if_valid(value, &fp->ShadowTone, fp_get_shadow_tone(fp));
    case D185_COLOR:
        return copy_value_if_valid(value, &fp->Color, fp_range);
    case D185_SHARPNESS:
        return copy_value_if_valid(value, &fp->Sharpness, fp_range);
    case D185_NOISE_REDUCTION:
        return copy_value_if_valid(value, &fp->NoisReduction, fp_noise_reduction);
    case D185_LENS_MODULATION_OPTIMIZATION:
        return copy_value_if_valid(value, &fp->LensModulationOpt, fp_lens_modulation);
    case D185_COLOR_SPACE:
        return copy_value_if_valid(value, &fp->ColorSpace, fp_color_space);
    case D185_BLACK_IMAGE_TONE:
        // something with WB presets
        //				22				25
        // take some "random values", that are reset to 0, when setting WB shift
        // e.g.
        // AsShot		0x08B08000		0x004F4005
        // Underwater	0x08FA2B80		0x08FA2B10
        // Incadecent	0x095C7000		0x004F4005
        // Shade		0x18C7A858		0x0
        // Daylight		0x09026000		0x004F4005
        // Fluo-1		0x097D3000		0x004F4005
        //
        // or if Acros or Monochrome, then: WC (WARM-COOL) shift in 0.1 steps up to +/-180
        return copy_value_if_valid(value, &fp->BlackImageTone, fp_range_mono_shift_warm_cold);
    case D185_UNKNOWN_24:
        return 0; // TODO, unknown
    case D185_COLOR_CHROME_BLUE:
        // COLOR CHROME FX BLUE
        return copy_value_if_valid(value, &fp->ColorChromeBlue, fp_color_chrome_blue);
    case D185_MONOCHROMATIC_COLOR_RG:
        // see prop 22 above
        // or if Acros or Monochrome, then: MG (MAGENTA-GREEN) shift up to +/-180
        return copy_value_if_valid(value, &fp->MonochromaticColor_RG, fp_range_mono_green_magenta);
    case D185_CLARITY:
        return copy_value_if_valid(value, &fp->Clarity, fp_clarity);
    case D185_UNKNOWN_28:
    case D185_UNKNOWN_29:
        return 0; // TODO, unknown
    default:
        return -1;
    }
}
static int get_prop(const struct FujiProfile *fp, int idx, uint32_t *value) {
	switch (idx) {
	case D185_SHOOTING_CONDITION:
		return copy_value_if_valid(fp->ShootingCondition, value, fp_shooting_condition);
	case D185_FILE_TYPE:
		return copy_value_if_valid(fp->FileType, value, fp_file_type);
	case D185_IMAGE_SIZE:
		return copy_value_if_valid(fp->ImageSize, value, fp_image_size);
	case D185_IMAGE_QUALITY:
		return copy_value_if_valid(fp->ImageQuality, value, fp_image_quality);
	case D185_EXPOSURE_BIAS:
		return copy_value_if_valid(fp->ExposureBias, value, fp_exposure_bias);
	case D185_DYNAMIC_RANGE:
		return copy_value_if_valid(fp->DynamicRange, value, fp_drange);
	case D185_WIDE_D_RANGE:
		return copy_value_if_valid(fp->WideDRange, value, fp_drange_priority);
	case D185_FILM_SIMULATION:
		return copy_value_if_valid(fp->FilmSimulation, value, fp_film_sim);
	case D185_GRAIN_EFFECT_AND_SIZE: {
		// this binary property is composed of 2 XML properties
		int rc;
		uint32_t value_size;
		rc = copy_value_if_valid(fp->GrainEffect, value, fp_grain_effect);
		rc |= copy_value_if_valid(fp->GrainEffectSize, &value_size, fp_grain_effect_size);
		*value += value_size;
		return rc;
	};
	case D185_CHROME_EFFECT:
		return copy_value_if_valid(fp->ChromeEffect, value, fp_chrome_effect);
	case D185_WHITE_BALANCE_SHOOT_COND:
		return copy_value_if_valid(fp->WBShootCond, value, fp_white_balance_shoot_cond);
	case D185_WHITE_BALANCE_MODE:
		return copy_value_if_valid(fp->WhiteBalance, value, fp_white_balance_mode);
	case D185_WHITE_BALANCE_SHIFT_RED_GREEN:
		return copy_value_if_valid(fp->WBShiftR, value, fp_range_wb_shift);
	case D185_WHITE_BALANCE_SHIFT_BLUE_YELLOW:
		return copy_value_if_valid(fp->WBShiftB, value, fp_range_wb_shift);
	case D185_WHITE_BALANCE_COLOR_TEMP:
		return copy_value_if_valid(fp->WBColorTemp, value, fp_color_temp);
	case D185_HIGHLIGHT_TONE:
		return copy_value_if_valid(fp->HighlightTone, value, fp_get_highlight_tone(fp));
	case D185_SHADOW_TONE:
		return copy_value_if_valid(fp->ShadowTone, value, fp_get_shadow_tone(fp));
	case D185_COLOR:
		return copy_value_if_valid(fp->Color, value, fp_range);
	case D185_SHARPNESS:
		return copy_value_if_valid(fp->Sharpness, value, fp_range);
	case D185_NOISE_REDUCTION:
		return copy_value_if_valid(fp->NoisReduction, value, fp_noise_reduction);
	case D185_LENS_MODULATION_OPTIMIZATION:
		return copy_value_if_valid(fp->LensModulationOpt, value, fp_lens_modulation);
	case D185_COLOR_SPACE:
		return copy_value_if_valid(fp->ColorSpace, value, fp_color_space);
	case D185_BLACK_IMAGE_TONE:
		return copy_value_if_valid(fp->BlackImageTone, value, fp_range_mono_shift_warm_cold);
	case D185_UNKNOWN_24:
		// TODO, unknown
		(*value) = 0;
		return 0;
	case D185_COLOR_CHROME_BLUE:
		return copy_value_if_valid(fp->ColorChromeBlue, value, fp_color_chrome_blue);
	case D185_MONOCHROMATIC_COLOR_RG:
		return copy_value_if_valid(fp->MonochromaticColor_RG, value, fp_range_mono_green_magenta);
	case D185_CLARITY:
		return copy_value_if_valid(fp->Clarity, value, fp_clarity);
	case D185_UNKNOWN_28:
		// TODO, unknown
		(*value) = 0;
		return 0;
	case D185_UNKNOWN_29:
		// TODO, unknown
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
	fp->StructVer = 0x10000;

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
			fp_set_error("Error parsing property of value %d at index %d", value, i);
			return -1;
		}
	}

	return 0;
}
