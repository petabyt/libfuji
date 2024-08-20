#ifndef FUJIAPP_FUJI_H
#define FUJIAPP_FUJI_H
#include <camlib.h>
#include "fujiptp.h"

#define DEVICE_NAME "Fudge"

// For a long time transfers over 1mb worked but for one image
// X-A2 decided to freak out and stall. So, we have to do it the Fuji way :)
#define FUJI_MAX_PARTIAL_OBJECT 0x100000

const char *app_get_camera_ip(void);
int app_do_connect_without_wifi(void);

enum DiscoverRet {
	FUJI_D_REGISTERED = 1,
	FUJI_D_GO_PTP = 2,
	FUJI_D_CANCELED = 3,
	FUJI_D_IO_ERR = 4,
	// TODO: In the case that Fuji's software prevents us from listening to the camera, let the user know to kill it
	FUJI_D_OPEN_DENIED = 5,
	FUJI_D_INVALID_NETWORK = 6,
};

struct DiscoverInfo {
	char camera_ip[64];
	char camera_name[64];
	char camera_model[64];
	char client_name[64];
	int camera_port;
	enum FujiTransport transport;
};

// Holds runtime info about the camera
struct FujiDeviceKnowledge {
	char ip_address[64];
	int camera_state;
	enum FujiTransport transport;
	int selected_imgs_mode;

	int get_object_version;
	int remote_image_view_version;
	int image_view_version;
	int image_get_version;
	int remote_version;

	int num_objects;

	int open_capture_trans_id;
};
struct FujiDeviceKnowledge *fuji_get(struct PtpRuntime *r);

/// @brief Do a weird hack where we GetPartialObject on the first few kb of a file, then grab the thumbnail
/// from the exif data. Not reliable. TODO: Move to camlib.c
int ptp_get_partial_exif(struct PtpRuntime *r, int handle, int *offset, int *length);

/// @note Not a part of camlib
void ptp_report_error(struct PtpRuntime *r, const char *reason, int code);

/// @note This will block
int fuji_discover_thread(struct DiscoverInfo *info, char *client_name, void *arg);
/// @brief Callback for discovery. Called when a new device wanting to pair is discovered. Return 1 if connection accepted
/// @note to be defined by frontend
int fuji_discover_ask_connect(void *arg, struct DiscoverInfo *info);
/// @brief Check if discovery is canceled
/// @note to be defined by frontend
int fuji_discovery_check_cancel(void *arg);
/// @brief Update the frontend on UI discovery progress, 0-7
/// @note to be defined by frontend
void fuji_discovery_update_progress(void *arg, int progress);

/// @brief Initializes allocations for Fuji PTP session
int fuji_reset_ptp(struct PtpRuntime *r);

/// @brief Setup the event/liveview sockets for remote mode
int fuji_setup_remote_mode(struct PtpRuntime *r);

/// @brief Main entry function for PTP/IP
int fuji_setup(struct PtpRuntime *r);

/// @brief Import files, based on an array of object IDs. Object Info will be fetched for each, mask will dictate if it's skipped or not.
int fuji_import_objects(struct PtpRuntime *r, int *object_ids, int length, int mask);

// Test suite stuff
int fuji_test_suite(struct PtpRuntime *r);
int fuji_test_setup(struct PtpRuntime *r);
int fuji_test_filesystem(struct PtpRuntime *r);

/// @brief Standard REQ/ACK for PTP/IP connection
int ptpip_fuji_init_req(struct PtpRuntime *r, char *device_name, struct PtpFujiInitResp *resp);

/// @brief Configure some mandatory viewer/gallery related version properties
int fuji_config_version(struct PtpRuntime *r);
/// @brief Determine and set what ClientState we need to be in
int fuji_config_init_mode(struct PtpRuntime *r);

/// @brief Configure the camera for the image viewer/gallery
int fuji_config_image_viewer(struct PtpRuntime *r);
/// @brief Fetch and parse initial device info
int fuji_config_device_info_routine(struct PtpRuntime *r);

int fuji_remote_mode_open_sockets(struct PtpRuntime *r);
int fuji_remote_mode_end(struct PtpRuntime *r);

/// @brief Poll events for setup event. Returns immediately if access is already granted.
int fuji_wait_for_access(struct PtpRuntime *r);

/// @brief Receives events once, and updates info struct with changes
int fuji_get_events(struct PtpRuntime *r);

// Enable/disable compression prop for downloading photos
int fuji_disable_compression(struct PtpRuntime *r);
int fuji_enable_compression(struct PtpRuntime *r);

/// @brief Covers classic 'SELECT_MULTIPLE' feature found in 2013-2017 cams.
int fuji_download_classic(struct PtpRuntime *r);

// Another socket on top of the 2 that camlib connects to
int ptpip_connect_video(struct PtpRuntime *r, const char *addr, int port);

/// Main entry function for all USB connections
int fujiusb_setup(struct PtpRuntime *r);
int fujitether_setup(struct PtpRuntime *r);

int fuji_register_device_info(struct PtpRuntime *r, uint8_t *data);

// Fuji (PTP/IP)
int ptp_fuji_get_init_info(struct PtpRuntime *r, struct PtpFujiInitResp *resp);
int ptp_fuji_parse_object_info(struct PtpRuntime *r, struct PtpFujiObjectInfo *oi);

/// @brief Download camera settings backup file
/// @note USB only, in backup/raw conv mode.
int fujiusb_download_backup(struct PtpRuntime *r, FILE *f);

#define PTP_SELET_JPEG (1 << 0)
#define PTP_SELET_RAW  (1 << 1)
#define PTP_SELET_MOV  (1 << 2)
#define PTP_SORT_NEWEST (1 << 3)
#define PTP_SORT_OLDEST (1 << 4)
#define PTP_SORT_LARGEST (1 << 5)
#define PTP_SORT_SMALLEST (1 << 6)

/// @brief Respects cancel signals.
/// @note If transport is FUJI_FEATURE_WIRELESS_COMM, compression property will be enabled after download
int fuji_download_file(struct PtpRuntime *r, int handle, int file_size, int (handle_add)(void *, void *, int, int), void *arg);

#endif
