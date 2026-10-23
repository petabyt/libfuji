/** \file */
#ifndef OPERATIONS_H
#define OPERATIONS_H

enum PtpLiveViewType {
	PTP_LV_NONE = 0,
	PTP_LV_EOS = 1,
	PTP_LV_CANON = 2,
	// ptpview v1, Old 360x240 manually computed 7fps liveview
	PTP_LV_ML = 3,
	// ptpview v2, only has bmp menu graphics
	PTP_LV_EOS_ML_BMP = 4,
};

enum PtpLiveViewFormat {
	PTP_LV_JPEG,
};

struct PtpLiveviewParams {
	uint32_t payload_offset_to_data;
	enum PtpLiveViewFormat format;
};

/// @defgroup Operations PTP Operations
/// @brief PTP opcode functions - may also call unpack data structures
/// @addtogroup Operations
/// @{

int ptp_liveview_params(struct PtpRuntime *r, struct PtpLiveviewParams *params);

/// @brief Set a generic property - abstraction over SetDeviceProp
/// @note May reject writes if an invalid property is found (see event code)
int ptp_set_generic_property(struct PtpRuntime *r, const char *name, int value);

/// @brief Call before taking a picture - this is generally for 'focusing'
/// On some cameras this does nothing.
/// @note This is meant for a onMouseDown-like event. ptp_take_picture should be called on onMouseUp
int ptp_pre_take_picture(struct PtpRuntime *r);

/// @brief Call after calling ptp_pre_take_picture - this time a picture will be taken.
int ptp_take_picture(struct PtpRuntime *r);

/// @brief Open a new session - required for most commands
int ptp_open_session(struct PtpRuntime *r);

int ptp_close_session(struct PtpRuntime *r);

int ptp_get_device_info(struct PtpRuntime *r, struct PtpDeviceInfo *di);

/// @brief Returns allocated array of storage IDs
/// @note call free() afterward
int ptp_get_storage_ids(struct PtpRuntime *r, struct PtpArray **a);

int ptp_init_capture(struct PtpRuntime *r, int storage_id, int object_format);

int ptp_init_open_capture(struct PtpRuntime *r, int storage_id, int object_format);

int ptp_terminate_open_capture(struct PtpRuntime *r, int trans);

int ptp_get_storage_info(struct PtpRuntime *r, int id, struct PtpStorageInfo *si);

int ptp_send_object_info(struct PtpRuntime *r, int storage_id, int handle, struct PtpObjectInfo *oi);

int ptp_send_object(struct PtpRuntime *r, const void *data, unsigned int data_length);

int ptp_get_prop_value(struct PtpRuntime *r, int code);

/// @brief Sets 32 bit value of a property
int ptp_set_prop_value(struct PtpRuntime *r, int code, int value);
/// @brief Sets 16 bit value of a property
int ptp_set_prop_value16(struct PtpRuntime *r, int code, uint16_t value);

int ptp_set_prop_value_data(struct PtpRuntime *r, int code, void *data, unsigned int length);

int ptp_get_prop_desc(struct PtpRuntime *r, int code, struct PtpPropDesc *pd);

/// @brief Gets a list of object handles in a storage device or folder.
/// @param id storage ID
/// @param format Can specify file format ID, or zero for all IDs
/// @param in Can be folder object ID, or 0 for recursive (entire filesystem)
int ptp_get_object_handles(struct PtpRuntime *r, int id, int format, int in, struct PtpArray **a);

int ptp_get_object_info(struct PtpRuntime *r, uint32_t handle, struct PtpObjectInfo *oi);

int ptp_move_object(struct PtpRuntime *r, int storage_id, int handle, int folder);

int ptp_delete_object(struct PtpRuntime *r, int handle);

/// @brief Raw JPEG data is accessible from ptp_get_payload()
/// @note Not thread safe.
int ptp_get_thumbnail(struct PtpRuntime *r, int handle);

/// @note Not thread safe.
int ptp_get_partial_object(struct PtpRuntime *r, uint32_t handle, unsigned int offset, unsigned int max);

/// @brief Download an object
int ptp_get_object(struct PtpRuntime *r, int handle);

/// @brief Download an object from handle, to a local file (uses GetPartialObject)
int ptp_download_object(struct PtpRuntime *r, int handle, FILE *stream, size_t max);

/// @brief Recieve a generic list of all properties received in DeviceInfo
/// This is similar to getting all events, but for first startup when you know nothing.
/// Some vendors do this, but this gets all the properties manually.
/// @param[out] s Output structure, caller must free
int ptp_get_all_known(struct PtpRuntime *r, struct PtpGenericEvent **s, unsigned int *length);

/// @note PTP/IP only
int ptpip_init_events(struct PtpRuntime *r);

/// @note PTP/IP only
int ptpip_init_command_request(struct PtpRuntime *r, const char *device_name);

// EOS Only functions - not for Canon point and shoot
int ptp_eos_get_viewfinder_data(struct PtpRuntime *r);
int ptp_eos_set_remote_mode(struct PtpRuntime *r, int mode);
int ptp_eos_set_prop_value(struct PtpRuntime *r, int code, uint32_t value);
int ptp_eos_set_event_mode(struct PtpRuntime *r, int mode);
int ptp_eos_remote_release_off(struct PtpRuntime *r, int mode);
int ptp_eos_remote_release_on(struct PtpRuntime *r, int mode);
/// @brief Runs PTP_OC_EOS_GetEvent
int ptp_eos_get_event(struct PtpRuntime *r);
int ptp_eos_hdd_capacity_push(struct PtpRuntime *r);
int ptp_eos_hdd_capacity_pop(struct PtpRuntime *r);
int ptp_eos_get_prop_value(struct PtpRuntime *r, int code);
int ptp_eos_bulb_start(struct PtpRuntime *r);
int ptp_eos_bulb_stop(struct PtpRuntime *r);
int ptp_eos_set_ui_lock(struct PtpRuntime *r);
int ptp_eos_reset_ui_lock(struct PtpRuntime *r);
int ptp_eos_cancel_af(struct PtpRuntime *r);
// steps can be between -3 and 3
int ptp_eos_drive_lens(struct PtpRuntime *r, int steps);
int ptp_eos_ping(struct PtpRuntime *r);

// Get max expected size of liveview, for allocations only
int ptp_liveview_size(struct PtpRuntime *r);
// Runs vendor-specific commands for the liveview
int ptp_liveview_init(struct PtpRuntime *r);
int ptp_liveview_deinit(struct PtpRuntime *r);
// Get a frame directly into a buffer. Size is expected to be from ptp_liveview_size()
int ptp_liveview_frame(struct PtpRuntime *r, void *buffer);
int ptp_liveview_type(struct PtpRuntime *r);

// Get Magic Lantern transparent menus buffer - see https://github.com/petabyt/ptpview
int ptp_ml_init_bmp_lv(struct PtpRuntime *r);
int ptp_ml_get_bmp_lv(struct PtpRuntime *r, uint32_t **buffer_ptr);
int ptp_ml_get_liveview_v1(struct PtpRuntime *r);

int ptp_chdk_get_version(struct PtpRuntime *r);
int ptp_chdk_upload_file(struct PtpRuntime *r, char *input, char *dest);

// Canon advanced extensions - available in canon-adv.c
int ptp_eos_activate_command(struct PtpRuntime *r);
int ptp_eos_exec_evproc(struct PtpRuntime *r, void *data, int length, int expect_return);
int ptp_eos_evproc_run(struct PtpRuntime *r, char *fmt, ...);
int ptp_eos_evproc_return_data(struct PtpRuntime *r);
int ptp_eos_fa_get_build_version(struct PtpRuntime *r, char *buffer, unsigned int max);

/// @returns enum PtpGeneralError, 0 for 'camera is busy', or the size of the image in the payload
int ptp_eos_get_liveview(struct PtpRuntime *r);

/** @}*/

#endif
