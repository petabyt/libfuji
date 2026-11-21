/** \file */ 
// PTP data structures. Byte-wise, most of them are very similar to the spec, but
// aren't actually compatible (they aren't packed). Don't try and memcpy into them.
#ifndef CL_DATA_H
#define CL_DATA_H

/// @defgroup Data Data structures
/// @brief Functions to pack/unpack PTP data structures
/// @addtogroup Data
/// @{

// To store unpacked device info data, after parsing
struct PtpDeviceInfo {
	uint16_t standard_version;
	uint32_t vendor_ext_id;
	uint16_t version;
	char extensions[128];
	uint16_t functional_mode;

	int ops_supported_length;
	uint16_t ops_supported[256];

	int events_supported_length;
	uint16_t events_supported[256];

	int props_supported_length;
	uint16_t props_supported[512];

	int capture_formats_length;
	uint16_t capture_formats[32];

	int playback_formats_length;
	uint16_t playback_formats[32];

	char manufacturer[128];
	char model[128];
	char device_version[64];
	char serial_number[128];
};

struct PtpStorageInfo {
	uint16_t storage_type;
	uint16_t fs_type;
	uint16_t access_capability;
	uint64_t max_capacity;
	uint64_t free_space;
	uint32_t free_objects;
	char storage_desc[128];
	char volume_identifier[128];
};

struct PtpObjectInfo {
	uint32_t storage_id;
	uint16_t obj_format;
	uint16_t protection;
	uint32_t compressed_size;
	uint16_t thumb_format;
	uint32_t thumb_compressed_size;
	uint32_t thumb_width;
	uint32_t thumb_height;
	uint32_t img_width;
	uint32_t img_height;
	uint32_t img_bit_depth;
	uint32_t parent_obj;
	uint16_t assoc_type; // association
	uint32_t assoc_desc;
	uint32_t sequence_num;

	char filename[64];
	char date_created[32];
	char date_modified[32];
	char keywords[64];
};

struct PtpEnumerationForm {
	uint16_t length;
	uint8_t data[];
};

struct PtpRangeForm {
	int min;
	int max;
	int step;
};

struct PtpPropDesc {
	uint16_t code;
	uint16_t data_type;
	uint8_t read_only; // (get/set)

	// uint32_t variables will be used for all types <= 4 bytes
	uint32_t default_value32;
	uint32_t current_value32;
	void *current_value;
	void *default_value;

	uint8_t form_type;
	struct PtpRangeForm range_form;
	struct PtpEnumerationForm *enum_form;
};

struct PtpObjPropDesc {
	uint32_t property_code;
	uint32_t data_type;
	uint8_t get_set;
	uint32_t default_value;
	uint32_t group_code;
	uint32_t form_flag;
	// mystery data type follows if form_flag == 0
};

struct PtpCanonEvent {
	int type;
	int code;
	int value;
	int def;
};

struct PtpEOSViewFinderData {
	uint32_t length;
	uint32_t type;
	// standard JPG follows
};

struct PtpMlLvInfo {
	uint32_t lv_pitch;
	uint32_t lv_width;
	uint32_t lcd_palette[256];
};

struct PtpMlLvHeader {
	uint8_t digic;
	uint8_t v1;
	uint8_t v2;
	uint8_t v3;
};

enum PtpMlBmpLvOption {
	PTP_ML_BMP_LV_GET_FRAME = 0,
	PTP_ML_BMP_LV_GET_SPEC = 1,
};

enum PtpCHDKCommands {
	PTP_CHDK_Version = 0,
	PTP_CHDK_UploadFile = 5,
};

int ptp_pack_object_info(struct PtpRuntime *r, struct PtpObjectInfo *oi, uint8_t *buf, unsigned int max);

int ptp_parse_prop_value(struct PtpRuntime *r);
int ptp_parse_device_info(struct PtpRuntime *r, struct PtpDeviceInfo *di);
int ptp_device_info_json(const struct PtpDeviceInfo *di, char *buffer, unsigned int max);
int ptp_parse_prop_desc(struct PtpRuntime *r, struct PtpPropDesc *oi);
int ptp_prop_desc_json(const struct PtpPropDesc *pd, char *buffer, unsigned int max);
int ptp_parse_object_info(struct PtpRuntime *r, struct PtpObjectInfo *oi);
int ptp_storage_info_json(const struct PtpStorageInfo *so, char *buffer, unsigned int max);
int ptp_object_info_json(const struct PtpObjectInfo *so, char *buffer, unsigned int max);
int ptp_parse_storage_info(struct PtpRuntime *r, struct PtpStorageInfo *si);
__attribute__((deprecated)) int ptp_eos_events(struct PtpRuntime *r, struct PtpGenericEvent **p);
/// @brif Open a stream reader to read the data currently in r->data from ptp_eos_get_event
/// @note Not thread safe
int ptp_eos_events_open(struct PtpRuntime *r, struct PtpEventReader *reader);
/// @returns nonzero for end of stream
/// @note Not thread safe
int ptp_eos_events_next(struct PtpRuntime *r, struct PtpEventReader *reader, struct PtpGenericEvent *event);
/// @brief Reads event list currently in r->data and converts to JSON
int ptp_eos_events_json(struct PtpRuntime *r, char *buffer, unsigned int max);

// Standard property value converters (conv.c)
int ptp_eos_get_shutter(int data, int *out, int dir);
int ptp_eos_get_iso(int data, int *out, int dir);
int ptp_eos_get_aperture(int data, int *out, int dir);
int ptp_eos_get_white_balance(int data, int *out, int dir);
int ptp_eos_get_imgformat_value(uint32_t data[5], int *out);

void *ptp_pack_chdk_upload_file(struct PtpRuntime *r, char *in, char *out, unsigned int *length);

void *canon_evproc_pack(struct PtpRuntime *r, int *out_length, char *string);

#endif
