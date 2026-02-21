#include <stdlib.h>
#include <runtime.h>
#include <wifi.h>
#include <fuji.h>

struct ModulePriv {
	struct PtpRuntime *r;
};

static int init(struct Module *mod) {
	pak_global_log("libfuji init");
	pak_rt_set_screen_supported(mod, SCREEN_FILE_GALLERY, 1);
	return 0;
}

static int on_try_connect_wifi(struct Module *mod, struct PakWiFiAdapter *handle, int job) {
	return 0;
}

static int on_idle_tick(struct Module *mod, unsigned int us_since_last_tick) {
	struct PtpRuntime *r = mod->priv->r;
	if (r->connection_type == PTP_USB) {
		int rc = ptpusb_get_status(r);
		if (rc) return -1;
	}

	int rc = fuji_get_events(r);
	if (rc) return -1;
	return 0;
}

static int on_disconnect(struct Module *mod) {
	return 0;
}

static int on_switch_screen(struct Module *mod, int old_screen, int new_screen, int job) {
	return 0;
}

static int on_request_file_contents(struct Module *mod, int screen, int job, struct FileHandle *file) {
	return 0;
}

static int on_request_thumbnail(struct Module *mod, int screen, int job, struct FileHandle *file) {
	struct PtpRuntime *r = mod->priv->r;
	int offset, length;
	ptp_mutex_lock(r);

#if 0
	int rc = fuji_get_thumb(r, handle, &offset, &length);
	if (rc) {
		ptp_mutex_unlock(r);
		return NULL;
	}

	// If error from fuji_get_thumb, array will be zero
	jbyteArray ret = (*env)->NewByteArray(env, length);
	(*env)->SetByteArrayRegion(env, ret, 0, length, (const jbyte *) ptp_get_payload(r) + offset);

	ptp_mutex_unlock(r);
#endif

	return 0;
}

static int on_request_file_metadata(struct Module *mod, int screen, int job, struct FileHandle *file) {
	return 0;
}

static int on_run_test(struct Module *mod, int screen, int job) {
	return 0;
}

static int on_custom_command(struct Module *mod, const char *request) {
	return 0;
}

int get_module_dummy(struct Module *mod) {
	mod->init = init;
	mod->on_try_connect_wifi = on_try_connect_wifi;
	mod->on_idle_tick = on_idle_tick;
	mod->on_disconnect = on_disconnect;
	mod->on_switch_screen = on_switch_screen;
	return 0;
}
