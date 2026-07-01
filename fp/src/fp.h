/// @file
#ifndef FUJI_FP_H
#define FUJI_FP_H

#include <stdint.h>
#include <stdio.h>

#define FP_FP1_VER 1
#define FP_FP2_VER 2

enum FujiFileType {
	FP_JPG = 7,
	FP_TIFF_8_BIT = 9,
	FP_TIFF_16_BIT = 11,
};

enum FujiImageSize {
	FP_L3x2 = 0x7,
	FP_L16x9 = 0x8,
	FP_L1x1 = 0x9,
	FP_M3x2 = 0x4,
	FP_M16x9 = 0x5,
	FP_M1x1 = 0x6,
	FP_S3x2 = 0x1,
	FP_S16x9 = 0x2,
	FP_S1x1 = 0x3,
};

enum FujiImageQuality {
	FP_FINE = 0x2,
	FP_NORMAL = 0x3,
};

enum FujiExposureBias {
	FP_PLUS_3_EV = 3000,
	FP_PLUS_2_2_3_EV = 2667,
	FP_PLUS_2_1_3_EV = 2333,
	FP_PLUS_2_EV = 2000,
	FP_PLUS_1_2_3_EV = 1667,
	FP_PLUS_1_1_3_EV = 1333,
	FP_PLUS_1_EV = 1000,
	FP_PLUS_2_3_EV = 667,
	FP_PLUS_1_3_EV = 333,
	FP_ZERO_EV = 0x0,
	FP_MIN_1_3_EV = (int)0xfffffeb3,
	FP_MIN_2_3_EV = (int)0xfffffd65,
	FP_MIN_1_EV = (int)0xfffffc18,
	FP_MIN_1_1_3_EV = (int)0xfffffacb,
	FP_MIN_1_2_3_EV = (int)0xfffff97d,
	FP_MIN_2_EV = (int)0xfffff830,
	FP_MIN_2_1_3_EV = (int)0xfffff6e3,
	FP_MIN_2_2_3_EV = (int)0xfffff595,
	FP_MIN_3_EV = (int)0xfffff448,
};

enum FujiSim {
	FP_Provia = 1,
	FP_Velvia = 2,
	FP_Astia = 3,
	FP_PRONegHi = 4,
	FP_ProNegStd = 5,
	FP_Monochrome = 6,
	FP_MonochromeYe = 7,
	FP_MonochromeR = 8,
	FP_MonochromeG = 9,
	FP_Sepia = 10,
	FP_ClassicChrome = 11,
	FP_AcrosSTD = 12,
	FP_AcrosYe = 13,
	FP_AcrosR = 14,
	FP_AcrosG = 15,
	FP_Eterna = 16,
	FP_ClassicNegative = 17,
};

enum FujiRange {
	FP_PLUS_18 = 180,
	FP_PLUS_17 = 170,
	FP_PLUS_16 = 160,
	FP_PLUS_15 = 150,
	FP_PLUS_14 = 140,
	FP_PLUS_13 = 130,
	FP_PLUS_12 = 120,
	FP_PLUS_11 = 110,
	FP_PLUS_10 = 100,
	FP_PLUS_9 = 90,
	FP_PLUS_8 = 80,
	FP_PLUS_7 = 70,
	FP_PLUS_6 = 60,
	FP_PLUS_5 = 50,
	FP_PLUS_4 = 40,
	FP_PLUS_3P5 = 35,
	FP_PLUS_3 = 30,
	FP_PLUS_2P5 = 25,
	FP_PLUS_2 = 20,
	FP_PLUS_1P5 = 15,
	FP_PLUS_1 = 10,
	FP_PLUS_P5 = 5,
	FP_ZERO = 0,
	FP_MIN_P5 = (int32_t)-FP_PLUS_P5,
	FP_MIN_1 = (int32_t)-FP_PLUS_1,
	FP_MIN_1P5 = (int32_t)-FP_PLUS_1P5,
	FP_MIN_2 = (int32_t)-FP_PLUS_2,
	FP_MIN_2P5 = (int32_t)-FP_PLUS_2P5,
	FP_MIN_3 = (int32_t)-FP_PLUS_3,
	FP_MIN_3P5 = (int32_t)-FP_PLUS_3P5,
	FP_MIN_4 = (int32_t)-FP_PLUS_4,
	FP_MIN_5 = (int32_t)-FP_PLUS_5,
	FP_MIN_6 = (int32_t)-FP_PLUS_6,
	FP_MIN_7 = (int32_t)-FP_PLUS_7,
	FP_MIN_8 = (int32_t)-FP_PLUS_8,
	FP_MIN_9 = (int32_t)-FP_PLUS_9,
	FP_MIN_10 = (int32_t)-FP_PLUS_10,
	FP_MIN_11 = (int32_t)-FP_PLUS_11,
	FP_MIN_12 = (int32_t)-FP_PLUS_12,
	FP_MIN_13 = (int32_t)-FP_PLUS_13,
	FP_MIN_14 = (int32_t)-FP_PLUS_14,
	FP_MIN_15 = (int32_t)-FP_PLUS_15,
	FP_MIN_16 = (int32_t)-FP_PLUS_16,
	FP_MIN_17 = (int32_t)-FP_PLUS_17,
	FP_MIN_18 = (int32_t)-FP_PLUS_18,
};

enum FujiNoiseReduction {
	FP_NR_PLUS_4 = 0x5000,
	FP_NR_PLUS_3 = 0x6000,
	FP_NR_PLUS_2 = 0x0,
	FP_NR_PLUS_1 = 0x1000,
	FP_NR_ZERO = 0x2000,
	FP_NR_MIN_1 = 0x3000,
	FP_NR_MIN_2 = 0x4000,
	FP_NR_MIN_3 = 0x7000,
	FP_NR_MIN_4 = 0x8000,
};

enum FujiOnOff {
	FP_OFF = 0,
	FP_ON = 1,
};

enum FujiShootingCondition {
	FP_SHOOTING_CONIDITION_ON = 1,
	FP_SHOOTING_CONIDITION_OFF = 2,
};

enum FujiBool {
	FP_FALSE = 0,
	FP_TRUE = 1,
};

enum FujiGrainEffect {
	FP_GRAIN_OFF = 1,
	FP_GRAIN_WEAK = 0x2,
	FP_GRAIN_STRONG = 0x3,
};

enum FujiChromeEffect {
	FP_CHROME_OFF = 0x1,
	FP_CHROME_WEAK = 0x2,
	FP_CHROME_STRONG = 0x3,
};

enum FujiColorChromeBlue {
	FP_COLOR_CHROME_BLUE_OFF = 0x0,
	FP_COLOR_CHROME_BLUE_WEAK = 0x1,
	FP_COLOR_CHROME_BLUE_STRONG = 0x2,
};

enum FujiGrainEffectSize {
	FP_GRAIN_SIZE_SMALL = 0,
	FP_GRAIN_SIZE_LARGE = 2,
};

enum FujiWhiteBalanceShootingConditions {
	FB_WB_SHOOT_COND_ON = 1,
	FB_WB_SHOOT_COND_OFF = 2,
};

enum FujiWhiteBalance {
	FP_WB_Invalid = 0x1,
	FP_WB_Auto = 0x2, // "AUTO"
	FP_WB_Custom1 = 0x8008, // "CUSTOM 1"
	FP_WB_Custom2 = 0x8009, // "CUSTOM 2"
	FP_WB_Custom3 = 0x800a, // "CUSTOM 3"
	FP_WB_Temperature = 0x8007, // "COLOR TEMPERATURE"
	FP_WB_Daylight = 0x4, // "DAYLIGHT"
	FP_WB_Shade = 0x8006, // "SHADE"
	FP_WB_FLUORESCENT_1 = 0x8001, // "FLUORESCENT LIGHT-1"
	FP_WB_FLUORESCENT_2 = 0x8002, // "FLUORESCENT LIGHT-2"
	FP_WB_FLUORESCENT_3 = 0x8003, // "FLUORESCENT LIGHT-3"
	FP_WB_INCANDESCENT = 0x6, // "INCANDESCENT"
	FP_WB_UNDERWATER = 0x8, // "UNDERWATER"
	FP_WB_DUMMY_AsShot = 0, // "AS SHOT WB", but really set with @see FujiWhiteBalanceShootingConditions
};

enum FujiLensModulation {
	FB_LENS_MODULATION_ON = 1,
	FB_LENS_MODULATION_OFF = 2,
};

enum FujiDynamicRange {
	FP_DRANGE_100 = 100,
	FP_DRANGE_200 = 200,
	FP_DRANGE_400 = 400,
};

enum FujiWideDynamicRange {
	FP_DRANGE_PRIO_OFF = 0,
	FP_DRANGE_PRIO_WEAK = 1,
	FP_DRANGE_PRIO_STRONG = 2,
	FP_DRANGE_PRIO_HDR800PLUS = 3,
};

enum FujiHDR {
	FP_HDR_200 = FP_DRANGE_200,
	FP_HDR_400 = FP_DRANGE_400,
	FP_HDR_800 = 800,
	FP_HDR_800_PLUS,
};

struct FujiProfile {
	int profile_version;
	char prop_group_device[64]; // such as X-T3
	char prop_group_version[64]; // such as X-T3_0100
	/// @brief This always appears to be blank
	char SerialNumber[64];
	/// @brief This has always been observed as 65536
	uint32_t StructVer;
	/// @brief 32 bit code that IDs the image processor
	uint32_t IOPCode;
	/// @brief In XML, this will either be blank or be the value of prop_group_version.
	/// If blank, this will be FP_FALSE, if matches prop_group_version, then FP_TRUE
	uint32_t TetherRAWConditonCode;
	/// @brief FP_TRUE or FP_FALSE
	uint32_t Editable;
	uint32_t ShootingCondition;
	uint32_t FileType;
	uint32_t ImageSize;
	uint32_t RotationAngle;
	uint32_t ImageQuality;
	uint32_t ExposureBias;
	uint32_t DynamicRange;
	uint32_t WideDRange;
	uint32_t FilmSimulation;
	uint32_t BlackImageTone;
	uint32_t MonochromaticColor_RG;
	/// @see struct FujiRange
	uint32_t GrainEffect;
	uint32_t GrainEffectSize;
	uint32_t ChromeEffect;
	uint32_t ColorChromeBlue; // no value
	uint32_t SmoothSkinEffect;
	uint32_t WBShootCond;
	uint32_t WhiteBalance;
	uint32_t WBShiftR;
	uint32_t WBShiftB;
	uint32_t WBColorTemp;
	uint32_t HighlightTone;
	uint32_t ShadowTone;
	uint32_t Color;
	uint32_t Sharpness;
	uint32_t NoisReduction;
	uint32_t Clarity;
	uint32_t LensModulationOpt;
	uint32_t ColorSpace;
	uint32_t HDR;
	uint32_t DigitalTeleConv;
	uint32_t PortraitEnhancer;
	// Not entirely sure how to do this
	uint32_t RejectedValue_BlackImageTone;
};

// This is here for reference
struct __attribute__((packed)) FujiBinaryProfile {
	// So far this has been observed as 0x17 or 0x1d and seems to represent the number of properties following iop_codes.
	// The problem is, the camera appears to always send this structure as exactly 601 bytes regardless of n_props.
	// In that case, one would expect n_props to be 0x16, but it puts 0x17 instead.
	// X Raw Studio seems to always write this correctly as 0x1d and sends 629 bytes.
	// When this structure is read back from the camera, it again sends it back as 601 bytes with n_props being 0x1d.
	uint16_t n_props;
	// This seems to hold a series of (or just one) standard MTP strings.
	// The camera also seems to incorrectly format this property, writing the string as length 8 rather than 9.
	// (See MTP spec page 21 - length of a string includes the null terminator)
	char iop_codes[0x1ff];
	// (offset 0x201)
	// These appear to be in the same order as the props in the XML files
	union Props {
		uint32_t values[0x1d];
		// This is the property structure observed on X-H1
		struct Props1 {
			uint32_t prop0; // 0
			uint32_t prop1; // 1
			uint32_t ImageSize; // 2
			uint32_t ImageQuality; // 3
			uint32_t ExposureBias; // 4
			uint32_t DynamicRange; // 5
			uint32_t WideDRange; // 6 (D RangePriority)
			uint32_t FilmSimulation; // 7
			uint32_t GrainEffect; // 8
			uint32_t GrainEffectSize; // 9
			uint32_t WBShootCond; // 10
			uint32_t WhiteBalance; // 11
			uint32_t WBShiftR; // 12
			uint32_t WBShiftB; // 13
			uint32_t WBColorTemp; // 14
			uint32_t HighlightTone; // 15
			uint32_t ShadowTone; // 16
			uint32_t Color; // 17
			uint32_t Sharpness; // 18
			uint32_t NoisReduction; // 19
			uint32_t Clarity; // 20
			uint32_t ColorSpace; // 21
		}props_xh1_temp;
	}props;
};

struct FujiLookup {
	char *key;
	uint32_t value;
};

extern struct FujiLookup fp_film_sim[];
extern struct FujiLookup fp_struct_ver[];
extern struct FujiLookup fp_on_off[];
extern struct FujiLookup fp_shooting_condition[];
extern struct FujiLookup fp_bool[];
extern struct FujiLookup fp_grain_effect[];
extern struct FujiLookup fp_grain_effect_size[];
extern struct FujiLookup fp_chrome_effect[];
extern struct FujiLookup fp_file_type[];
extern struct FujiLookup fp_image_size[];
extern struct FujiLookup fp_image_quality[];
extern struct FujiLookup fp_exposure_bias[];
extern struct FujiLookup fp_white_balance_shoot_cond[];
extern struct FujiLookup fp_white_balance_mode[];
extern struct FujiLookup fp_color_temp[];
extern struct FujiLookup fp_range[];
extern struct FujiLookup fp_range_wb_shift[];
extern struct FujiLookup fp_range_mono_shift_warm_cold[];
extern struct FujiLookup fp_range_mono_green_magenta[];
extern struct FujiLookup fp_range_p4_n2[];
extern struct FujiLookup fp_drange[];
extern struct FujiLookup fp_drange_priority[];
extern struct FujiLookup fp_noise_reduction[];
extern struct FujiLookup fp_clarity[];
extern struct FujiLookup fp_color_chrome_blue[];
extern struct FujiLookup fp_range_p4_n4_halfs[];
extern struct FujiLookup fp_hdr[];
extern struct FujiLookup fp_lens_modulation[];
extern struct FujiLookup fp_color_space[];
struct FujiLookup *fp_get_highlight_tone(const struct FujiProfile *fp);
struct FujiLookup *fp_get_shadow_tone(const struct FujiProfile *fp);

extern char fp_error_str[64];
void fp_set_error(const char *fmt, ...);

/// @brief All functions in this library will (should) fill an error buffer noting what went wrong
/// if an error code was returned.
const char *fp_get_error(void);

/// @brief Parse FP1/FP2/FP3 files
/// @returns non-zero for error
int fp_parse_fp1(const char *path, struct FujiProfile *fp1);

/// @brief Parse a raw binary profile from PTP property 0xd185
/// @returns Non-zero for error
int fp_parse_d185(const uint8_t *bin, int len, struct FujiProfile *fp1);

/// @brief Dump struct as XML text to a file.
/// @returns non-zero for error
int fp_dump_struct(FILE *f, struct FujiProfile *fp);

/// @brief Convert to d185 structure that can be accepted by a camera
/// @returns non-zero for error
int fp_create_d185(const struct FujiProfile *fp, uint8_t *bin, int len);

/// @brief Merge a profile 'from' into 'to'
/// @returns non-zero for error
int fp_apply_profile(const struct FujiProfile *from, struct FujiProfile *to);

#endif
