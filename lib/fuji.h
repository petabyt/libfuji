/** \file */
// Fudge implementations and app logic
#ifndef FUJIAPP_FUJI_H
#define FUJIAPP_FUJI_H
#include <libpict.h>
#include "fujiptp.h"
#include "app.h"

#define FUJI_MAX_PARTIAL_OBJECT 0x100000

enum DiscoverRet {
	FUJI_D_REGISTERED = 1,
	FUJI_D_GO_PTP = 2,
	FUJI_D_CANCELED = 3,
	FUJI_D_IO_ERR = 4,
	/// TODO: This should only be returned if Fuji's software (X Acquire, PC AutoSave) on another device connected to the camera before we could.
	/// In that case, the app should let the user know that and how to prevent it (uninstall the software, etc)
	FUJI_D_OPEN_DENIED = 5,
	FUJI_D_INVALID_NETWORK = 6,
};

/// @brief Holds all information about a camera that has been detected (through any means)
struct DiscoverInfo {
	enum FujiTransport transport;
	char camera_ip[64];
	char camera_name[64];
	char camera_model[64];
	char client_name[64];
	int camera_port;
	int vendor_id;
	int product_id;
};

/// Fujifilm priv struct for PtpRuntime->priv variable
struct PtpUserPriv {
	struct FujiModulePriv *priv;
	char ip_address[64];
	enum FujiTransport transport;

	/// @brief Current value of PTP_DPC_FUJI_CameraState_DF00
	int camera_state;
	/// @brief Current value of PTP_DPC_FUJI_SelectedImgsMode_D220
	int selected_imgs_mode;
	/// @brief Camera's initial value of PTP_DPC_FUJI_GetObjectVersion_DF22
	int get_object_version;
	/// @brief Camera's initial value of PTP_DPC_FUJI_RemoteGetObjectVersion_DF25
	int remote_image_view_version;
	/// @brief Camera's initial value of PTP_DPC_FUJI_RemotePhotoViewExVersion_DF28
	int remote_image_view_ex_version;
	/// @brief Camera's initial value of PTP_DPC_FUJI_ImageGetVersion_DF21
	int image_get_version;
	/// @brief Camera's initial value of PTP_DPC_FUJI_RemoteVersion_DF24
	int remote_version;
	// has liveview sockets been opened already
	int opened_liveview_sockets;
	int num_objects;
	char storage_device_name[64];
	int open_capture_trans_id;
	int allow_autosave_thumbnails;
};

typedef struct PtpUserPriv fujipriv_t;
fujipriv_t *fuji_get(struct PtpRuntime *r);

/// @brief Get a jpeg thumbnail for an object, through whatever means possible
/// Caller must wrap this in a mutex lock. Once returns, r->data will hold the thumb
/// with bounds set by `offset` and `length`
int fuji_get_thumb(struct PtpRuntime *r, int handle, unsigned int *offset, unsigned int *length);

/// @brief Shut down a connection with an error code from struct PtpGeneralError. reason can be NULL. code can be zero for intentional disconnect
/// @note Not a part of camlib
void ptp_report_error(struct PtpRuntime *r, const char *reason, int code);

/// @note This will block
int fuji_discover_thread(struct PtpRuntime *r, struct DiscoverInfo *info, const char *client_name);

/// @brief Check if discovery is canceled
/// @note to be defined by frontend
int fuji_discovery_check_cancel(struct PtpRuntime *r);

/// @brief New Fuji PTP session
struct PtpRuntime *fuji_ptp_new(int options);
int fuji_reset_ptp(struct PtpRuntime *r);

/// @brief Starts connection and configures everything for image gallery
/// fuji_config_image_gallery should not be called after this
int fuji_setup(struct PtpRuntime *r, const char *client_name);

/// @brief Perform REQ/ACK for PTP/IP connection
int ptpip_fuji_connect_handshake(struct PtpRuntime *r, const char *device_name, struct PtpFujiInitResp *resp);

/// @brief Enters liveview
int fuji_config_liveview(struct PtpRuntime *r);
/// @brief Exits liveview
int fuji_end_liveview(struct PtpRuntime *r);

/// @brief Enter image gallery
/// This should only be called after fuji_end_liveview
int fuji_config_image_gallery(struct PtpRuntime *r);

/// @brief Receives events once, and updates info struct with changes
int fuji_get_events(struct PtpRuntime *r);

/// @brief Covers classic 'SELECT_MULTIPLE' feature found in 2013-2017 cams.
int fuji_download_classic(struct PtpRuntime *r);

/// @note Another socket on top of the 2 that camlib connects to
int ptpip_connect_video(struct PtpRuntime *r, const char *addr, int port);

/// Main entry function for all USB connections
/// @param num Index of device to connect to, -1 to connect to first available device
int fujiusb_try_connect(struct PtpRuntime *r, int num);

/// @brief Sets up PTP session and tries to detect USB mode (fills in f->transport)
int fujiusb_setup(struct PtpRuntime *r);
int fujitether_setup(struct PtpRuntime *r, const char *client_name);

int fuji_register_device_info(struct PtpRuntime *r, uint8_t *data);

// Fuji (PTP/IP)
int ptp_fuji_parse_init_struct(struct PtpRuntime *r, struct PtpFujiInitResp *resp);

/// @brief Respects cancel signals.
/// @note If transport is FUJI_FEATURE_WIRELESS_COMM, compression property will be enabled after download
int fuji_download_file(struct PtpRuntime *r, int handle, int (handle_add)(void *arg, void *buf, unsigned int len, unsigned int offset, unsigned int total_size), void *arg);
int fuji_download_file_ex(struct PtpRuntime *r, int handle, int (info)(void *arg, struct PtpObjectInfo *oi), int (handle_add)(void *arg, void *buf, unsigned int len, unsigned int offset, unsigned int total_size), void *arg);

/// @brief Gets list of object handles regardless of transport
int ptp_fuji_get_object_handles(struct PtpRuntime *r, struct PtpArray **a);

/// @brief Function for 0x900c
int fuji_send_object_info_ex(struct PtpRuntime *r, int storage_id, int handle, struct PtpObjectInfo *oi);

/// @brief Function for 0x900d
int fuji_send_object_ex(struct PtpRuntime *r, const void *data, size_t length);

/// @param input_raf_path Path for RAF file
/// @param profile_xml_path String data for XML profile to be parsed by fp
int fuji_process_raf(struct PtpRuntime *r, const char *input_raf_path, const char *output_path, const char *profile_xml_path);

// Test suite stuff
int fuji_test_suite(struct PtpRuntime *r);
int fuji_test_setup(struct PtpRuntime *r);
int fuji_test_filesystem(struct PtpRuntime *r);

#endif
