#ifndef FUJIAPP_FUJI_H
#define FUJIAPP_FUJI_H
#include <camlib.h>
#include "fujiptp.h"
#include "app.h"

#define DEVICE_NAME "Fudge"

// For a long time transfers over 1mb worked but for one image
// X-A2 decided to freak out and stall. So, we have to do it the Fuji way :)
#define FUJI_MAX_PARTIAL_OBJECT 0x100000

/// @brief IP address used for all PTP connections
/// @note must free()
char *app_get_camera_ip(void);

/// @brief Get friendly client name
/// @note must free()
char *app_get_client_name(void);

struct NetworkHandle *ptp_get_network_info(struct PtpRuntime *r);

enum DiscoverUpdateMessages {
	FUJI_UM_GOT_FIRST_MESSAGE,
	FUJI_UM_CONNECTING_TO_NOTIFY_SERVER,
	FUJI_UM_STARTING_INVITE_SERVER,
	FUJI_UM_CAMERA_CONNETED_TO_INVITE_SERVER,
	FUJI_UM_ALL_DONE,
};

enum DiscoverRet {
	FUJI_D_REGISTERED = 1,
	FUJI_D_GO_PTP = 2,
	FUJI_D_CANCELED = 3,
	FUJI_D_IO_ERR = 4,
	// TODO: In the case that Fuji's software prevents us from listening to the camera, let the user know to kill it
	FUJI_D_OPEN_DENIED = 5,
	FUJI_D_INVALID_NETWORK = 6,
};

/// @brief Holds all information about a camera that has been detected (through any means)
struct DiscoverInfo {
	enum FujiTransport transport;
	struct NetworkHandle h;
	char camera_ip[64];
	char camera_name[64];
	char camera_model[64];
	char client_name[64];
	int camera_port;
	int vendor_id;
	int product_id;
};

/// @brief Holds runtime info about the camera
struct FujiDeviceKnowledge {
	/// @note applied from struct DiscoverInfo
	struct NetworkHandle net;
	/// @note applied from struct DiscoverInfo
	char ip_address[64];
	/// @note applied from struct DiscoverInfo
	enum FujiTransport transport;

	char autosave_client_name[64];
	int camera_state;
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

int fuji_connect_from_discoverinfo(struct PtpRuntime *r, struct DiscoverInfo *info);

/// @brief Do a weird hack where we GetPartialObject on the first few kb of a file, then grab the thumbnail
/// from the exif data. Not reliable.
int ptp_get_partial_exif(struct PtpRuntime *r, int handle, int *offset, int *length);

/// @brief Shut down a connection with an error code from struct PtpGeneralError. reason can be NULL. code can be zero for intentional disconnect
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

/// @brief Fun update messages from discovery service
/// @note to be defined by frontend
void fuji_discovery_update_progress(void *arg, enum DiscoverUpdateMessages progress);

/// @brief Initializes allocations for Fuji PTP session
int fuji_reset_ptp(struct PtpRuntime *r);

/// @brief Setup the event/liveview sockets for remote mode
int fuji_setup_remote_mode(struct PtpRuntime *r);

/// @brief Main entry function for PTP/IP
int fuji_setup(struct PtpRuntime *r);

int fuji_connection_entry(struct PtpRuntime *r);

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

/// @brief Set a prop needed to have the correct file size
int fuji_end_file_download(struct PtpRuntime *r);
/// @brief Unset the prop that fuji_end_file_download sets
int fuji_begin_file_download(struct PtpRuntime *r);

int fuji_begin_download_get_object_info(struct PtpRuntime *r, int handle, struct PtpObjectInfo *oi);

/// @brief Covers classic 'SELECT_MULTIPLE' feature found in 2013-2017 cams.
int fuji_download_classic(struct PtpRuntime *r);

/// @note Another socket on top of the 2 that camlib connects to
int ptpip_connect_video(struct PtpRuntime *r, const char *addr, int port);

/// Main entry function for all USB connections
int fujiusb_try_connect(struct PtpRuntime *r);
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

/// @brief Respects cancel signals.
/// @note If transport is FUJI_FEATURE_WIRELESS_COMM, compression property will be enabled after download
int fuji_download_file(struct PtpRuntime *r, int handle, int file_size, int (handle_add)(void *, void *, int, int), void *arg);

/// @brief Gets list of object handles regardless of transport
int ptp_fuji_get_object_handles(struct PtpRuntime *r, struct PtpArray **a);

/// @brief Download backup object (raw/conv mode only)
int fujiusb_download_backup(struct PtpRuntime *r, FILE *f);

/// @brief Function for 0x900c
int fuji_send_object_info_ex(struct PtpRuntime *r, int storage_id, int handle, struct PtpObjectInfo *oi);

/// @brief Function for 0x900d
int fuji_send_object_ex(struct PtpRuntime *r, const void *data, size_t length);

/// @param input_raf_path Path for RAF file
/// @param profile_xml String data for XML profile to be parsed by fp
int fuji_process_raf(struct PtpRuntime *r, const char *input_raf_path, const char *output_path, const char *profile_xml);

/// @brief CLI function to do quick conversion
int fudge_process_raf(const char *input, const char *output, const char *profile);

#endif
