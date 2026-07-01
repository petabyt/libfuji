// Copyright 2025 Daniel C
#include <stdio.h>
#include <string.h>
#ifdef FP_LIBXML2
#include <libxml/parser.h>
#endif
#include "ezxml.h"
#include <stdlib.h>
#include "fp.h"

static int dump_property(FILE *f, const char *label, uint32_t value, struct FujiLookup *tbl) {
	if (tbl == NULL) {
		fprintf(f, "  <%s>%d</%s>\n", label, value, label);
		return 0;
	} else {
		for (int i = 0; tbl[i].key != 0; i++) {
			if (tbl[i].value == value) {
				fprintf(f, "  <%s>%s</%s>\n", label, tbl[i].key, label);
				return 0;
			}
		}
		fp_set_error("Invalid value %x for %s", value, label);
		return -1;
	}
}

int fp_dump_struct(FILE *f, struct FujiProfile *fp) {
	int rc = 0;
	fprintf(f, "  <%s>%04X</%s>\n", "IOPCode", fp->IOPCode, "IOPCode");
	rc |= dump_property(f, "StructVer", fp->StructVer, fp_struct_ver);
	rc |= dump_property(f, "IOPCode", fp->IOPCode, NULL);
	rc |= dump_property(f, "TetherRAWConditonCode", fp->TetherRAWConditonCode, NULL);
	rc |= dump_property(f, "Editable", fp->Editable, fp_bool);
	rc |= dump_property(f, "ShootingCondition", fp->ShootingCondition, NULL);
	rc |= dump_property(f, "FileType", fp->FileType, fp_file_type);
	rc |= dump_property(f, "ImageSize", fp->ImageSize, fp_image_size);
	rc |= dump_property(f, "RotationAngle", fp->RotationAngle, NULL);
	rc |= dump_property(f, "ImageQuality", fp->ImageQuality, fp_image_quality);
	rc |= dump_property(f, "ExposureBias", fp->ExposureBias, fp_exposure_bias);
	rc |= dump_property(f, "DynamicRange", fp->DynamicRange, fp_drange);
	rc |= dump_property(f, "WideDRange", fp->WideDRange, fp_drange_priority);
	rc |= dump_property(f, "FilmSimulation", fp->FilmSimulation, fp_film_sim);
	rc |= dump_property(f, "BlackImageTone", fp->BlackImageTone, fp_range_mono_shift_warm_cold);
	rc |= dump_property(f, "MonochromaticColor_RG", fp->MonochromaticColor_RG, fp_range_mono_green_magenta);
	rc |= dump_property(f, "GrainEffect", fp->GrainEffect, fp_grain_effect);
	rc |= dump_property(f, "GrainEffectSize", fp->GrainEffectSize, fp_grain_effect_size);
	rc |= dump_property(f, "ChromeEffect", fp->ChromeEffect, fp_chrome_effect);
	rc |= dump_property(f, "ColorChromeBlue", fp->ColorChromeBlue, fp_color_chrome_blue);
	rc |= dump_property(f, "SmoothSkinEffect", fp->SmoothSkinEffect, fp_on_off);
	rc |= dump_property(f, "WBShootCond", fp->WBShootCond, fp_white_balance_shoot_cond);
	rc |= dump_property(f, "WhiteBalance", fp->WhiteBalance, fp_white_balance_mode);
	rc |= dump_property(f, "WBShiftR", fp->WBShiftR, fp_range_wb_shift);
	rc |= dump_property(f, "WBShiftB", fp->WBShiftB, fp_range_wb_shift);
	rc |= dump_property(f, "WBColorTemp", fp->WBColorTemp, fp_color_temp);
	rc |= dump_property(f, "HighlightTone", fp->HighlightTone, fp_get_highlight_tone(fp));
	rc |= dump_property(f, "ShadowTone", fp->ShadowTone, fp_get_shadow_tone(fp));
	rc |= dump_property(f, "Color", fp->Color, fp_range);
	rc |= dump_property(f, "Sharpness", fp->Sharpness, fp_range);
	rc |= dump_property(f, "NoisReduction", fp->NoisReduction, fp_noise_reduction);
	rc |= dump_property(f, "Clarity", fp->Clarity, fp_clarity);
	rc |= dump_property(f, "LensModulationOpt", fp->LensModulationOpt, fp_lens_modulation);
	rc |= dump_property(f, "ColorSpace", fp->ColorSpace, fp_color_space);
	rc |= dump_property(f, "HDR", fp->HDR, fp_hdr);
	rc |= dump_property(f, "DigitalTeleConv", fp->DigitalTeleConv, fp_on_off);
	rc |= dump_property(f, "PortraitEnhancer", fp->PortraitEnhancer, fp_on_off);
	return rc;
}

static int validate_lookup(const char *str, uint32_t *out, struct FujiLookup *tbl) {
	if (str == NULL) {
		fp_set_error("validate_lookup has to handle NULL value");
		return -1;
	}

	for (int i = 0; tbl[i].key != NULL; i++) {
		if (!strcmp(str, tbl[i].key)) {
			(*out) = tbl[i].value;
			return 0;
		}
	}
	return -1;
}

static int parse_prop(struct FujiProfile *fp, const char *key, const char *value) {
	if (!strcmp(key, "SerialNumber")) {
		// Nothing to do with this it seems like
	} else if (!strcmp(key, "Editable")) {
		return validate_lookup(value, &fp->Editable, fp_bool);
	} else if (!strcmp(key, "IOPCode")) {
		if (value == NULL) return -1;
		if (strlen(value) != 8) return -1;
		fp->IOPCode = strtoul(value, NULL, 16);
	} else if (!strcmp(key, "StructVer")) {
        // Should always be 0x10000
        return validate_lookup(value, &fp->StructVer, fp_struct_ver);
	} else if (!strcmp(key, "FileType")) {
		return validate_lookup(value, &fp->FileType, fp_file_type);
	} else if (!strcmp(key, "ImageSize")) {
		return validate_lookup(value, &fp->ImageSize, fp_image_size);
	} else if (!strcmp(key, "ImageQuality")) {
		return validate_lookup(value, &fp->ImageQuality, fp_image_quality);
	} else if (!strcmp(key, "ExposureBias")) {
		return validate_lookup(value, &fp->ExposureBias, fp_exposure_bias);
	} else if (!strcmp(key, "ChromeEffect")) {
		return validate_lookup(value, &fp->ChromeEffect, fp_chrome_effect);
	} else if (!strcmp(key, "WhiteBalance")) {
		return validate_lookup(value, &fp->WhiteBalance, fp_white_balance_mode);
	} else if (!strcmp(key, "WBColorTemp")) {
		return validate_lookup(value, &fp->WBColorTemp, fp_color_temp);
	} else if (!strcmp(key, "WBShiftR")) {
		if (value == NULL) return -1;
		int val = (int)strtoul(value, NULL, 0);
		if (!(val >= -10 && val <= 10)) return -1;
		fp->WBShiftR = val;
	} else if (!strcmp(key, "WBShiftB")) {
		if (value == NULL) return -1;
		int val = (int)strtoul(value, NULL, 0);
		if (!(val >= -10 && val <= 10)) return -1;
		fp->WBShiftR = val;
	} else if (!strcmp(key, "HighlightTone")) {
		return validate_lookup(value, &fp->HighlightTone, fp_get_highlight_tone(fp));
	} else if (!strcmp(key, "ShadowTone")) {
		return validate_lookup(value, &fp->ShadowTone, fp_get_shadow_tone(fp));
	} else if (!strcmp(key, "Color")) {
		return validate_lookup(value, &fp->Color, fp_range);
	} else if (!strcmp(key, "Sharpness")) {
		return validate_lookup(value, &fp->Sharpness, fp_range);
	} else if (!strcmp(key, "NoisReduction")) {
		return validate_lookup(value, &fp->NoisReduction, fp_noise_reduction);
	} else if (!strcmp(key, "Clarity")) {
		return validate_lookup(value, &fp->Clarity, fp_clarity);
	} else if (!strcmp(key, "TetherRAWConditonCode")) {
		// TODO
	} else if (!strcmp(key, "SourceFileName")) {
		// TODO
	} else if (!strcmp(key, "Fileerror")) {
		// TODO
	} else if (!strcmp(key, "ShootingCondition")) {
		// TODO
	} else if (!strcmp(key, "FilmSimulation")) {
		return validate_lookup(value, &fp->FilmSimulation, fp_film_sim);
	} else if (!strcmp(key, "RotationAngle")) {
		if (value == NULL) return -1;
		fp->RotationAngle = (int)strtoul(value, NULL, 0);
	} else if (!strcmp(key, "DynamicRange")) {
		if (value == NULL) return -1;
		fp->DynamicRange = (int)strtoul(value, NULL, 0);
		if (fp->DynamicRange != 100 && fp->DynamicRange != 200 && fp->DynamicRange != 400) {
			return -1;
		}
	} else if (!strcmp(key, "WideDRange")) {
		if (value == NULL) return -1;
		uint32_t v = (int)strtoul(value, NULL, 0);
		if (!(v <= 100)) {
			return -1;
		}
		fp->WideDRange = v;
	} else if (!strcmp(key, "BlackImageTone")) {
		if (value == NULL) return -1;
		fp->BlackImageTone = (int)strtoul(value, NULL, 0);
	} else if (!strcmp(key, "MonochromaticColor_RG")) {
		if (value == NULL) return -1;
		fp->MonochromaticColor_RG = (int)strtoul(value, NULL, 0); // TODO Was gonna do sometyhing here
	} else if (!strcmp(key, "MonochromaticColor_RG")) {
		if (value == NULL) return -1;
		fp->MonochromaticColor_RG = (int)strtoul(value, NULL, 0);
	} else if (!strcmp(key, "GrainEffect")) {
		return validate_lookup(value, &fp->GrainEffect, fp_grain_effect);
	} else if (!strcmp(key, "GrainEffectSize")) {
		return validate_lookup(value, &fp->GrainEffectSize, fp_grain_effect_size);
	} else if (!strcmp(key, "ColorChromeBlue")) {
		if (value == NULL) {
			fp->ColorChromeBlue = 0;
			return 0;
		} else {
			return validate_lookup(value, &fp->ColorChromeBlue, fp_color_chrome_blue);
		}
	} else if (!strcmp(key, "HDR")) {
		if (value == NULL) {
			fp->HDR = FP_OFF;
			return 0;
		}
		return validate_lookup(value, &fp->HDR, fp_hdr);
	} else if (!strcmp(key, "SmoothSkinEffect")) {
		// Observed as
		// <SmoothSkinEffect/>
		// and
		// <SmoothSkinEffect>OFF</SmoothSkinEffect>
		// so far
		if (value == NULL) { fp->SmoothSkinEffect = FP_ON; return 0; }
		return validate_lookup(value, &fp->WBShootCond, fp_on_off);
	} else if (!strcmp(key, "WBShootCond")) {
		return validate_lookup(value, &fp->WBShootCond, fp_on_off);
	} else if (!strcmp(key, "LensModulationOpt")) {
		return validate_lookup(value, &fp->WBShootCond, fp_on_off);
	} else if (!strcmp(key, "ColorSpace")) {
		return validate_lookup(value, &fp->ColorSpace, fp_color_space);
	} else if (!strcmp(key, "DigitalTeleConv")) {
		if (value == NULL) { fp->DigitalTeleConv = FP_ON; return 0; }
		return validate_lookup(value, &fp->DigitalTeleConv, fp_on_off);
	} else if (!strcmp(key, "PortraitEnhancer")) {
		if (value == NULL) { fp->PortraitEnhancer = FP_ON; return 0; }
	} else {
		printf("TODO: %s\n", key);
		return -1;
	}
	return 0;
}

/// @brief Enforces rules about overlapping settings
static int sanitize_parsed_conv_profile(struct FujiProfile* fp) {
	if (fp == NULL) return -1;

	// sanitize white balance properties
	// white balance precedence: WBShootCond, WhiteBalance, WBColorTemp
	if (fp->WBShootCond == FB_WB_SHOOT_COND_ON) {
		// this means "AS SHOT WB", which ignores any conv profile settings 
		fp->WhiteBalance = FP_WB_Invalid;
		fp->WBColorTemp = 0;
		fp->WBShiftR = 0;
		fp->WBShiftB = 0;
	}

	// sanitize dynamic range properties
	// for HDR photos, the HDR tag value is put into existing DynamicRange property
	switch(fp->HDR) {
	case FP_HDR_800_PLUS:
		// expect WideDRange to be P3 for 800PLUS
		if (fp->WideDRange != FP_DRANGE_PRIO_HDR800PLUS) {
			fp_set_error("To set HDR tag value to \"800 PLUS\" WideDRange needs to be FP_DRANGE_PRIO_HDR800PLUS");
			return -1;
		}
		fp->DynamicRange = FP_HDR_800;
		break;

	case FP_HDR_800:
	case FP_HDR_400:
	case FP_HDR_200:
		fp->DynamicRange = fp->HDR;
		break;
	}
	
	// sanitize image type properties
	switch (fp->FileType) {
	case FP_TIFF_8_BIT:
	case FP_TIFF_16_BIT:
		fp->ImageQuality = 0;
		fp->ImageSize = 0;
		break;
	}

	// sanitize grain size properties
	if (fp->GrainEffect == FP_GRAIN_OFF) {
		fp->GrainEffectSize = 0;
	}

	// sanitize color properties
	switch (fp->FilmSimulation) {
	case FP_AcrosSTD:
	case FP_AcrosYe:
	case FP_AcrosR:
	case FP_AcrosG:
	case FP_Monochrome:
	case FP_MonochromeYe:
	case FP_MonochromeR:
	case FP_MonochromeG:
		// monochrome film simulation
		fp->Color = 0;
		break;

	case FP_Sepia:
	// neither color nor monochrome
		fp->Color = 0;
		fp->MonochromaticColor_RG = 0;
		fp->BlackImageTone = 0;
		break;

	default:
		// color film simulation
		fp->MonochromaticColor_RG = 0;
		fp->BlackImageTone = 0;
		break;
	}
	return 0;
}

int fp_parse_fp1(const char *path, struct FujiProfile *fp) {
	memset(fp, 0, sizeof(struct FujiProfile));
	ezxml_t xml = ezxml_parse_file(path);

	if (strcmp(xml->name, "ConversionProfile") != 0) {
		fp_set_error("Root XML node is not ConversionProfile");
		return -1;
	}

	const char *application = ezxml_attr(xml, "application");
	if (application == NULL) return -1;
	if (strcmp(application, "XRFC") != 0) return -1;

	const char *version = ezxml_attr(xml, "version");
	if (version == NULL) return -1;

	if (!strcmp(version, "1.10.0.0") || !strcmp(version, "1.11.0.0")) {
		fp->profile_version = FP_FP1_VER;
	} else if (!strcmp(version, "1.12.0.0")) {
		fp->profile_version = FP_FP2_VER;
	} else {
		fp_set_error("Profile version '%s' not supported.\n", version);
		return -1;
	}

	ezxml_t group = xml->child;
	if (group == NULL) return -1;
	if (strcmp(group->name, "PropertyGroup") != 0) {
		fp_set_error("Expected node 'PropertyGroup', not '%s'\n", group->name);
		return -1;
	}

	ezxml_t prop_group = group->child;
	if (prop_group == NULL) return -1;

	for (ezxml_t prop = prop_group; prop != NULL; prop = prop->sibling) {
		printf("Prop: %s\n", prop->name); 

		if (!strcmp(prop->name, "RejectedValue")) {
			printf("Skipping %s\n", prop->name);
		} else {
			const char *name = prop->name;
			const char *value = prop->txt;
			if (prop->flags & EZXML_SELFCLOSING) value = NULL;
			int rc = parse_prop(fp, name, value);
			if (rc) {
				printf("Error parsing prop '%s' = '%s'\n", name, value);
				return rc;
			}
		}
	}

	sanitize_parsed_conv_profile(fp);

	return 0;
}
