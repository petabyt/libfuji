#include <stdlib.h>
#include <runtime.h>
#include <wifi.h>
#include <fuji.h>

int fuji_connect_bluetooth(struct PakBt *ctx, struct PakBtDevice *dev);

struct ModulePriv {
	struct PtpRuntime *r;
	struct Module *mod;
	int current_job;

	int update_progress_bar_job;
	unsigned int total_read;
	unsigned to_read_target;
	int adapter_is_present;
	struct PakWiFiAdapter adapter;
};

struct FujiModulePriv {
	struct ModulePriv priv;
};

static int handle_ptperr(struct Module *mod, int rc, const char *action) {
	switch (rc) {
		case PTP_IO_ERR: {
			pak_rt_fatal_error(mod, "%s", action);
			return PAK_ERR_IO;
		}
		case PTP_NO_DEVICE: return PAK_ERR_NO_CONNECTION;
		case PTP_NO_PERM: return PAK_ERR_PERMISSION;
		default: return PAK_ERR_NON_FATAL;
	}
}

static struct Module *get_mod(struct PtpRuntime *r) {
	return r->priv->priv->priv.mod;
}

int ptpip_set_extra_socket_settings(struct PtpRuntime *r, int sockfd) {
	if (!get_mod(r)->priv->adapter_is_present) return -1;
	return pak_wifi_bind_socket_to_adapter(get_mod(r)->net, &get_mod(r)->priv->adapter, sockfd);
}

void ptp_verbose_log(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	//pak_global_log(buffer);
}

__attribute__((weak))
void ptp_error_log(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__ ((noreturn))
void ptp_panic(char *fmt, ...) {
	printf("PTP abort: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	fflush(stdout);
	putchar('\n');
	abort();
}

void app_print(struct PtpRuntime *r, char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	pak_debug_log(r->priv->priv->priv.mod, buffer);
}

void app_send_cam_name(struct PtpRuntime *r, const char *name) {
	pak_rt_set_session_property(get_mod(r), PAK_PROP_NAME, name);
}

static int on_free(struct Module *mod) {
	if (mod->priv->r != NULL) ptp_close(mod->priv->r);
	free(mod->priv);
	return 0;
}

static int init(struct Module *mod) {
	pak_debug_log(mod, "libfuji init");
	mod->priv = calloc(1, sizeof(struct ModulePriv));
	mod->priv->mod = mod;
	pak_rt_set_tick_interval(mod, 1000 * 1000); // 1sec
	return 0;
}

static int on_try_connect_wifi(struct Module *mod, struct PakWiFiAdapter *handle, int job) {
	mod->priv->current_job = job;
	mod->priv->adapter_is_present = 1;
	memcpy(&mod->priv->adapter, handle, sizeof(struct PakWiFiAdapter));
	struct PtpRuntime *r = ptp_new(PTP_IP_USB);
	fuji_reset_ptp(r);
	mod->priv->r = r;
	r->priv->priv = (struct FujiModulePriv *)mod->priv;

	// TODO: Determine transport from connection info
	fuji_get(r)->transport = FUJI_FEATURE_WIRELESS_COMM;

	//strcpy(fuji_get(r)->ip_address, "192.168.0.1");
	strcpy(fuji_get(r)->ip_address, "192.168.1.164");
	pak_debug_log(mod, "Connecting to %s", fuji_get(r)->ip_address);
	int rc = ptpip_connect(r, fuji_get(r)->ip_address, FUJI_CMD_IP_PORT, 1);
	pak_debug_log(mod, "done");
	if (rc) {
		return PAK_ERR_NO_CONNECTION;
	}

	pak_rt_set_screen_supported(mod, SCREEN_DASHBOARD, 1);

	if (fuji_get(r)->transport == FUJI_FEATURE_WIRELESS_TETHER) {
		rc = fujitether_setup(r);
		if (rc) {
			return handle_ptperr(mod, rc, "fujitether_setup");
		}
	} else {
		rc = fuji_setup(r, "fudge");
		if (rc) {
			return PAK_ERR_IO;
		} else {
			if (fuji_get(r)->camera_state == FUJI_MULTIPLE_TRANSFER) {
				pak_rt_set_screen_supported(mod, SCREEN_LIVE_FEED, 1);
				// TODO: Do downloading after connected
				rc = fuji_download_classic(r);
				if (rc) {
					app_print(r, "Error downloading images");
					return PAK_ERR_DISCONNECTED;
				}
				app_print(r, "Check your file manager app/gallery.");
				ptp_report_error(r, "Disconnected", 0);
				return PAK_ERR_DISCONNECTED;
			} else {
				pak_rt_set_screen_supported(mod, SCREEN_FILE_VIEWER, 1);
				pak_rt_set_screen_supported(mod, SCREEN_FILE_GALLERY, 1);
				pak_rt_set_screen_supported(mod, SCREEN_GEOTAGGING, 1);
				pak_rt_set_screen_supported(mod, SCREEN_LIVEVIEW, 1);
				pak_rt_set_storage_info(mod, "sdcard", fuji_get(r)->num_objects, PAK_NEWEST_FIRST);
			}
		}
	}

	return 0;
}

static int on_try_connect_bluetooth(struct Module *mod, struct PakBtDevice *device, int job) {
	return fuji_connect_bluetooth(mod->bt, device);
}

static int on_idle_tick(struct Module *mod, unsigned int us_since_last_tick) {
	struct PtpRuntime *r = mod->priv->r;
	if (r == NULL) {
		return 0;
	}
	if (r->connection_type == PTP_USB) {
		int rc = ptpusb_get_status(r);
		if (rc) return handle_ptperr(mod, rc, "ptpusb_get_status");
	}

	int rc = fuji_get_events(r);
	if (rc) return handle_ptperr(mod, rc, "fuji_get_events");
	return 0;
}

static int on_disconnect(struct Module *mod) {
	struct PtpRuntime *r = mod->priv->r;
	if (r == NULL) {
		return 0;
	}
	ptp_report_error(r, "requested disconnect", 0);
	return 0;
}

static int on_switch_screen(struct Module *mod, int old_screen, int new_screen, int job) {
	struct PtpRuntime *r = mod->priv->r;
	if (r == NULL) {
		return 0;
	}
	if (new_screen == SCREEN_FILE_GALLERY) {
		int rc = fuji_config_image_viewer(r);
		if (rc) return handle_ptperr(mod, rc, "fuji_config_image_viewer");
	}
	return 0;
}

void ptp_report_read_progress(struct PtpRuntime *r, unsigned int size) {
	struct Module *mod = get_mod(r);
	if (mod->priv->update_progress_bar_job) {
		mod->priv->total_read += size;
		unsigned int percent = (mod->priv->total_read * 100) / (mod->priv->to_read_target);
		pak_rt_set_progress_bar(mod, mod->priv->update_progress_bar_job, (int)percent);
	}
}

struct TempStruct {
	struct Module *mod;
	struct FileHandle *file;
	int target_size;
};

static int jbytearray_add(void *arg, void *data, int size, int read) {
	struct TempStruct *temp = arg;
	pak_rt_add_file_contents(temp->mod, temp->file, data, size, read < temp->target_size);
	return 0;
}

static int on_request_file_contents(struct Module *mod, int job, struct FileHandle *file) {
	struct PtpRuntime *r = mod->priv->r;
	if (r == NULL) {
		return 0;
	}

	struct PtpObjectInfo oi;
	int rc = fuji_begin_download_get_object_info(r, file->index_in_view + 1, &oi);
	if (rc) return handle_ptperr(mod, rc, "fuji_begin_download_get_object_info");

	struct FileMetadata *md = pak_rt_get_metadata(mod, file);
	if (md == NULL) {
		rc = ptp_get_object_info(r, file->index_in_view + 1, &oi);
		if (rc == PTP_CHECK_CODE || rc == PTP_RUNTIME_ERR) {
			pak_rt_add_file_metadata(mod, file, NULL);
			return PAK_ERR_NON_FATAL;
		} else if (rc) return handle_ptperr(mod, rc, "fuji_begin_download_get_object_info");

		plat_update_object_info(r, file->index_in_view + 1, &oi);
		md = pak_rt_get_metadata(mod, file);
		if (md == NULL) return PAK_ERR_UNDEFINED;
	}
	mod->priv->update_progress_bar_job = job;
	mod->priv->to_read_target = md->file_size;
	mod->priv->total_read = 0;

	pak_rt_set_progress_bar(mod, job, 0);
	rc = fuji_download_file(r, file->index_in_view + 1, md->file_size, jbytearray_add, &(struct TempStruct){
		.mod = mod,
		.file = file,
		.target_size = md->file_size,
	});
	//pak_rt_set_progress_bar(mod, job, 100);
	pak_rt_add_file_contents(mod, file, file, 0, 0); // bad
	pak_rt_release_metadata(mod, md);
	mod->priv->update_progress_bar_job = 0;
	if (rc) return handle_ptperr(mod, rc, "fuji_download_file");
	return 0;
}

static int on_request_thumbnail(struct Module *mod, int job, struct FileHandle *file) {
	unsigned int offset, length;
	struct PtpRuntime *r = mod->priv->r;
	if (r == NULL) {
		return 0;
	}

	ptp_mutex_lock(r);
	int rc = fuji_get_thumb(r, file->index_in_view + 1, &offset, &length);
	if (rc == PTP_CHECK_CODE || rc == PTP_RUNTIME_ERR) {
		pak_rt_add_file_metadata(mod, file, NULL);
		ptp_mutex_unlock(r);
		return 0;
	} else if (rc) return handle_ptperr(mod, rc, "fuji_get_thumb");

	pak_rt_add_file_thumbnail(mod, file, ptp_get_payload(r) + offset, length);

	ptp_mutex_unlock(r);
	return 0;
}

static const char *get_mime_type(uint16_t object_format) {
	switch (object_format) {
		case PTP_OF_JPEG: return "image/jpeg";
		case PTP_OF_MOV: return "image/quicktime";
		case PTP_OF_PNG: return "image/png";
		default: return "application/unknown";
	}
}


int plat_update_object_info(struct PtpRuntime *r, int handle, const struct PtpObjectInfo *oi) {
	struct Module *mod = get_mod(r);

	struct FileHandle file = {0};
	file.index_in_view = handle - 1;
	file.storage_name = "sdcard";

	return pak_rt_add_file_metadata(mod, &file, &(struct FileMetadata){
		.filename = oi->filename,
		.file_size = (int)oi->compressed_size,
		.mime_type = get_mime_type(oi->obj_format),
		.image_height = (int)oi->img_height,
		.image_width = (int)oi->img_width,
	});
}

static int on_request_file_metadata(struct Module *mod, int job, struct FileHandle *file) {
	struct PtpObjectInfo info;
	struct PtpRuntime *r = mod->priv->r;
	if (r == NULL) {
		return 0;
	}
	int rc = ptp_get_object_info(r, file->index_in_view + 1, &info);
	if (rc == PTP_CHECK_CODE || rc == PTP_RUNTIME_ERR) {
		pak_rt_add_file_metadata(mod, file, NULL);
		return 0;
	} else if (rc) return handle_ptperr(mod, rc, "fuji_get_thumb");

	return plat_update_object_info(r, file->index_in_view + 1, &info);
}

int get_module_libfuji(struct Module *mod) {
	mod->init = init;
	mod->free = on_free;
	mod->on_try_connect_wifi = on_try_connect_wifi;
	mod->on_try_connect_bluetooth = on_try_connect_bluetooth;
	mod->on_request_file_thumbnail = on_request_thumbnail;
	mod->on_request_file_metadata = on_request_file_metadata;
	mod->on_request_file_contents = on_request_file_contents;
	//mod->on_find_connection = on_find_connection;
	mod->on_idle_tick = on_idle_tick;
	mod->on_disconnect = on_disconnect;
	mod->on_switch_screen = on_switch_screen;
	return 0;
}
__attribute__((weak)) int get_module(struct Module *mod) { return get_module_libfuji(mod); }
