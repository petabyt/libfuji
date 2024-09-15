// https://www.media.mit.edu/pia/Research/deepview/exif.html
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "exif.h"
#ifdef ANDROID
#include <jni.h>
#endif

static inline int read_be_u32(void *b, uint32_t *o) {
	uint32_t x = ((uint32_t *)b)[0];
	*o = ((x & 0x000000FF) << 24) |
		((x & 0x0000FF00) << 8)  |
		((x & 0x00FF0000) >> 8)  |
		((x & 0xFF000000) >> 24);
	return 4;
}

static inline int read_be_u16(void *b, uint16_t *o) {
	uint16_t x = ((uint16_t *)b)[0];
	*o = ((x & 0x00FF) << 8) |
		((x & 0xFF00) >> 8);
	return 2;
}

static inline int read_u32(void *b, uint32_t *o) {
	*o = (((uint32_t *)b)[0]);
	return 4;
}

static inline int read_u16(void *b, uint16_t *o) {
	*o = (((uint16_t *)b)[0]);
	return 2;
}

static int ensure_size(struct ExifC *c, int length) {
	if (length > c->length) {
		length += 1000;
		uint8_t *buf = c->get_more(c->arg, c->buf, length, c->length);
		if (buf == NULL) return -1;
		c->buf = buf;
		c->length = length;
	}
	return 0;
}

int exif_parse_ifd(struct ExifC *c, int of) {
	uint16_t entries;
	of += read_u16(c->buf + of, &entries);

	for (int i = 0; i < entries; i++) {
		if (ensure_size(c, of + 12)) return -1;
		uint16_t tag, format;
		uint32_t components, value_offset;
		of += read_u16(c->buf + of, &tag);
		of += read_u16(c->buf + of, &format);
		of += read_u32(c->buf + of, &components);
		of += read_u32(c->buf + of, &value_offset);

		switch (tag) {
		case 0x0201:
			c->thumb_of = c->exif_start + value_offset;
			continue;
		case 0x0202:
			c->thumb_size = value_offset;
			continue;
		case 0x8769:
			c->subifd = value_offset;
			continue;
		}

#if 0
		switch (format) {
		case 2:
			printf("%04X str: %s\n", tag, c->buf + c->exif_start + value_offset);
			break;
		case 4:
			printf("%04X u32: %X\n", tag, value_offset);
			break;
		case 3:
			printf("%04X u16: %u\n", tag, value_offset);
			break;
		case 5: {
			uint32_t num, denom;
			read_u32(c->buf + c->exif_start + value_offset, &denom);
			read_u32(c->buf + c->exif_start + value_offset + 4, &num);
			printf("%04X u64: %u/%u\n", tag, num, denom);
			}
			break;
		case 7:
			printf("udf: %x\n", value_offset);
			break;
		default:
			printf("Unhandled format %d\n", format);
		}
#endif
	}

	if (c->thumb_of != 0 && c->thumb_size != 0) {
		if (ensure_size(c, c->thumb_of + c->thumb_size)) return -1;
	}
	
	return of;
}

int exif_start_entries(struct ExifC *c, int of) {
	uint32_t offset;
	uint16_t order, version, entries;
	of += read_be_u16(c->buf + of, &order);

	of += read_u16(c->buf + of, &version);
	of += read_u32(c->buf + of, &offset);

	of += offset - 8;

	int rc = exif_parse_ifd(c, of);
	if (rc < 0) return rc;
	of = rc;

	uint32_t ifd1 = 0;
	of += read_u32(c->buf + of, &ifd1);

	if (ifd1 != 0) {
		if (ensure_size(c, c->exif_start + ifd1)) return -1;
		rc = exif_parse_ifd(c, c->exif_start + ifd1);
		if (rc < 0) return rc;
	}

	if (c->subifd != 0) {
		if (ensure_size(c, c->exif_start + c->subifd)) return -1;
		rc = exif_parse_ifd(c, c->exif_start + c->subifd);
		if (rc < 0) return rc;
	}

	return of;
}

int exif_start(struct ExifC *c, int of) {
	uint16_t type, len;
	of += read_be_u16(c->buf + of, &type);
	of += read_be_u16(c->buf + of, &len);

	if (!memcmp(c->buf + of, "Exif\0\0", 6)) {
		of += 6;
		c->exif_start = of;
		of = exif_start_entries(c, of);
	}
	
	return of;
}

int exif_start_raw(struct ExifC *c) {
	int of = 0;

	if (ensure_size(c, 100)) return -1;

	// Handle RAF header
	const char *fuji_raw = "FUJIFILMCCD-RAW";
	if (!memcmp(c->buf, fuji_raw, strlen(fuji_raw))) {
		uint32_t header_len;
		of = 0x54;
		read_be_u32(c->buf + of, &header_len);
		if (ensure_size(c, header_len)) return -1;
		of = header_len;
	}

	if (c->buf[of] == 0xFF && c->buf[of + 1] == 0xD8) {
		of += 2;
		of = exif_start(c, of);
	}

	return of;
}

static uint8_t *file_add(void *arg, uint8_t *buffer, int new_len, int old_len) {
	uint8_t *new = realloc(buffer, new_len);
	fread(new + old_len, 1, new_len - old_len, (FILE *)arg);
	return new;
}

static int exif_main(void) {
	FILE *f = fopen("DSCF5319.RAF", "rb");
	if (f == NULL) {
		puts("bad file");
		abort();
	}

	uint8_t *buffer = malloc(1000);
	fread(buffer, 1, 1000, f);

	struct ExifC c = {0};
	c.length = 1000;
	c.buf = buffer;
	c.arg = f;
	c.get_more = file_add;

	exif_start_raw(&c);

	fclose(f);

	return 0;
}

#ifdef ANDROID
JNIEXPORT jbyteArray JNICALL Java_dev_danielc_common_Exif_getThumbnail__Ljava_lang_String_2(JNIEnv *env, jobject thiz, jstring filepath) {
	const char *cfilepath = (*env)->GetStringUTFChars(env, filepath, 0);
	FILE *f = fopen(cfilepath, "rb");
	if (f == NULL) {
		abort();
	}

	uint8_t *buffer = malloc(100);
	fread(buffer, 1, 100, f);

	struct ExifC c = {0};
	c.length = 100;
	c.buf = buffer;
	c.arg = f;
	c.get_more = file_add;

	int rc = exif_start_raw(&c);
	if (rc < 0) {
		abort();
	}

	if (c.thumb_of == 0 || c.thumb_size == 0) {
		return NULL;
	}

	jbyteArray result = (*env)->NewByteArray(env, c.thumb_size);
	(*env)->SetByteArrayRegion(env, result, 0, c.thumb_size, (jbyte *)(c.buf + c.thumb_of));

	free(c.buf);
	fclose(f);
	(*env)->ReleaseStringUTFChars(env, filepath, cfilepath);

	return result;
}

JNIEXPORT jstring JNICALL Java_dev_danielc_common_Exif_getInformationJSON(JNIEnv *env, jobject thiz, jstring path) {
	return NULL;
}
#endif
