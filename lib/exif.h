/** \file */
// Custom stream-based EXIF parser
#ifndef EXIF_H
#define EXIF_H

typedef uint8_t *get_additional_bytes(void *arg, uint8_t *buffer, int old_len, int last_len);

struct ExifC {
	uint8_t *buf;
	int length;
	int exif_start;
	void *arg;
	get_additional_bytes *get_more;
	int subifd;
	int thumb_of;
	int thumb_size;
};

int exif_start_raw(struct ExifC *c);

#endif
