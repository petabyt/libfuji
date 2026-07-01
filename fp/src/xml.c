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
		fp_set_error("Invalid value %x for %s\n", value, label);
		return -1;
	}
}

int fp_dump_struct(FILE *f, struct FujiProfile *fp) {
	int rc = 0;
	fprintf(f, "  <%s>%04X</%s>\n", "IOPCode", fp->IOPCode, "IOPCode");
	rc |= dump_property(f, "TetherRAWConditonCode", fp->TetherRAWConditonCode, NULL);
	rc |= dump_property(f, "Editable", fp->Editable, NULL);
	rc |= dump_property(f, "ShootingCondition", fp->ShootingCondition, NULL);
	rc |= dump_property(f, "FileType", fp->FileType, NULL);
	rc |= dump_property(f, "ImageSize", fp->ImageSize, NULL);
	rc |= dump_property(f, "RotationAngle", fp->RotationAngle, NULL);
	rc |= dump_property(f, "ImageQuality", fp->ImageQuality, NULL);
	rc |= dump_property(f, "ExposureBias", fp->ExposureBias, NULL);
	rc |= dump_property(f, "DynamicRange", fp->DynamicRange, NULL);
	rc |= dump_property(f, "WideDRange", fp->WideDRange, NULL);
	rc |= dump_property(f, "FilmSimulation", fp->FilmSimulation, fp_film_sim);
	rc |= dump_property(f, "BlackImageTone", fp->BlackImageTone, NULL);
	rc |= dump_property(f, "MonochromaticColor_RG", fp->MonochromaticColor_RG, NULL);
	rc |= dump_property(f, "GrainEffect", fp->GrainEffect, fp_grain_effect);
	rc |= dump_property(f, "GrainEffectSize", fp->GrainEffectSize, NULL);
	rc |= dump_property(f, "ChromeEffect", fp->ChromeEffect, NULL);
	rc |= dump_property(f, "ColorChromeBlue", fp->ColorChromeBlue, NULL);
	rc |= dump_property(f, "SmoothSkinEffect", fp->SmoothSkinEffect, NULL);
	rc |= dump_property(f, "WBShootCond", fp->WBShootCond, NULL);
	rc |= dump_property(f, "WhiteBalance", fp->WhiteBalance, NULL);
	rc |= dump_property(f, "WBShiftR", fp->WBShiftR, NULL);
	rc |= dump_property(f, "WBShiftB", fp->WBShiftB, NULL);
	rc |= dump_property(f, "WBColorTemp", fp->WBColorTemp, NULL);
	rc |= dump_property(f, "HighlightTone", fp->HighlightTone, NULL);
	rc |= dump_property(f, "ShadowTone", fp->ShadowTone, NULL);
	rc |= dump_property(f, "Color", fp->Color, NULL);
	rc |= dump_property(f, "Sharpness", fp->Sharpness, NULL);
	rc |= dump_property(f, "NoisReduction", fp->NoisReduction, NULL);
	rc |= dump_property(f, "Clarity", fp->Clarity, NULL);
	rc |= dump_property(f, "LensModulationOpt", fp->LensModulationOpt, NULL);
	rc |= dump_property(f, "ColorSpace", fp->ColorSpace, NULL);
	rc |= dump_property(f, "HDR", fp->HDR, NULL);
	return rc;
}

static int validate_lookup(const char *str, uint32_t *out, struct FujiLookup *tbl) {
	if (str == NULL) {
		fp_set_error("validate_lookup has to handle NULL value\n");
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
		if (value == NULL) return -1;
		// Should always be 0x10000
		if (!strcmp(value, "65536")) {
			return 0;
		}
		return -1;
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
		return validate_lookup(value, &fp->WhiteBalance, fp_white_balance);
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
		if (value != NULL) return -1;
		fp->HDR = FP_TRUE;
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

	return 0;
}
