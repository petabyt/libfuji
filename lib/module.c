#include <stdarg.h>
#include <stdlib.h>
#include <runtime.h>
#include <wifi.h>
#include <fuji.h>
#include "module.h"

static int file_to_oh(struct PakFileHandle *file) { return file->index_in_view + 1; }
//static int oh_to_file(struct PakFileHandle *file) { return file->index_in_view + 1; }

struct FujiModulePriv {
	struct ModulePriv priv;
};

static int handle_ptperr(struct PakModule *mod, int rc, const char *action) {
	switch (rc) {
		case PTP_IO_ERR: {
			if (mod->priv->dev == NULL) {
				pak_rt_fatal_error(mod, "%s", action);
				// on_disconnect will be called
			}
			mod->priv->dev = NULL; // leak
			return PAK_ERR_IO;
		}
		case PTP_NO_DEVICE: return PAK_ERR_NO_CONNECTION;
		case PTP_NO_PERM: return PAK_ERR_PERMISSION;
		default: return PAK_ERR_NON_FATAL;
	}
}

static struct PakModule *get_mod(struct PtpRuntime *r) {
	return r->priv->priv->priv.mod;
}

static int ptpip_set_extra_socket_settings(struct PtpRuntime *r, int sockfd) {
	if (r->priv->priv == NULL) return 0;
	if (get_mod(r)->priv->adapter != NULL) return pak_wifi_bind_socket_to_adapter(get_mod(r)->net, get_mod(r)->priv->adapter, sockfd);
	return 0;
}

//void ptp_verbose_log(char *fmt, ...) {
//#ifndef NDEBUG
//	char buffer[512];
//	va_list args;
//	va_start(args, fmt);
//	vsnprintf(buffer, sizeof(buffer), fmt, args);
//	va_end(args);
//	pak_global_log(buffer);
//#endif
//}

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

static int on_free(struct PakModule *mod) {
	if (mod->priv->r != NULL) {
		if (mod->priv->r->priv != NULL) free(mod->priv->r->priv);
		ptp_close(mod->priv->r);
	}
	if (mod->priv->dev != NULL) pak_bt_unref_device(mod->bt, mod->priv->dev);
	if (mod->priv->adapter != NULL) pak_wifi_unref_adapter(mod->net, mod->priv->adapter);
	free(mod->priv);
	return 0;
}

static int init(struct PakModule *mod) {
	pak_debug_log(mod, "WIP libfuji - please report bugs");
	mod->priv = calloc(1, sizeof(struct ModulePriv));
	mod->priv->mod = mod;
	pak_rt_set_tick_interval(mod, 1000 * 1000); // 1sec
	pak_rt_set_screen_supported(mod, PAK_SCREEN_DASHBOARD, 1);
	return 0;
}

int fuji_discovery_check_cancel(struct PtpRuntime *r) {
	return pak_rt_is_job_cancelled(get_mod(r), get_mod(r)->priv->current_job);
}
int app_check_thread_cancel(struct PtpRuntime *r) {
	return pak_rt_is_job_cancelled(get_mod(r), get_mod(r)->priv->current_job);
}
void app_report_download_speed(struct PtpRuntime *r, long time, size_t size) {
	pak_rt_set_download_stats(get_mod(r), get_mod(r)->priv->current_job, time, size);
}

void ptp_report_read_progress(struct PtpRuntime *r, unsigned int size) {
	if (r->priv->priv == NULL) return;
	struct PakModule *mod = get_mod(r);
	if (mod->priv->update_progress_bar_job) {
		mod->priv->total_read += size;
		unsigned int percent = ((uint64_t)mod->priv->total_read * 100ULL) / ((uint64_t)mod->priv->to_read_target);
		pak_rt_set_progress_bar(mod, mod->priv->update_progress_bar_job, (int)percent);
	}
}

static int on_try_connect_wifi(struct PakModule *mod, struct PakWiFiAdapter *handle, struct PakSavedConnection *saved, int job) {
	mod->priv->current_job = job;
	mod->priv->adapter = handle;

	struct PtpRuntime *r = fuji_ptp_new(PTP_IP_USB);
	r->set_extra_socket_settings = ptpip_set_extra_socket_settings;
	r->report_read_progress = ptp_report_read_progress;
	mod->priv->r = r;
	r->priv->priv = (struct FujiModulePriv *)mod->priv;

	const char *client_name = pak_rt_get_client_name();
	const char *setup_option = pak_rt_get_setup_option(mod);
	if (setup_option == NULL) return PAK_ERR_UNSUPPORTED;
	pak_debug_log(mod, "setup opt: %s", setup_option);
	if (!strcmp(setup_option, "wifi")) {
		r->priv->transport = FUJI_FEATURE_WIRELESS_COMM;
		strcpy(r->priv->ip_address, "192.168.0.1");
		int rc = ptpip_connect(r, r->priv->ip_address, FUJI_CMD_IP_PORT, 1);
		if (rc) return PAK_ERR_NO_CONNECTION;
	} else if (!strcmp(setup_option, "wifi-from-bt")) {
		r->priv->transport = FUJI_FEATURE_XAPP_WIRELESS_COMM;
		strcpy(r->priv->ip_address, "192.168.0.1");
		int rc = ptpip_connect(r, r->priv->ip_address, FUJI_CMD_IP_PORT, 1);
		if (rc) return PAK_ERR_NO_CONNECTION;
	} else if (!strcmp(setup_option, "local-network")) {
		struct DiscoverInfo info = {0};
		int rc = fuji_discover_thread(r, &info, client_name);
		if (rc < 0) {
			pak_debug_log(mod, "fuji_discover_thread: %d", rc);
			return PAK_ERR_NO_CONNECTION;
		}
		if (rc == FUJI_D_GO_PTP) {
			r->priv->transport = info.transport;
			strlcpy(r->priv->ip_address, info.camera_ip, sizeof(info.camera_ip));
			usleep(1000 * 1000);
			rc = ptpip_connect(r, r->priv->ip_address, info.camera_port, 3);
			if (rc) {
				pak_debug_log(mod, "ptpip_connect(%s, %d): %d", r->priv->ip_address, info.camera_port, rc);
				return PAK_ERR_NO_CONNECTION;
			}
		} else if (rc == FUJI_D_REGISTERED) {
			// todo
			pak_debug_log(mod, "Device registered");
			return 0;
		}
	} else {
		return PAK_ERR_UNSUPPORTED;
	}

	if (r->priv->transport == FUJI_FEATURE_WIRELESS_TETHER) {
		int rc = fujitether_setup(r, client_name);
		if (rc) {
			return handle_ptperr(mod, rc, "fujitether_setup");
		}
		return 0;
	}

	int rc = fuji_setup(r, client_name);
	if (rc) return PAK_ERR_NO_CONNECTION;

	if (r->priv->camera_state == FUJI_MULTIPLE_TRANSFER) {
		// TODO:
		pak_rt_set_screen_supported(mod, PAK_SCREEN_LIVE_FEED, 1);
		// TODO: Do downloading after connected
		rc = fuji_download_classic(r);
		if (rc) {
			app_print(r, "Error downloading images");
			return PAK_ERR_DISCONNECTED;
		}
		app_print(r, "Check your file manager app/gallery.");
		ptp_report_error(r, "Disconnected", 0);
		return PAK_ERR_DISCONNECTED;
	} else if (r->priv->camera_state == FUJI_FULL_ACCESS || r->priv->camera_state == FUJI_REMOTE_ACCESS || r->priv->camera_state == FUJI_MODE_REMOTE_IMG_VIEW_XAPP) {
		pak_rt_set_screen_supported(mod, PAK_SCREEN_FILE_VIEWER, 1);
		pak_rt_set_screen_supported(mod, PAK_SCREEN_FILE_GALLERY, 1);
		pak_rt_set_screen_supported(mod, PAK_SCREEN_GEOTAGGING, 1);
		pak_rt_set_screen_supported(mod, PAK_SCREEN_LIVEVIEW, 1);
	} else if (r->priv->camera_state == FUJI_PC_AUTO_SAVE) {
		pak_rt_set_screen_supported(mod, PAK_SCREEN_FILE_VIEWER, 1);
		pak_rt_set_screen_supported(mod, PAK_SCREEN_FILE_GALLERY, 1);

		pak_rt_set_dashboard_pane(mod, &(struct PakWidget) {
			.name = "autosave-thumbnails",
			.title = "Enable thumbnails (buggy)",
			.type = PAK_BOOLEAN,
			.u.boolv.v = 0,
		});
	}

	pak_rt_set_storage_info(mod, "sdcard", r->priv->num_objects, PAK_NEWEST_FIRST);

	return 0;
}

static int on_try_connect_bluetooth(struct PakModule *mod, struct PakBtDevice *device, struct PakSavedConnection *saved, int job) {
	pak_rt_set_dashboard_pane(mod, &(struct PakWidget) {
			.name = "switch-wifi",
			.title = "Connect over WiFi",
			.type = PAK_BUTTON,
	});

	mod->priv->current_job = job;
	int rc = fuji_connect_bluetooth(mod, mod->bt, device, saved);
	if (rc) return rc;
	mod->priv->dev = device;
	return 0;
}

static int on_idle_tick(struct PakModule *mod, unsigned int us_since_last_tick) {
	struct PtpRuntime *r = mod->priv->r;
	if (r != NULL) {
		if (r->connection_type == PTP_USB) {
			int rc = ptpusb_get_status(r);
			if (rc) return handle_ptperr(mod, rc, "ptpusb_get_status");
		}

		int rc = fuji_get_events(r);
		if (rc) return handle_ptperr(mod, rc, "fuji_get_events");
	}
	if (mod->priv->dev != NULL) {
		pak_bt_device_update(mod->bt, mod->priv->dev);
		if (!mod->priv->dev->is_connected) {
			pak_rt_fatal_error(mod, "Disconnected");
			return PAK_ERR_DISCONNECTED;
		}
	}
	return 0;
}

static int on_disconnect(struct PakModule *mod) {
	struct PtpRuntime *r = mod->priv->r;
	if (r != NULL) {
		ptp_report_error(r, "requested disconnect", 0);
	}
	if (mod->priv->dev != NULL) {
		pak_bt_device_disconnect(mod->bt, mod->priv->dev);
	}
	return 0;
}

static int on_switch_screen(struct PakModule *mod, int old_screen, int new_screen, int job) {
	struct PtpRuntime *r = mod->priv->r;
	if (r != NULL) {
//		pak_debug_log(mod, "%d %d", old_screen, new_screen);
//		if (new_screen == PAK_SCREEN_FILE_VIEWER && old_screen == PAK_SCREEN_FILE_VIEWER) return 0;
		if (old_screen == PAK_SCREEN_FILE_VIEWER) {
			int rc = fuji_end_file_download(r);
			if (rc) return handle_ptperr(mod, rc, "fuji_end_file_download");
		}
		if (new_screen == PAK_SCREEN_FILE_GALLERY) {
			int rc = fuji_config_image_gallery(r);
			if (rc) return handle_ptperr(mod, rc, "fuji_config_image_gallery");
		} else if (new_screen == PAK_SCREEN_FILE_VIEWER) {
			int rc = fuji_begin_file_download(r);
			if (rc) return handle_ptperr(mod, rc, "fuji_begin_file_download");
		}
	}
	return 0;
}

struct TempStruct {
	struct PakModule *mod;
	struct PakFileHandle *file;
	unsigned int target_size;
};

static int download_add(void *arg, void *data, unsigned int size, unsigned int offset) {
	struct TempStruct *temp = arg;
	pak_rt_add_file_contents(temp->mod, temp->file, data, size, offset, temp->target_size);
	return 0;
}

static int on_request_file_contents(struct PakModule *mod, int job, struct PakFileHandle *file) {
	struct PtpRuntime *r = mod->priv->r;
	if (r == NULL) return 0;

	mod->priv->current_job = job;

	struct PtpObjectInfo oi;
	int rc = ptp_get_object_info(r, file->index_in_view + 1, &oi);
	if (rc == PTP_CHECK_CODE || rc == PTP_RUNTIME_ERR) {
		pak_rt_add_file_metadata(mod, file, NULL);
		return PAK_ERR_NON_FATAL;
	} else if (rc) return handle_ptperr(mod, rc, "fuji_begin_download_get_object_info");

	plat_update_object_info(r, file->index_in_view + 1, &oi);

	mod->priv->update_progress_bar_job = job;
	mod->priv->to_read_target = oi.compressed_size;
	mod->priv->total_read = 0;

	pak_rt_set_progress_bar(mod, job, 0);
	rc = fuji_download_file(r, file->index_in_view + 1, oi.compressed_size, download_add, &(struct TempStruct){
		.mod = mod,
		.file = file,
		.target_size = oi.compressed_size,
	});
	pak_rt_add_file_contents(mod, file, NULL, 0, 0, 0);
	mod->priv->update_progress_bar_job = 0;
	if (rc) return handle_ptperr(mod, rc, "fuji_download_file");
	return 0;
}

static int on_request_thumbnail(struct PakModule *mod, int job, struct PakFileHandle *file) {
	unsigned int offset, length;
	struct PtpRuntime *r = mod->priv->r;
	if (r == NULL) return 0;

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
		case PTP_OF_MOV: return "video/quicktime";
		case PTP_OF_PNG: return "image/png";
		default: return "application/unknown";
	}
}

int plat_update_object_info(struct PtpRuntime *r, int handle, const struct PtpObjectInfo *oi) {
	struct PakModule *mod = get_mod(r);

	struct PakFileHandle file = {0};
	file.index_in_view = handle - 1;
	file.storage_name = "sdcard";

	return pak_rt_add_file_metadata(mod, &file, &(struct PakFileMetadata){
		.filename = oi->filename,
		.file_size = (int)oi->compressed_size,
		.mime_type = get_mime_type(oi->obj_format),
		.image_height = (int)oi->img_height,
		.image_width = (int)oi->img_width,
	});
}

static int on_request_file_metadata(struct PakModule *mod, int job, struct PakFileHandle *file) {
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

static int on_command(struct PakModule *mod, int job, int argc, const char * const *argv) {
	if (mod->priv->dev != NULL) return fuji_bt_handle_command(mod, mod->priv->dev, argc, argv);
	return PAK_ERR_UNIMPLEMENTED;
}

static int on_prop_changed(struct PakModule *mod, int job, struct PakWidget *prop) {
	if (!strcmp(prop->name, "switch-wifi")) {
		return fuji_bluetooth_connect_to_wifi(mod, mod->bt, mod->priv->dev);
	} else if (!strcmp(prop->name, "autosave-thumbnails")) {
		mod->priv->r->priv->allow_autosave_thumbnails = prop->u.boolv.v;
	}
	return 0;
}

static int on_try_connect_usb(struct PakModule *mod, libusb_device *dev, struct PakSavedConnection *saved, int job) {
	pak_debug_log(mod, "USB not supported yet");
	return -1;
}

int get_module(struct PakModule *mod) {
	mod->init = init;
	mod->free = on_free;
	mod->on_try_connect_usb = on_try_connect_usb;
	mod->on_try_connect_wifi = on_try_connect_wifi;
	mod->on_try_connect_bluetooth = on_try_connect_bluetooth;
	mod->on_request_file_thumbnail = on_request_thumbnail;
	mod->on_request_file_metadata = on_request_file_metadata;
	mod->on_request_file_contents = on_request_file_contents;
	mod->on_idle_tick = on_idle_tick;
	mod->on_disconnect = on_disconnect;
	mod->on_switch_screen = on_switch_screen;
	mod->on_custom_command = on_command;
	mod->on_setting_changed = on_prop_changed;
	return 0;
}