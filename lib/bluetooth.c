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

// Shutter characteristic
#define CHR_SHUTTER_UUID "7fcf49c6-4ff0-4777-a03d-1a79166af7a8"

// Geo location characteristic
#define GEOTAG_UPDATE "ad06c7b7-f41a-46f4-a29a-712055319122"

#define SVC_GEOTAG_UUID "3b46ec2b-48ba-41fd-b1b8-ed860b60d22b"
#define CHR_GEOTAG_UUID "0f36ec14-29e5-411a-a1b6-64ee8383f090"

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

int fuji_connect_bluetooth(struct PakBt *ctx, struct PakBtDevice *dev) {
	adv_basic_t mfgdata;
	unsigned int sz = pak_bt_get_manufacturer_data(ctx, dev, 0, (uint8_t *)&mfgdata, sizeof(mfgdata));

	int rc = pak_bt_device_connect(ctx, dev);
	if (rc) return rc;

	struct PakGattService service;
	rc = pak_bt_get_gatt_service_uuid(ctx, dev, &service, SVC_PAIR_UUID);
	if (rc) return rc;

	struct PakGattCharacteristic chr;
	rc = pak_bt_get_gatt_characteristic_uuid(ctx, &service, &chr, CHR_PAIR_UUID);
	if (rc) return PAK_ERR_UNSUPPORTED;

	rc = pak_bt_write_characteristic(ctx, &chr, mfgdata.token.data, sizeof(mfgdata.token.data), 1);
	if (rc) return rc;

	char iden_str[0xff];
	struct PakGattCharacteristic iden_chr;
	rc = pak_bt_get_gatt_characteristic_uuid(ctx, &service, &iden_chr, CHR_IDEN_UUID);
	if (rc) return rc;
	pak_bt_read_characteristic(ctx, &iden_chr, 1);
	pak_bt_read_characteristic_cached_value(ctx, &iden_chr, (uint8_t *)iden_str, sizeof(iden_str));
	pak_global_log("%s\n", iden_str);

	return -1;
}
