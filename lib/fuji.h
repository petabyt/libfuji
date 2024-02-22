#ifndef FUJIAPP_FUJI_H
#define FUJIAPP_FUJI_H

#define DEVICE_NAME "Fudge"

// (Not a part of camlib)
void ptp_report_error(struct PtpRuntime *r, char *reason, int code);

int fuji_reset_ptp(struct PtpRuntime *r);

int fuji_setup_remote_mode(struct PtpRuntime *r, char *ip);

int fuji_setup(struct PtpRuntime *r, char *ip);

// Test suite stuff
int fuji_test_suite(struct PtpRuntime *r, char *ip);
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
int ptpip_connect_video(struct PtpRuntime *r, char *addr, int port);

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

#endif
