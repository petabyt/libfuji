#ifndef FUJIAPP_FUJI_H
#define FUJIAPP_FUJI_H
#include <camlib.h>
#include "fujiptp.h"

#define DEVICE_NAME "Fudge"

// For a long time transfers over 1mb worked but for one image
// X-A2 decided to freak out and stall. So, we have to do it the Fuji way :)
#define FUJI_MAX_PARTIAL_OBJECT 0x100000

struct DiscoverInfo {
	char camera_ip[64];
	char camera_name[64];
	char camera_model[64];
	char client_name[64];
};

enum DiscoverRet {
	FUJI_D_REGISTERED = 1,
	FUJI_D_GO_PTP = 2,
	FUJI_D_CANCELED = 3,
};

int ptp_dirty_rotten_thumb_hack(struct PtpRuntime *r, int handle, int *offset, int *length);

int fuji_discover_thread(struct DiscoverInfo *info, char *client_name);

// (Not a part of camlib)
void ptp_report_error(struct PtpRuntime *r, const char *reason, int code);

int fuji_reset_ptp(struct PtpRuntime *r);

int fuji_setup_remote_mode(struct PtpRuntime *r, const char *ip);

int fuji_setup(struct PtpRuntime *r, const char *ip);

// Test suite stuff
int fuji_test_suite(struct PtpRuntime *r, const char *ip);
int fuji_test_setup(struct PtpRuntime *r);
int fuji_test_filesystem(struct PtpRuntime *r);

// Send init packet, receive response
int ptpip_fuji_init_req(struct PtpRuntime *r, char *device_name, struct PtpFujiInitResp *resp);

int fuji_config_version(struct PtpRuntime *r);
int fuji_config_init_mode(struct PtpRuntime *r);

int fuji_config_image_viewer(struct PtpRuntime *r);
int fuji_config_device_info_routine(struct PtpRuntime *r);

int fuji_remote_mode_open_sockets(struct PtpRuntime *r);
int fuji_remote_mode_end(struct PtpRuntime *r);

int fuji_wait_for_access(struct PtpRuntime *r);

// Receives events once, and updates info struct with changes
int fuji_get_events(struct PtpRuntime *r);

// Enable/disable compression prop for downloading photos
int fuji_disable_compression(struct PtpRuntime *r);
int fuji_enable_compression(struct PtpRuntime *r);

int fuji_download_multiple(struct PtpRuntime *r);

// Another socket on top of the 2 that camlib connects to
int ptpip_connect_video(struct PtpRuntime *r, const char *addr, int port);

// Holds runtime info about the camera
struct FujiDeviceKnowledge {
	struct FujiCameraInfo *info;
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

extern struct FujiDeviceKnowledge fuji_known;

// Fuji USB
int fujiusb_setup(struct PtpRuntime *r);

int fuji_register_device_info(struct PtpRuntime *r, uint8_t *data);

// Fuji (PTP/IP)
int ptp_fuji_get_init_info(struct PtpRuntime *r, struct PtpFujiInitResp *resp);
int ptp_fuji_parse_object_info(struct PtpRuntime *r, struct PtpFujiObjectInfo *oi);

#endif
