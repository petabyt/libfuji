#pragma once
#include <runtime.h>
#include <wifi.h>
#include <fuji.h>

struct ModulePriv {
	struct PtpRuntime *r;
	struct PakModule *mod;
	struct PakBtDevice *dev;
	int current_job;

	int update_progress_bar_job;
	unsigned int total_read;
	unsigned to_read_target;
	struct PakWiFiAdapter *adapter;
};

int fuji_connect_bluetooth(struct PakModule *mod, struct PakBt *ctx, struct PakBtDevice *dev, struct PakSavedConnection *saved);

int fuji_bt_handle_command(struct PakModule *mod, struct PakBtDevice *dev, int argc, const char * const *argv);

int fuji_bluetooth_connect_to_wifi(struct PakModule *mod, struct PakBt *ctx, struct PakBtDevice *dev);

#define LIVE_STORAGE_DEVICE_NAME "live"

#define OPTION_WIFI_FROM_BT_XAPP "wifi-from-bt-xapp"
#define OPTION_WIFI "wifi"
#define OPTION_LOCAL_NETWORK "local-network"

#define WIDGET_RESIZE_IMAGES "resize-images"
#define WIDGET_AUTOSAVE_THUMBS "autosave-thumbnails"
#define WIDGET_SWITCH_WIFI "switch-wifi"