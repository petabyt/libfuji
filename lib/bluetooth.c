#include <stdint.h>
#include <runtime.h>
#include <bluetooth.h>

// 0x4001
#define SVC_PAIR_UUID "91f1de68-dff6-466e-8b65-ff13b0f16fb8"
// 0x4042
#define CHR_PAIR_UUID "aba356eb-9633-4e60-b73f-f52516dbd671"
// 0x4012
#define CHR_IDEN_UUID "85b9163e-62d1-49ff-a6f5-054b4630d4a1"

// Subscriptions
#define SVC_CONF_UUID "4c0020fe-f3b6-40de-acc9-77d129067b14"

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

// Geo location characteristic
#define GEOTAG_UPDATE "ad06c7b7-f41a-46f4-a29a-712055319122"

#define SVC_GEOTAG_UUID "3b46ec2b-48ba-41fd-b1b8-ed860b60d22b"
#define CHR_GEOTAG_UUID "0f36ec14-29e5-411a-a1b6-64ee8383f090"

#define GENERIC_ACCESS_SERVICE "00001800-0000-1000-8000-00805f9b34fb"
#define DEVICE_NAME "00002a00-0000-1000-8000-00805f9b34fb"

#define TOKEN_LEN 4
#define TYPE_TOKEN 0x02

#define MAX_NAME (64)

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

int subscribe(struct PakBt *ctx, struct PakBtDevice *dev, const char *uuid_svc, const char *uuid_chr, int notif) {
	struct PakGattService service;
	int rc = pak_bt_get_gatt_service_uuid(ctx, dev, &service, uuid_svc);
	if (rc) {
		pak_global_log("pak_bt_get_gatt_service_uuid");
		return PAK_ERR_UNSUPPORTED;
	}

	struct PakGattCharacteristic chr;
	rc = pak_bt_get_gatt_characteristic_uuid(ctx, &service, &chr, uuid_chr);
	if (rc) {
		pak_global_log("pak_bt_get_gatt_characteristic_uuid");
		pak_bt_unref_gatt_service(ctx, &service);
		return PAK_ERR_UNSUPPORTED;
	}

	rc = pak_bt_set_watching_characteristic(ctx, &chr, notif);
	if (rc) {
		pak_global_log("pak_bt_set_watching_characteristic");
	}

	rc = pak_bt_set_cccd(ctx, &chr, 0x1);
	if (rc) {
		pak_global_log("pak_bt_set_cccd");
	}

	pak_bt_unref_gatt_service(ctx, &service);
	pak_bt_unref_gatt_characteristic(ctx, &chr);
	return 0;
}

int fuji_connect_bluetooth(struct Module *mod, struct PakBt *ctx, struct PakBtDevice *dev, struct PakSavedConnection *saved) {
	char name_buf[32];
	adv_basic_t mfgdata;
	if (saved == NULL) {
		unsigned int sz = pak_bt_get_manufacturer_data(ctx, dev, 0, (uint8_t *)&mfgdata, sizeof(mfgdata));
		if (sz == 0) {
			pak_global_log("Device is not in pairing mode");
			return PAK_ERR_NO_CONNECTION;
		}
		pak_global_log("mfgdata sz: %u", sz);
		pak_global_log("Token = %02x%02x%02x%02x", mfgdata.token.data[0], mfgdata.token.data[1], mfgdata.token.data[2],
			   mfgdata.token.data[3]);
	} else {
		memcpy(mfgdata.token.data, saved->aux_data, TOKEN_LEN);
	}

	int rc = pak_bt_device_connect(ctx, dev);
	if (rc) {
		pak_global_log("pak_bt_device_connect");
		return rc;
	}

	{
		struct PakGattService service;
		if (pak_bt_get_gatt_service_uuid(ctx, dev, &service, GENERIC_ACCESS_SERVICE)) {
			return PAK_ERR_UNSUPPORTED;
		}
		struct PakGattCharacteristic chr;
		if (pak_bt_get_gatt_characteristic_uuid(ctx, &service, &chr, DEVICE_NAME)) {
			pak_bt_unref_gatt_service(ctx, &service);
			return PAK_ERR_UNSUPPORTED;
		}
		pak_bt_read_characteristic(ctx, &chr, 1);
		pak_bt_read_characteristic_cached_value(ctx, &chr, (uint8_t *)name_buf, sizeof(name_buf));
		pak_bt_unref_gatt_characteristic(ctx, &chr);
		pak_bt_unref_gatt_service(ctx, &service);
	}

	pak_rt_set_session_property(mod, PAK_PROP_NAME, name_buf);

	struct PakGattService pair_service;
	rc = pak_bt_get_gatt_service_uuid(ctx, dev, &pair_service, SVC_PAIR_UUID);
	if (rc) {
		pak_global_log("pak_bt_get_gatt_service_uuid");
		return rc;
	}

	struct PakGattCharacteristic pair_chr;
	rc = pak_bt_get_gatt_characteristic_uuid(ctx, &pair_service, &pair_chr, CHR_PAIR_UUID);
	if (rc) {
		pak_global_log("pak_bt_get_gatt_characteristic_uuid");
		return PAK_ERR_UNSUPPORTED;
	}

	rc = pak_bt_write_characteristic(ctx, &pair_chr, mfgdata.token.data, sizeof(mfgdata.token.data), 1);
	if (rc) {
		pak_global_log("pak_bt_write_characteristic");
		return rc;
	}

	char iden_str[0xff];
	struct PakGattCharacteristic iden_chr;
	rc = pak_bt_get_gatt_characteristic_uuid(ctx, &pair_service, &iden_chr, CHR_IDEN_UUID);
	if (rc) {
		pak_global_log("pak_bt_get_gatt_characteristic_uuid");
		return rc;
	}
	char client_name[] = "Pixel-6a-1234";
	rc = pak_bt_write_characteristic(ctx, &iden_chr, (uint8_t *)client_name, sizeof(client_name) - 1, 1);
	if (rc) {
		pak_global_log("pak_bt_write_characteristic");
		return rc;
	}

	subscribe(ctx, dev, "4e941240-d01d-46b9-a5ea-67636806830b", "bf6dc9cf-3606-4ec9-a4c8-d77576e93ea4", 1);
	subscribe(ctx, dev, SVC_CONF_UUID, CHR_IND1_UUID, 1);
	subscribe(ctx, dev, SVC_CONF_UUID, CHR_IND2_UUID, 1);
	subscribe(ctx, dev, SVC_CONF_UUID, CHR_NOT1_UUID, 1);
	subscribe(ctx, dev, SVC_CONF_UUID, GEOTAG_UPDATE, 1);
	subscribe(ctx, dev, SVC_CONF_UUID, CHR_IND3_UUID, 1);

	pak_rt_save_session_signature(mod, &(struct PakSavedConnection){
		.name = name_buf,
		.unique_id = dev->mac_address,
		.aux_data = (uint8_t *)mfgdata.token.data,
		.aux_data_length = sizeof(mfgdata.token.data),
	});

	{
		struct PakGattService service;
		if (pak_bt_get_gatt_service_uuid(ctx, dev, &service, SVC_GEOTAG_UUID)) {
			return PAK_ERR_UNSUPPORTED;
		}
		struct PakGattCharacteristic chr;
		if (pak_bt_get_gatt_characteristic_uuid(ctx, &service, &chr, CHR_GEOTAG_UUID)) {
			pak_bt_unref_gatt_service(ctx, &service);
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

		pak_bt_write_characteristic(ctx, &chr, (uint8_t *)&geotag, sizeof(geotag), 1);

		pak_bt_unref_gatt_characteristic(ctx, &chr);
		pak_bt_unref_gatt_service(ctx, &service);
	}

	return 0;
}
