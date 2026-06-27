#pragma once
#include <runtime.h>
#include <wifi.h>
#include <fuji.h>

struct ModulePriv {
	struct PtpRuntime *r;
	struct Module *mod;
	struct PakBtDevice dev;
	int current_job;

	int update_progress_bar_job;
	unsigned int total_read;
	unsigned to_read_target;
	int adapter_is_present;
	struct PakWiFiAdapter adapter;
};

int fuji_connect_bluetooth(struct Module *mod, struct PakBt *ctx, struct PakBtDevice *dev, struct PakSavedConnection *saved);

int fuji_bt_handle_command(struct Module *mod, struct PakBtDevice *dev, int argc, const char * const *argv);