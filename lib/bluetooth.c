// Derivative work of https://github.com/gkoh/furble
#include <stdint.h>
#include <runtime.h>
#include <bluetooth.h>
#include "module.h"

#define SVC_BACKUPS "af854c2e-b214-458e-97e2-912c4ecf2cb8"
// 0000   62 61 63 6b 75 70 2e 64 61 74 00 00 00 00 00 00
// 0010   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
// 0020   00 00 00 00 00 00 00 00 7c 82 00 00
struct __attribute__((packed)) BackupFileInfo {
	char name[40];
	uint32_t filesize;
};
#define CHR_BACKUPS_FILE_NAME "c922ac69-9480-4348-8f4b-9ee29bc30d1d"
// Seems to be a mode setting
#define CHR_BACKUPS_UNKNOWN2 "68052e8a-fb91-404f-8847-0eb4be24308c"
#define CHR_BACKUPS_FILE_DATA "ac0c799a-fa6c-4df5-bbc5-bb95cce7e6ea"
// 05 00 ff ff
#define CHR_BACKUPS_UNKNOWN3 "2e27ed9f-5506-41cd-ba48-dac06669ad95"

#define SVC_UNKNOWN1 "4e941240-d01d-46b9-a5ea-67636806830b"
#define CHR_UNKNOWN1_DEVICE_NAME "bf6dc9cf-3606-4ec9-a4c8-d77576e93ea4" // SSID
// 01 00
#define CHR_UNKNOWN1_UNKNOWN1 "aab609c4-94dd-4d89-bc60-665d5090b828"
#define CHR_UNKNOWN1_UNKNOWN2 "c95d91ae-b247-4d6d-8661-7dd5d6a0f85b"
#define CHR_UNKNOWN1_UNKNOWN3 "75823784-fbb7-4b71-abae-cd9a34072e3c"
#define CHR_UNKNOWN1_UNKNOWN4 "82a9f452-c5ce-4ef5-8203-3fc9a47f8171"

#define SVC_UNKNOWN2 "117c4142-edd4-4c77-8696-dd18eebb770a"
#define CHR_UNKNOWN2_UNKNOWN1 "49a12959-dfaa-4eb2-89ce-62548ad948f3"

#define SVC_UNKNOWN3 "e872b11f-d526-4ae1-9bb4-89a99d48fa59"
#define CHR_UNKNOWN3_UNKNOWN1 "c52edbce-1fe2-4ecc-9483-907e6592be9e"

// 0x4001
#define SVC_PAIR_UUID "91f1de68-dff6-466e-8b65-ff13b0f16fb8"
// 0x4042
#define CHR_PAIR_UUID "aba356eb-9633-4e60-b73f-f52516dbd671"
// 0x4012
#define CHR_IDEN_UUID "85b9163e-62d1-49ff-a6f5-054b4630d4a1"
// 0x4062
#define CHR_UNKNOWN_UUID "eb4166b0-9cca-445e-a4e4-75b3817fd57a"

// Subscriptions
#define SVC_CONF_UUID "4c0020fe-f3b6-40de-acc9-77d129067b14"
#define CHR_CONF_UNKNOWN1 "1587b102-0b6d-4b63-9226-66fcc6d17387"

// 0x5013
#define CHR_IND1_UUID "a68e3f66-0fcc-4395-8d4c-aa980b5877fa"
// 0x5023
#define CHR_IND2_UUID "bd17ba04-b76b-4892-a545-b73ba1f74dae"
// 0x5033
#define CHR_NOT1_UUID "f9150137-5d40-4801-a8dc-f7fc5b01da50"
#define CHR_IND3_UUID "049ec406-ef75-4205-a390-08fe209c51f0"

#define SVC_SHUTTER_UUID "6514eb81-4e8f-458d-aa2a-e691336cdfac"
// Shutter characteristic
#define CHR_SHUTTER_UUID "7fcf49c6-4ff0-4777-a03d-1a79166af7a8"
#define CHR_SHUTTER_UUID2 "600655e6-3637-42f1-8fb2-44efc5c63b13"

// Geo location characteristic
#define GEOTAG_UPDATE "ad06c7b7-f41a-46f4-a29a-712055319122"

#define SVC_GEOTAG_UUID "3b46ec2b-48ba-41fd-b1b8-ed860b60d22b"
#define CHR_GEOTAG_UUID "0f36ec14-29e5-411a-a1b6-64ee8383f090"

#define GENERIC_ACCESS_SERVICE "00001800-0000-1000-8000-00805f9b34fb"
#define DEVICE_NAME "00002a00-0000-1000-8000-00805f9b34fb"

#define TOKEN_LEN 4
#define TYPE_TOKEN 0x02

#define MAX_NAME 64

const static uint8_t SHUTTER_RELEASE[] = {0x00, 0x00};
const static uint8_t SHUTTER_CMD[] = {0x01, 0x00};
const static uint8_t SHUTTER_PRESS[] = {0x02, 0x00};
const static uint8_t SHUTTER_FOCUS[] = {0x03, 0x00};

/**
 * Advertisement manufacturer data.
 */
typedef struct __attribute__((packed)) {
	uint16_t company_id;
	uint8_t type;
} fujifilm_adv_t;

/** 4 byte token. */
typedef struct __attribute__((packed)) {
	uint8_t data[TOKEN_LEN];
} token_t;

/** Advertisement manufacturer data. */
typedef struct __attribute__((packed)) {
	fujifilm_adv_t adv;
	token_t token;
} adv_basic_t;

/**
 * Non-volatile storage type.
 */
typedef struct _nvs_t {
	char name[MAX_NAME]; /** Human readable device name. */
	uint64_t address;		/** Device MAC address. */
	uint8_t type;				/** Address type. */
	token_t token;			 /** Pairing token. */
} nvs_t;


/**
* Time synchronisation.
*/
typedef struct __attribute__((packed)) _fujifilm_time_t {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} fujifilm_time_t;

/**
* Location and time packet.
*/
typedef struct __attribute__((packed)) _fujigeotag_t {
	int32_t latitude;
	int32_t longitude;
	int32_t altitude;
	uint8_t pad[4];
	fujifilm_time_t gps_time;
} geotag_t;

static int subscribe(struct PakBt *ctx, struct PakBtDevice *dev, const char *uuid_svc, const char *uuid_chr, int notif) {
	struct PakGattService *service = pak_bt_get_gatt_service_uuid(ctx, dev, uuid_svc);
	if (service == NULL) {
		pak_global_log("pak_bt_get_gatt_service_uuid");
		return PAK_ERR_UNSUPPORTED;
	}

	struct PakGattCharacteristic *chr = pak_bt_get_gatt_characteristic_uuid(ctx, service, uuid_chr);
	if (chr == NULL) {
		pak_global_log("pak_bt_get_gatt_characteristic_uuid");
		pak_bt_unref_gatt_service(ctx, service);
		return PAK_ERR_UNSUPPORTED;
	}

	if (pak_bt_set_watching_characteristic(ctx, chr, notif)) {
		pak_global_log("pak_bt_set_watching_characteristic");
	}

	if (pak_bt_set_cccd(ctx, chr, 0x1)) {
		pak_global_log("pak_bt_set_cccd");
	}

	pak_bt_unref_gatt_service(ctx, service);
	pak_bt_unref_gatt_characteristic(ctx, chr);
	return 0;
}

static int send_shutter_command(struct Module *mod, struct PakBtDevice *dev, const uint8_t *cmd, const uint8_t *params) {
	struct PakBt *ctx = mod->bt;
	struct PakGattService *service;
	if ((service = pak_bt_get_gatt_service_uuid(ctx, dev, SVC_SHUTTER_UUID)) == NULL) {
		return PAK_ERR_UNSUPPORTED;
	}
	struct PakGattCharacteristic *chr;
	if ((chr = pak_bt_get_gatt_characteristic_uuid(ctx, service, CHR_SHUTTER_UUID)) != NULL) {
		pak_bt_unref_gatt_service(ctx, service);
		return PAK_ERR_UNSUPPORTED;
	}
	pak_bt_write_characteristic(ctx, chr, cmd, 2, 1);
	pak_bt_write_characteristic(ctx, chr, params, 2, 1);
	pak_bt_unref_gatt_characteristic(ctx, chr);
	pak_bt_unref_gatt_service(ctx, service);
	return 0;
}

int fuji_bt_handle_command(struct Module *mod, struct PakBtDevice *dev, int argc, const char * const *argv) {
	if (!strcmp(argv[0], PAK_CMD_SHUTTER_DOWN)) {
		return send_shutter_command(mod, dev, SHUTTER_CMD, SHUTTER_PRESS);
	}
	if (!strcmp(argv[0], PAK_CMD_FOCUS_DOWN)) {
		return send_shutter_command(mod, dev, SHUTTER_CMD, SHUTTER_FOCUS);
	}
	if (!strcmp(argv[0], PAK_CMD_FOCUS_UP) || !strcmp(argv[0], PAK_CMD_SHUTTER_UP)) {
		return send_shutter_command(mod, dev, SHUTTER_CMD, SHUTTER_RELEASE);
	}
	return PAK_ERR_UNIMPLEMENTED;
}

static int device_callback(struct PakBt *ctx, enum PakBtEvent ev, struct PakBtDevice *dev, struct PakGattCharacteristic *chr, void *arg) {
	struct Module *mod = arg;
	if (ev == PAK_BT_EVENT_SERVICES_DISCOVERED) {
		pak_debug_log(mod, "Services discovered");
		pak_rt_set_progress_bar(mod, mod->priv->current_job, 30);
	} else if (ev == PAK_BT_EVENT_CONNECTED) {
		pak_debug_log(mod, "Connected");
		pak_rt_set_progress_bar(mod, mod->priv->current_job, 20);
	}
	return 0;
}

static int setup_misc_properties(struct Module *mod, struct PakBt *ctx, struct PakBtDevice *dev) {
	{
		struct PakGattService *service;
		if ((service = pak_bt_get_gatt_service_uuid(ctx, dev, "0000180a-0000-1000-8000-00805f9b34fb")) == NULL) {
			return PAK_ERR_UNSUPPORTED;
		}
		struct PakGattCharacteristic *chr;
		if ((chr = pak_bt_get_gatt_characteristic_uuid(ctx, service, "00002A26-0000-1000-8000-00805f9b34fb")) == NULL) {
			pak_bt_unref_gatt_service(ctx, service);
			return PAK_ERR_UNSUPPORTED;
		}
		pak_bt_read_characteristic(ctx, chr, 1);
		char buf[64];
		buf[pak_bt_read_characteristic_cached_value(ctx, chr, (uint8_t *)buf, sizeof(buf))] = '\0';
		pak_rt_set_session_property(mod, PAK_PROP_FW_VER, buf);
		pak_bt_unref_gatt_characteristic(ctx, chr);
		pak_bt_unref_gatt_service(ctx, service);
	}

	return 0;
}

int fuji_bluetooth_connect_to_wifi(struct Module *mod, struct PakBt *ctx, struct PakBtDevice *dev) {
	char wifi_ssid[64];
	struct PakGattService *service;
	struct PakGattCharacteristic *chr;
	{
		if ((service = pak_bt_get_gatt_service_uuid(ctx, dev, SVC_UNKNOWN1)) == NULL) {
			return PAK_ERR_UNSUPPORTED;
		}
		if ((chr = pak_bt_get_gatt_characteristic_uuid(ctx, service, CHR_UNKNOWN1_DEVICE_NAME)) == NULL) {
			pak_bt_unref_gatt_service(ctx, service);
			return PAK_ERR_UNSUPPORTED;
		}
		pak_bt_read_characteristic(ctx, chr, 1);
		wifi_ssid[pak_bt_read_characteristic_cached_value(ctx, chr, (uint8_t *)wifi_ssid, sizeof(wifi_ssid))] = '\0';
		pak_bt_unref_gatt_characteristic(ctx, chr);
		pak_bt_unref_gatt_service(ctx, service);
	}

	{
		if ((service = pak_bt_get_gatt_service_uuid(ctx, dev, SVC_SHUTTER_UUID)) == NULL) {
			return PAK_ERR_UNSUPPORTED;
		}
		if ((chr = pak_bt_get_gatt_characteristic_uuid(ctx, service, CHR_SHUTTER_UUID2)) == NULL) {
			pak_bt_unref_gatt_service(ctx, service);
			return PAK_ERR_UNSUPPORTED;
		}
		pak_bt_write_characteristic(ctx, chr, (uint8_t[]){0x4, 0x0}, 2, 1);
		pak_bt_unref_gatt_characteristic(ctx, chr);
		pak_bt_unref_gatt_service(ctx, service);
	}

	struct PakWiFiApFilter filter = {0};
	filter.has_ssid = 1;
	strlcpy(filter.ssid_pattern, wifi_ssid, sizeof(filter.ssid_pattern));

	pak_rt_add_wifi_connection(mod, &filter);

	return 0;
}

int fuji_connect_bluetooth(struct Module *mod, struct PakBt *ctx, struct PakBtDevice *dev, struct PakSavedConnection *saved) {
	pak_bt_set_device_callback(ctx, dev, device_callback, mod);

	pak_rt_set_progress_bar(mod, mod->priv->current_job, 10);

	char name_buf[32];
	adv_basic_t mfgdata;
	if (saved == NULL) {
		unsigned int sz = pak_bt_get_manufacturer_data(ctx, dev, 0, (uint8_t *)&mfgdata, sizeof(mfgdata));
		if (sz == 0) {
			pak_debug_log(mod, "Device is not in pairing mode");
			return PAK_ERR_NO_CONNECTION;
		}
		pak_debug_log(mod, "mfgdata sz: %u", sz);
		pak_debug_log(mod, "Token = %02x%02x%02x%02x", mfgdata.token.data[0], mfgdata.token.data[1], mfgdata.token.data[2], mfgdata.token.data[3]);
	} else {
		memcpy(mfgdata.token.data, saved->aux_data, TOKEN_LEN);
	}

	int rc = pak_bt_device_connect(ctx, dev);
	if (rc) {
		pak_debug_log(mod, "pak_bt_device_connect");
		return rc;
	}

	{
		struct PakGattService *service;
		if ((service = pak_bt_get_gatt_service_uuid(ctx, dev, GENERIC_ACCESS_SERVICE)) != NULL) {
			return PAK_ERR_UNSUPPORTED;
		}
		struct PakGattCharacteristic *chr;
		if ((chr = pak_bt_get_gatt_characteristic_uuid(ctx, service, DEVICE_NAME)) == NULL) {
			pak_bt_unref_gatt_service(ctx, service);
			return PAK_ERR_UNSUPPORTED;
		}
		pak_bt_read_characteristic(ctx, chr, 1);
		name_buf[pak_bt_read_characteristic_cached_value(ctx, chr, (uint8_t *)name_buf, sizeof(name_buf))] = '\0';
		pak_bt_unref_gatt_characteristic(ctx, chr);
		pak_bt_unref_gatt_service(ctx, service);

		pak_rt_set_session_property(mod, PAK_PROP_NAME, name_buf);
	}

	setup_misc_properties(mod, ctx, dev);

	struct PakGattService *pair_service;
	pair_service = pak_bt_get_gatt_service_uuid(ctx, dev, SVC_PAIR_UUID);
	if (pair_service == NULL) {
		pak_debug_log(mod, "pak_bt_get_gatt_service_uuid");
		return rc;
	}

	struct PakGattCharacteristic *pair_chr;
	pair_chr = pak_bt_get_gatt_characteristic_uuid(ctx, pair_service, CHR_PAIR_UUID);
	if (pair_chr == NULL) {
		pak_debug_log(mod, "pak_bt_get_gatt_characteristic_uuid");
		return PAK_ERR_UNSUPPORTED;
	}

	rc = pak_bt_write_characteristic(ctx, pair_chr, mfgdata.token.data, sizeof(mfgdata.token.data), 1);
	if (rc) {
		// TODO: Fails if 'SELECT PAIRING DESTINATION' setting on camera doesn't match this device
		pak_debug_log(mod, "pak_bt_write_characteristic");
		return rc;
	}

	pak_rt_set_progress_bar(mod, mod->priv->current_job, 40);

	char iden_str[0xff];
	struct PakGattCharacteristic *iden_chr;
	iden_chr = pak_bt_get_gatt_characteristic_uuid(ctx, pair_service, CHR_IDEN_UUID);
	if (iden_chr) {
		pak_debug_log(mod, "pak_bt_get_gatt_characteristic_uuid");
		return rc;
	}

	const char *client_name = pak_rt_get_client_name();
	rc = pak_bt_write_characteristic(ctx, iden_chr, (const uint8_t *)client_name, strlen(client_name), 1);
	if (rc) {
		pak_debug_log(mod, "pak_bt_write_characteristic");
		return rc;
	}

	pak_rt_set_progress_bar(mod, mod->priv->current_job, 50);

	subscribe(ctx, dev, "4e941240-d01d-46b9-a5ea-67636806830b", "bf6dc9cf-3606-4ec9-a4c8-d77576e93ea4", 1);
	subscribe(ctx, dev, SVC_CONF_UUID, CHR_IND1_UUID, 1);
	subscribe(ctx, dev, SVC_CONF_UUID, CHR_IND2_UUID, 1);
	pak_rt_set_progress_bar(mod, mod->priv->current_job, 60);
	subscribe(ctx, dev, SVC_CONF_UUID, CHR_NOT1_UUID, 1);
	pak_rt_set_progress_bar(mod, mod->priv->current_job, 70);
	subscribe(ctx, dev, SVC_CONF_UUID, GEOTAG_UPDATE, 1);
	pak_rt_set_progress_bar(mod, mod->priv->current_job, 80);
	subscribe(ctx, dev, SVC_CONF_UUID, CHR_IND3_UUID, 1);

	pak_rt_save_session_signature(mod, &(struct PakSavedConnection){
		.name = name_buf,
		.unique_id = dev->mac_address,
		.aux_data = (uint8_t *)mfgdata.token.data,
		.aux_data_length = sizeof(mfgdata.token.data),
	});

	{
		struct PakGattService *service;
		if ((service = pak_bt_get_gatt_service_uuid(ctx, dev, SVC_GEOTAG_UUID)) == NULL) {
			return PAK_ERR_UNSUPPORTED;
		}
		struct PakGattCharacteristic *chr;
		if ((chr = pak_bt_get_gatt_characteristic_uuid(ctx, service, CHR_GEOTAG_UUID)) == NULL) {
			pak_bt_unref_gatt_service(ctx, service);
			return PAK_ERR_UNSUPPORTED;
		}

		geotag_t geotag = {
			.latitude = (int32_t)(123 * 10000000),
			.longitude = (int32_t)(123 * 10000000),
			.altitude = (int32_t)50000,
			.pad = {0},
			.gps_time = {
				.year = (uint16_t)2026,
				.month = (uint8_t)6,
				.day = (uint8_t)5,
				.hour = (uint8_t)12,
				.minute = (uint8_t)43,
				.second = (uint8_t)12
			}
		};

		pak_bt_write_characteristic(ctx, chr, (uint8_t *)&geotag, sizeof(geotag), 1);

		pak_bt_unref_gatt_characteristic(ctx, chr);
		pak_bt_unref_gatt_service(ctx, service);
	}

	pak_rt_set_progress_bar(mod, mod->priv->current_job, 100);

	pak_rt_set_screen_supported(mod, SCREEN_INTERVALOMETER, 1);

	return 0;
}
