// Data structure unpacking and packing functions
// Copyright 2022 by Daniel C (https://github.com/petabyt/libpict)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <libpict.h>

// Custom snprint with offset
static int osnprintf(char *str, int cur, unsigned int size, const char *format, ...) {
	if (size - cur < 0) {
		ptp_panic("osnprintf overflow %d/%d", cur, size);
	}

	int r;
	va_list args;
	va_start(args, format);
	r = vsnprintf(str + cur, size - cur, format, args);
	va_end(args);

	return r;
}

static void boundcheck(uint8_t *bs, uint8_t *be, int size) {
	if (be == NULL) return;
	if ((uintptr_t)bs > (uintptr_t)be) ptp_panic("PTP: bs after be\n");
	if (((uintptr_t)be - (uintptr_t)bs) < (uintptr_t)size) {
		ptp_panic("PTP: buffer overflow %p-%p (%u)\n", bs, be, size);
	}
}

// Read standard UTF16 string
int ptp_read_string(uint8_t *d, char *string, int max) {
	int of = 0;
	uint8_t length;
	of += ptp_read_u8(d + of, &length);
	if (length == 0) {
		string[0] = '\0';
		return of;
	}

	uint8_t i = 0;
	uint16_t wchr = 0x0;
	while (i < length) {
		of += ptp_read_u16(d + of, &wchr);
		if (wchr > 128) wchr = '?';
		else if (wchr != '\0' && wchr < 32) wchr = ' ';
		string[i] = (char)wchr;
		i++;
		if (i >= max - 1) break;
	}

	if (wchr != 0x0) {
		fprintf(stderr, "PTP String does not have null terminator!\n");
	} else {
		string[i] = '\0';
	}

	return of;
}

int ptp_read_uint16_array(const uint8_t *dat, uint16_t *buf, int max, int *length) {
	ptp_panic("Unfinished\n");
	int of = 0;

	uint32_t n;
	of += ptp_read_u32(dat + of, &n);

	for (uint32_t i = 0; i < n; i++) {
		if (i >= max) {
			ptp_panic("ptp_read_uint16_array overflow\n");
		} else {
			of += ptp_read_u16(dat + of, &buf[i]);
		}
	}

	return of;
}

int ptp_read_uint16_array_s(uint8_t *bs, uint8_t *be, uint16_t *buf, int max, int *length) {
	int of = 0;
	uint32_t n;
	of += ptp_read_u32(bs + of, &n);
	(*length) = (int)n;
	boundcheck(bs, be, (2 * (int)n) + 4);
	for (int i = 0; i < (int)n; i++) {
		if (i >= max) {
			ptp_panic("ptp_read_uint16_array overflow %i >= %d\n", i, n);
		} else {
			of += ptp_read_u16(bs + of, &buf[i]);
		}
	}
	return of;
}

// Write standard MTP compliant string
int ptp_write_string(uint8_t *dat, const char *string) {
	int of = 0;

	// The length is one byte including the terminator: at most 254 characters.
	size_t full_length = strlen(string);
	uint32_t length = full_length > 254 ? 254 : (uint32_t)full_length;
	if (length == 0) {
		// 'An empty string is represented by a single 8-bit integer containing a value of 0x00.'
		ptp_write_u8(dat, 0x0);
		return 1;
	}
	of += ptp_write_u8(dat + of, length + 1);

	for (int i = 0; i < length; i++) {
		of += ptp_write_u8(dat + of, string[i]);
		of += ptp_write_u8(dat + of, '\0');
	}

	of += ptp_write_u16(dat + of, 0x0);

	return of;
}

// Write normal UTF-8 string
int ptp_write_utf8_string(void *dat, const char *string) {
	char *o = (char *)dat;
	int x = 0;
	while (string[x] != '\0') {
		o[x] = string[x];
		x++;
	}

	o[x] = '\0';
	x++;
	return x;
}

// Write wchar string
int ptp_write_unicode_string(char *dat, const char *string) {
	int i;
	for (i = 0; string[i] != '\0'; i++) {
		dat[i * 2] = string[i];
		dat[i * 2 + 1] = '\0';
	}
	dat[i * 2 + 1] = '\0';
	return i;
}

// Read null-terminated UTF16 string
int ptp_read_unicode_string(char *buffer, const char *dat, int max) {
	int i;
	for (i = 0; dat[i] != '\0'; i += 2) {
		buffer[i / 2] = dat[i];
		if (i >= max - 2) {
			buffer[(i / 2) + 1] = '\0';
			return i;
		}
	}

	buffer[(i / 2)] = '\0';
	return i / 2;
}

int ptp_read_utf8_string(void *dat, char *string, int max) {
	char *d = (char *)dat;
	int x = 0;
	while (d[x] != '\0') {
		string[x] = d[x];
		x++;
		if (x > max - 1) break;
	}

	string[x] = '\0';
	x++;

	return x;
}

int ptp_get_prop_size(uint8_t *d, int type) {
	uint32_t length32;
	uint8_t length8;
	switch (type) {
	case PTP_TC_INT8:
	case PTP_TC_UINT8:
		return 1;
	case PTP_TC_INT16:
	case PTP_TC_UINT16:
		return 2;
	case PTP_TC_INT32:
	case PTP_TC_UINT32:
		return 4;
	case PTP_TC_INT64:
	case PTP_TC_UINT64:
		return 8;
	case PTP_TC_UINT8ARRAY:
		ptp_read_u32(d, &length32);
		return 4 + ((int)length32 * 1);
	case PTP_TC_UINT16ARRAY:
		ptp_read_u32(d, &length32);
		return 4 + ((int)length32 * 2);
	case PTP_TC_UINT32ARRAY:
		ptp_read_u32(d, &length32);
		return 4 + ((int)length32 * 4);
	case PTP_TC_UINT64ARRAY:
		ptp_read_u32(d, &length32);
		return 4 + ((int)length32 * 8);
	case PTP_TC_STRING:
		ptp_read_u8(d, &length8);
		return 1 + ((int)length8 * 2);
	default: return -1;
	}
}

// Cheap dumb function
int ptp_parse_data_u32(void *d, int type, int *out) {
	uint8_t a;
	uint16_t b;
	uint32_t c;
	switch (type) {
	case PTP_TC_INT8:
	case PTP_TC_UINT8:
		ptp_read_u8(d, &a); (*out) = (int)a; return 1;
	case PTP_TC_INT16:
		ptp_read_u16(d, &b); (*out) = (int)((short)b); return 2;
	case PTP_TC_UINT16:
		ptp_read_u16(d, &b); (*out) = (int)b; return 2;
	case PTP_TC_INT32:
	case PTP_TC_UINT32:
		ptp_read_u32(d, &c); (*out) = (int)c; return 4;
	}

	// skip the array
	return ptp_get_prop_size(d, type);
}

int ptp_parse_prop_value(struct PtpRuntime *r) {
	int type;
	switch (ptp_get_payload_length(r)) {
	case 1:
		type = PTP_TC_UINT8; break;
	case 2:
		type = PTP_TC_UINT16; break;
	case 4:
		type = PTP_TC_UINT32; break;
	default:
		ptp_verbose_log(r, "%s: unknown data type size %d - returning -1\n", __func__, ptp_get_payload_length(r));
		return -1;
	}

	int out;
	ptp_parse_data_u32(ptp_get_payload(r), type, &out);

	return out;
}

static int get_arr_item_size(int t) {
	switch (t) {
	case PTP_TC_UINT8ARRAY:
		return 1;
	case PTP_TC_UINT16ARRAY:
		return 2;
	case PTP_TC_UINT32ARRAY:
		return 4;
	case PTP_TC_UINT64ARRAY:
		return 8;
	}
	ptp_panic("Unknown arr type\n");
}

int parse_data_data_or_u32(uint8_t *d, int type, uint32_t *u32, void **data) {
	int size;
	uint32_t length32;
	uint8_t length8;
	int len_total;
	switch (type) {
	case PTP_TC_INT8:
	case PTP_TC_UINT8:
	case PTP_TC_INT16:
	case PTP_TC_UINT16:
	case PTP_TC_INT32:
	case PTP_TC_UINT32:
		return ptp_parse_data_u32(d, type, (int *)u32);
	case PTP_TC_INT64:
	case PTP_TC_UINT64:
		(*data) = malloc(8);
		memcpy((*data), d, 8);
		return 8;
	case PTP_TC_UINT8ARRAY:
	case PTP_TC_UINT16ARRAY:
	case PTP_TC_UINT32ARRAY:
	case PTP_TC_UINT64ARRAY:
		size = get_arr_item_size(type);
		ptp_read_u32(d, &length32);
		(*data) = malloc(4 + (int)length32 * size);
		memcpy((*data), d, 4 + (int)length32 * size);
		return 4 + (int)length32 * size;
	case PTP_TC_STRING:
		ptp_read_u8(d, &length8);
		len_total = 1 + (length8 * 2);
		(*data) = malloc(len_total);
		memcpy((*data), d, len_total);
		return len_total;
	default:
		ptp_panic("Unknown data type %d\n", type);
	}
}

int ptp_parse_prop_desc(struct PtpRuntime *r, struct PtpPropDesc *oi) {
	uint8_t *d = ptp_get_payload(r);

	d += ptp_read_u16(d, &oi->code);
	d += ptp_read_u16(d, &oi->data_type);
	d += ptp_read_u8(d, &oi->read_only);

	d += parse_data_data_or_u32(d, oi->data_type, &oi->default_value32, &oi->default_value);
	d += parse_data_data_or_u32(d, oi->data_type, &oi->current_value32, &oi->current_value);

	d += ptp_read_u8(d, &oi->form_type);

	switch (oi->form_type) {
	case PTP_RangeForm:
		d += ptp_parse_data_u32(d, oi->data_type, &oi->range_form.min);
		d += ptp_parse_data_u32(d, oi->data_type, &oi->range_form.max);
		d += ptp_parse_data_u32(d, oi->data_type, &oi->range_form.step);
		return 0;
	case PTP_EnumerationForm: {
		uint16_t num_values = 0;
		d += ptp_read_u16(d, &num_values);
		int length = 0;
		for (uint32_t i = 0; i < num_values; i++) {
			int rc = ptp_get_prop_size(d + length, oi->data_type);
			if (rc < 0) return -1;
			length += rc;
		}
		struct PtpEnumerationForm *form = (struct PtpEnumerationForm *)malloc(sizeof(struct PtpEnumerationForm) + length);
		form->length = num_values;
		memcpy(form->data, d, length);
		oi->enum_form = form;
		} return 0;
	case PTP_NoForm:
	case PTP_DateTimeForm:
	case PTP_FixedLengthArrayForm:
	case PTP_RegularExpressionForm:
	case PTP_ByteArrayForm:
	case PTP_LongStringForm:
		ptp_panic("Unimplemented form type %d\n", oi->form_type);
		return -1;
	default:
		ptp_panic("Unknown form type %d\n", oi->form_type);
		return -1;
	}
}

int ptp_prop_desc_json(const struct PtpPropDesc *pd, char *buffer, unsigned int max) {
	int curr = osnprintf(buffer, 0, max, "{\n");
	curr += osnprintf(buffer, curr, max, "\"code\": %d,\n", pd->code);
	curr += osnprintf(buffer, curr, max, "\"type\": %d,\n", pd->data_type);

	switch (pd->data_type) {
	case PTP_TC_INT8:
	case PTP_TC_UINT8:
	case PTP_TC_INT16:
	case PTP_TC_UINT16:
	case PTP_TC_INT32:
	case PTP_TC_UINT32:
	case PTP_TC_INT64:
	case PTP_TC_UINT64:
		curr += osnprintf(buffer, curr, max, "\"currentValue32\": %d,\n", pd->current_value32);
		curr += osnprintf(buffer, curr, max, "\"defaultValue32\": %u,\n", pd->default_value32);
		break;
	case PTP_TC_UINT8ARRAY:
	case PTP_TC_UINT16ARRAY:
	case PTP_TC_UINT32ARRAY:
	case PTP_TC_UINT64ARRAY: // TODO:
		ptp_panic("Writeme\n");
		break;
	case PTP_TC_STRING:
		ptp_panic("Writeme\n");
		break;
	}

	if (pd->form_type == PTP_RangeForm) {
		curr += osnprintf(buffer, curr, max, "\"validEntries\": [");
		for (int i = pd->range_form.min; i < pd->range_form.max; i += pd->range_form.step) {
			char *end = ", ";
			if (i >= pd->range_form.max - pd->range_form.step) end = "";
			curr += osnprintf(buffer, curr, max, "%d%s", i, end);
		}
		curr += osnprintf(buffer, curr, max, "],\n");
	} else {
		curr += osnprintf(buffer, curr, max, "\"validEntries\": [");
		int of = 0;
		for (int i = 0; i < pd->enum_form->length; i++) {
			char *end = ", ";
			if (i >= pd->enum_form->length - 1) end = "";
			uint32_t value;
			void *data = NULL;
			of += parse_data_data_or_u32(pd->enum_form->data + of, pd->data_type, &value, &data);
			curr += osnprintf(buffer, curr, max, "%d%s", value, end);
		}
		curr += osnprintf(buffer, curr, max, "],\n");
	}

	curr += osnprintf(buffer, curr, max, "\"form_type\": %d\n", pd->form_type);

	curr += osnprintf(buffer, curr, max, "}");
	return curr;
}

int ptp_parse_object_info(struct PtpRuntime *r, struct PtpObjectInfo *oi) {
	uint8_t *d = ptp_get_payload(r);

	d += ptp_read_u32(d, &oi->storage_id);
	d += ptp_read_u16(d, &oi->obj_format);
	d += ptp_read_u16(d, &oi->protection);
	d += ptp_read_u32(d, &oi->compressed_size);
	d += ptp_read_u16(d, &oi->thumb_format);
	d += ptp_read_u32(d, &oi->thumb_compressed_size);
	d += ptp_read_u32(d, &oi->thumb_width);
	d += ptp_read_u32(d, &oi->thumb_height);
	d += ptp_read_u32(d, &oi->img_width);
	d += ptp_read_u32(d, &oi->img_height);
	d += ptp_read_u32(d, &oi->img_bit_depth);
	d += ptp_read_u32(d, &oi->parent_obj);
	d += ptp_read_u16(d, &oi->assoc_type);
	d += ptp_read_u32(d, &oi->assoc_desc);
	d += ptp_read_u32(d, &oi->sequence_num);

	d += ptp_read_string(d, oi->filename, sizeof(oi->filename));
	d += ptp_read_string(d, oi->date_created, sizeof(oi->date_created));
	d += ptp_read_string(d, oi->date_modified, sizeof(oi->date_modified));
	d += ptp_read_string(d, oi->keywords, sizeof(oi->keywords));

	return 0;
}

int ptp_parse_storage_info(struct PtpRuntime *r, struct PtpStorageInfo *si) {
	if (ptp_get_payload_length(r) < 26) {
		return PTP_RUNTIME_ERR;
	}

	int of = 0;
	uint8_t *b = ptp_get_payload(r);
	of += ptp_read_u16(b + of, &si->storage_type);
	of += ptp_read_u16(b + of, &si->fs_type);
	of += ptp_read_u16(b + of, &si->access_capability);
	of += ptp_read_u64(b + of, &si->max_capacity);
	of += ptp_read_u64(b + of, &si->free_space);
	of += ptp_read_u32(b + of, &si->free_objects);
	of += ptp_read_string(b + of, si->storage_desc, sizeof(si->storage_desc));
	of += ptp_read_string(b + of, si->volume_identifier, sizeof(si->volume_identifier));

	return 0;
}

int ptp_pack_object_info(struct PtpRuntime *r, struct PtpObjectInfo *oi, uint8_t *buf, unsigned int max) {
	if (1024 > max) {
		return 0;
	}

	int of = 0;
	of += ptp_write_u32(buf + of, oi->storage_id);
	of += ptp_write_u16(buf + of, oi->obj_format);
	of += ptp_write_u16(buf + of, oi->protection);
	of += ptp_write_u32(buf + of, oi->compressed_size);
	of += ptp_write_u16(buf + of, oi->thumb_format);
	of += ptp_write_u32(buf + of, oi->thumb_compressed_size);
	of += ptp_write_u32(buf + of, oi->thumb_width);
	of += ptp_write_u32(buf + of, oi->thumb_height);
	of += ptp_write_u32(buf + of, oi->img_width);
	of += ptp_write_u32(buf + of, oi->img_height);
	of += ptp_write_u32(buf + of, oi->img_bit_depth);
	of += ptp_write_u32(buf + of, oi->parent_obj);
	of += ptp_write_u16(buf + of, oi->assoc_type);
	of += ptp_write_u32(buf + of, oi->assoc_desc);
	of += ptp_write_u32(buf + of, oi->sequence_num);
	of += ptp_write_string(buf + of, oi->filename);
	of += ptp_write_string(buf + of, oi->date_created);
	of += ptp_write_string(buf + of, oi->date_modified);
	of += ptp_write_string(buf + of, oi->keywords);
	return of;
}

void *ptp_pack_chdk_upload_file(struct PtpRuntime *r, char *in, char *out, unsigned int *length) {
	FILE *f = fopen(in, "rb");
	if (f == NULL) {
		ptp_verbose_log(r, "Unable to open %s\n", in);
		return NULL;
	}

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

	size_t name_size = strlen(out) + 1;
	if (file_size < 0 || (unsigned long)file_size > UINT32_MAX - 4 - name_size) {
		fclose(f);
		return NULL;
	}
	unsigned int size_all = (unsigned int)(4 + name_size + (size_t)file_size);
	*length = size_all;
	uint8_t *data = malloc(size_all);
	if (data == NULL) return NULL;

	int of = 0;
	of += ptp_write_u32(data + of, (uint32_t)name_size);
	of += ptp_write_utf8_string(data + of, out);

	fread(data + of, 1, file_size, f);

	return data;
}

int ptp_parse_device_info(struct PtpRuntime *r, struct PtpDeviceInfo *di) {
	uint8_t *b = ptp_get_payload(r);
	uint8_t *e = b + r->data_length;

	b += ptp_read_u16(b, &di->standard_version);
	b += ptp_read_u32(b, &di->vendor_ext_id);
	b += ptp_read_u16(b, &di->version);

	b += ptp_read_string(b, di->extensions, sizeof(di->extensions));
	b += ptp_read_u16(b, &di->functional_mode);

	b += ptp_read_uint16_array_s(b, e, di->ops_supported, sizeof(di->ops_supported) / 2, &di->ops_supported_length);
	b += ptp_read_uint16_array_s(b, e, di->events_supported, sizeof(di->events_supported) / 2, &di->events_supported_length);
	b += ptp_read_uint16_array_s(b, e, di->props_supported, sizeof(di->props_supported) / 2, &di->props_supported_length);
	b += ptp_read_uint16_array_s(b, e, di->capture_formats, sizeof(di->capture_formats) / 2, &di->capture_formats_length);
	b += ptp_read_uint16_array_s(b, e, di->playback_formats, sizeof(di->playback_formats) / 2, &di->playback_formats_length);

	b += ptp_read_string(b, di->manufacturer, sizeof(di->manufacturer));	
	b += ptp_read_string(b, di->model, sizeof(di->model));

	b += ptp_read_string(b, di->device_version, sizeof(di->device_version));
	b += ptp_read_string(b, di->serial_number, sizeof(di->serial_number));

	r->di = di; // set last parsed di

	return 0;
}

int ptp_device_info_json(const struct PtpDeviceInfo *di, char *buffer, unsigned int max) {
	int curr = osnprintf(buffer, 0, max, "{\n    \"opsSupported\": [");
	for (int i = 0; i < di->ops_supported_length; i++) {
		char *end = ", ";
		if (i >= di->ops_supported_length - 1) end = "";
		curr += osnprintf(buffer, curr, max, "%d%s", di->ops_supported[i], end);
	}
	curr += osnprintf(buffer, curr, max, "],\n");

	curr += osnprintf(buffer, curr, max, "    \"eventsSupported\": [");
	for (int i = 0; i < di->events_supported_length; i++) {
		char *end = ", ";
		if (i >= di->events_supported_length - 1) end = "";
		curr += osnprintf(buffer, curr, max, "%d%s", di->events_supported[i], end);
	}
	curr += osnprintf(buffer, curr, max, "],\n");

	curr += osnprintf(buffer, curr, max, "    \"propsSupported\": [");
	for (int i = 0; i < di->props_supported_length; i++) {
		char *end = ", ";
		if (i >= di->props_supported_length - 1) end = "";
		curr += osnprintf(buffer, curr, max, "%d%s", di->props_supported[i], end);
	}
	curr += osnprintf(buffer, curr, max, "],\n");

	curr += osnprintf(buffer, curr, max, "    \"manufacturer\": \"%s\",\n", di->manufacturer);
	curr += osnprintf(buffer, curr, max, "    \"extensions\": \"%s\",\n", di->extensions);
	curr += osnprintf(buffer, curr, max, "    \"model\": \"%s\",\n", di->model);
	curr += osnprintf(buffer, curr, max, "    \"deviceVersion\": \"%s\",\n", di->device_version);
	curr += osnprintf(buffer, curr, max, "    \"serialNumber\": \"%s\"\n", di->serial_number);
	curr += osnprintf(buffer, curr, max, "}");
	return curr;
}

static const char *eval_obj_format(int code) {
	char *x = ptp_get_enum_all(code);
	switch (code) {
	case PTP_OF_Association:
		return "folder";
	default:
		if (x) return x;
		return "none";
	}
}

static const char *eval_protection(int code) {
	switch (code) {
	case 0x0:
		return "none";
	case 0x1:
		return "read-only";
	case 0x8002:
		return "read-only data";
	case 0x8003:
		return "non-transferable data";
	default:
		return "reserved";
	}
}

int ptp_object_info_json(const struct PtpObjectInfo *so, char *buffer, unsigned int max) {
	int curr = sprintf(buffer, "{");
	curr += osnprintf(buffer, curr, max, "\"storage_id\": %u,", so->storage_id);
	curr += osnprintf(buffer, curr, max, "\"compressedSize\": %u,", so->compressed_size);
	curr += osnprintf(buffer, curr, max, "\"parent\": %u,", so->parent_obj);
	curr += osnprintf(buffer, curr, max, "\"format\": \"%s\",", eval_obj_format(so->obj_format));
	curr += osnprintf(buffer, curr, max, "\"format_int\": %u,", so->obj_format);
	curr += osnprintf(buffer, curr, max, "\"protection\": \"%s\",", eval_protection(so->protection));
	curr += osnprintf(buffer, curr, max, "\"filename\": \"%s\",", so->filename);
	if (so->compressed_size != 0) {
		curr += osnprintf(buffer, curr, max, "\"imgWidth\": %u,", so->img_width);
		curr += osnprintf(buffer, curr, max, "\"imgHeight\": %u,", so->img_height);
	}
	curr += osnprintf(buffer, curr, max, "\"dateCreated\": \"%s\",", so->date_created);
	curr += osnprintf(buffer, curr, max, "\"dateModified\": \"%s\"", so->date_modified);
	curr += osnprintf(buffer, curr, max, "}");

	return curr;
}

static const char *eval_storage_type(int id) {
	switch (id) {
	case 1:
		return "FixedROM";
	case 2:
		return "RemovableROM";
	case 3:
		return "Internal Storage";
	case 4:
		return "Removable Storage";
	default:
		return "Unknown";
	}
}

int ptp_storage_info_json(const struct PtpStorageInfo *so, char *buffer, unsigned int max) {
	int len = osnprintf(buffer, 0, max, "{");
	len += osnprintf(buffer, len, max, "\"storageType\": \"%s\",", eval_storage_type(so->storage_type));
	len += osnprintf(buffer, len, max, "\"fsType\": %u,", so->fs_type);
	len += osnprintf(buffer, len, max, "\"maxCapacity\": %lu,", so->max_capacity);
	len += osnprintf(buffer, len, max, "\"freeSpace\": %lu", so->free_space);
	len += osnprintf(buffer, len, max, "}");
	return len;
}

int ptp_eos_prop_next(uint8_t *d, struct PtpGenericEvent *p) {
	uint32_t code, value_u32;
	int value;

	int of = 0;
	of += ptp_read_u32(d + of, &code);
	of += ptp_read_u32(d + of, &value_u32);
	value = (int)value_u32;

	const char *name = ptp_get_enum_all((int)code);
	const char *str_value = NULL;
	switch (code) {
	case PTP_DPC_EOS_Aperture:
		if (ptp_eos_get_aperture(value, &value, 0)) break;
		name = "aperture";
		break;
	case PTP_DPC_EOS_ShutterSpeed:
		if (ptp_eos_get_shutter(value, &value, 0)) break;
		name = "shutter speed";
		break;
	case PTP_DPC_EOS_ISOSpeed:
		if (ptp_eos_get_iso(value, &value, 0)) break;
		name = "iso";
		break;
	case PTP_DPC_EOS_BatteryPower:
		// EOS has 3 battery bars
		value++;
		if (value == 3) value = 4;
		name = "battery";
		break;
	case PTP_DPC_EOS_ImageFormat: {
			uint32_t data[5] = {value};
			of += ptp_read_u32(d + of, &data[1]);
			of += ptp_read_u32(d + of, &data[2]);
			of += ptp_read_u32(d + of, &data[3]);
			of += ptp_read_u32(d + of, &data[4]);
			if (value == 1) {
				if (ptp_eos_get_imgformat_value(data, &value)) break;
			} else {
				value = IMG_FORMAT_RAW_JPEG;
			}
			name = "image format";
		} break;
	case PTP_DPC_EOS_VF_Output:
		name = "mirror";
		if (value == 0) {
			str_value = "finder";
		} else {
			str_value = "open";
		}
		break;
	case PTP_DPC_EOS_AEModeDial:
		name = "mode dial";
		break;
	case PTP_DPC_EOS_FocusMode:
		name = "focus mode";
		if (value == 3) {
			str_value = "MF";
		} else {
			str_value = "AF";
		}
		break;
	case PTP_DPC_EOS_WhiteBalance:
		if (ptp_eos_get_white_balance(value, &value, 0)) break;
		name = "white balance";
		break;
	case PTP_DPC_EOS_FocusInfoEx:
		name = "focused";
		break;
	}

	p->code = code;
	if (name == enum_null) {
		p->name = NULL;
	} else {
		p->name = name;
	}

	p->value = value;
	if (str_value == NULL) {
		p->str_value = NULL;
	} else {
		p->str_value = str_value;
	}

	return 0;
}

static int ptp_eos_events_length(struct PtpRuntime *r) {
	uint8_t *dp = ptp_get_payload(r);
	unsigned int of = 0;

	int length = 0;
	while (dp != NULL) {
		if (of >= ptp_get_payload_length(r)) break;
		uint32_t size, type;
		ptp_read_u32(dp + of, &size);
		ptp_read_u32(dp + of + 4, &type);

		of += size;

		// type 0 is termination (or it seems to be so far)
		if (type == 0) break;

		length++;
	}

	return length;
}

int ptp_eos_events_open(struct PtpRuntime *r, struct PtpEventReader *reader) {
	memset(reader, 0, sizeof(struct PtpEventReader));
	reader->n_entries = ptp_eos_events_length(r);
	return 0;
}

int ptp_eos_events_next(struct PtpRuntime *r, struct PtpEventReader *reader, struct PtpGenericEvent *event) {
	if (reader->index >= reader->n_entries) return -1;
	if (reader->data_of >= r->data_length) ptp_panic("overflow");
	memset(event, 0, sizeof(struct PtpGenericEvent));

	uint8_t *d = ptp_get_payload(r);
	unsigned int of = reader->data_of;

	uint32_t size, type;
	of += ptp_read_u32(d + of, &size);
	of += ptp_read_u32(d + of, &type);

	if (type == 0) return -1;

	switch (type) {
	case PTP_EC_EOS_PropValueChanged:
		ptp_eos_prop_next(d + of, event);
		break;
	case PTP_EC_EOS_InfoCheckComplete:
	case PTP_DPC_EOS_FocusInfoEx:
		event->name = ptp_get_enum_all((int)type);
		break;
	case PTP_EC_EOS_RequestObjectTransfer: {
		uint32_t a, b;
		of += ptp_read_u32(d + of, &a);
		of += ptp_read_u32(d + of, &b);
		event->name = "request object transfer";
		event->code = a;
		event->value = (int)b;
		} break;
	case PTP_EC_EOS_ObjectAddedEx: {
		uint32_t handle;
		of += ptp_read_u32(d + of, &handle);
		event->name = "new object";
		event->value = (int)handle;
		} break;
	case PTP_EC_EOS_AvailListChanged: {
		uint32_t code, dat_type, count;
		of += ptp_read_u32(d + of, &code);
		of += ptp_read_u32(d + of, &dat_type);
		of += ptp_read_u32(d + of, &count);

		unsigned int payload_size = (size - 20);

		// Make sure to not divide by zero :)
		if (payload_size != 0 && count != 0) {
			unsigned int memb_size = payload_size / count;
			ptp_set_prop_avail_info(r, (int)code, memb_size, count, d + of);
		}
		} break;
	default:
		ptp_verbose_log(r, "Unknown PTP event code %02x, event will be zeroed\n", type);
	}

	reader->data_of += size;
	reader->index++;

	return 0;
}

int ptp_eos_events(struct PtpRuntime *r, struct PtpGenericEvent **p) {
	struct PtpEventReader reader;
	ptp_eos_events_open(r, &reader);

	(*p) = malloc(sizeof(struct PtpGenericEvent) * reader.n_entries); // Leaks memory

	int i = 0;
	while (ptp_eos_events_next(r, &reader, &((*p)[i])) == 0) {
		i++;
	}

	return i;
}

int ptp_eos_events_json(struct PtpRuntime *r, char *buffer, unsigned int max) {
	struct PtpEventReader reader;
	ptp_eos_events_open(r, &reader);

	struct PtpGenericEvent event;
	int i = 0;
	unsigned int curr = osnprintf(buffer, 0, max, "[");
	while (ptp_eos_events_next(r, &reader, &event) == 0) {
		char *end = ",\n";
		if (i >= reader.n_entries - 1) end = "";

		if (event.name == NULL) {
			curr += osnprintf(buffer, curr, max, "[%u, %u]", event.code, event.value);
		} else {
			if (event.str_value == NULL) {
				curr += osnprintf(buffer, curr, max, "[\"%s\", %u]", event.name, event.value);
			} else {
				curr += osnprintf(buffer, curr, max, "[\"%s\", \"%s\"]", event.name, event.str_value);
			}
		}

		curr += osnprintf(buffer, curr, max, "%s", end);
		i++;
	}

	curr += osnprintf(buffer, curr, max, "]");

	return curr;
}
