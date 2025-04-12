// Native rendering tests

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libpict.h>
#include "app.h"
#include "backend.h"
#include "android.h"
#include "fuji.h"

#define JPEG_LIB_VERSION 62
typedef unsigned char JSAMPLE;
#include <jpeglib.h>

METHODDEF(void)my_error_exit(j_common_ptr cinfo) {
	struct jpeg_error_mgr *err = cinfo->err;
	char buffer[1024];
	err->format_message(cinfo, buffer);
	plat_dbg(buffer);
	abort();
}

int render_liveview_jpeg(const unsigned char *input, size_t size, uint8_t *framebuffer, int screen_width, int screen_height) {
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jerr.error_exit = my_error_exit;
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, input, size);

	// Note: We don't actually need to read the header - optimize this?
	jpeg_read_header(&cinfo, TRUE);

	cinfo.out_color_space = JCS_EXT_RGBA;

	int y = 0;
	int x = 0;
	if (screen_width < screen_height) {
		// Scale image height to aspect ratio (hacky math, shouldn't overflow though)
		y = (screen_height / 2) - (cinfo.image_height * screen_width / cinfo.image_width / 2);
		cinfo.scale_num = screen_width;
		cinfo.scale_denom = cinfo.image_width;
	} else {
		cinfo.scale_num = screen_height;
		cinfo.scale_denom = cinfo.image_height;
		x = (screen_width / 2) - (cinfo.image_width);
	}

	jpeg_start_decompress(&cinfo);

	while (cinfo.output_scanline < cinfo.output_height) {
		// Read scanlines directly into framebuffer
		unsigned char *buffer_array[1];
		buffer_array[0] = framebuffer + (y * (screen_width * 4)) + (x * 4);
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
		if (y <= screen_height - 2) {
			y++;
		}
	}

	// Finish decompression
	jpeg_finish_decompress(&cinfo);

	// Release the JPEG decompression object
	jpeg_destroy_decompress(&cinfo);

	return 0;
}

#ifndef JNI_LV
#define JNI_LV(ret, name) JNIEXPORT ret JNICALL Java_dev_danielc_fujiapp_Liveview_##name
#endif

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <inttypes.h>
static ANativeWindow *window = 0;

static void draw_button(JNIEnv *env, ANativeWindow_Buffer *winbuf, const char *text, int posx, int posy, int fg, int bg) {
	jmethodID render_text_m = (*env)->GetStaticMethodID(env, (*env)->FindClass(env, "dev/danielc/fujiapp/Liveview"), "renderText", "(Ljava/lang/String;II)[B");
	jbyteArray jdata = (*env)->CallStaticObjectMethod(env, (*env)->FindClass(env, "dev/danielc/fujiapp/Liveview"), render_text_m, (*env)->NewStringUTF(env, text), fg, bg);
	uint32_t *data = (uint32_t *)(*env)->GetByteArrayElements(env, jdata, 0);

	for (int x = 0; x < 256; x++) {
		for (int y = 0; y < 256; y++) {
			((uint32_t *)winbuf->bits)[(x + posx) + ((y + posy) * winbuf->stride)] = (data[x + (y * 256)]);
		}
	}
}

JNI_LV(void, cUpdateSurface)(JNIEnv *env, jobject thiz, jobject ctx, jobject surface) {
	if (surface == 0) abort();
	window = ANativeWindow_fromSurface(env, surface);
	ANativeWindow_acquire(window);

	ANativeWindow_setBuffersGeometry(window, 0, 0,  AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);

	ANativeWindow_Buffer winbuf;
	ARect bounds = {0, 0, 1, 1};
	if (ANativeWindow_lock(window, &winbuf, &bounds)) {
		abort();
	}

	plat_dbg("Dimension: %dx%dx%d", winbuf.width, winbuf.height, winbuf.stride);
	plat_dbg("Prop: %X", winbuf.format);
	plat_dbg("Framebuffer: %LX", winbuf.bits);

#define RGBA888(x) ((x & 0xFF) << 16) | (x & 0xFF00) | ((x & 0xFF0000) >> 8)

	//draw_button(env, &winbuf, "Hello, World\nyjis is test", 1, 1, 0xffffff, 0x4d71ab);

	int size;
	uint8_t *jpeg = jni_get_assets_file(env, ctx, "lv2.jpg", &size);
	render_liveview_jpeg(jpeg, size, winbuf.bits, winbuf.stride, winbuf.height);

	if (ANativeWindow_unlockAndPost(window)) {
		abort();
	}
}
