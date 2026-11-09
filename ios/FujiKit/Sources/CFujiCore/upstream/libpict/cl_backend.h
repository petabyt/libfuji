/** \file */ 
#ifndef CL_BACKEND_H
#define CL_BACKEND_H

/// @defgroup Backend Backend interface
/// @brief Layer over OS-dependent I/O interface
/// @addtogroup Backend
/// @{

/// @brief Linked-list entry for a single USB device
struct PtpDeviceEntry {
	struct PtpDeviceEntry *prev;

	int id;
	int vendor_id;
	int product_id;

	uint32_t endpoint_in;
	uint32_t endpoint_out;
	uint32_t endpoint_int;

	char name[16];
	char manufacturer[16];
	void *device_handle_ptr;

	struct PtpDeviceEntry *next;
};

/// @brief Get a linked list of USB or PTP Devices
/// @returns linked list of devices or NULL if no devices are connected (or OS error)
struct PtpDeviceEntry *ptpusb_device_list(struct PtpRuntime *r);

/// @brief Frees the device entry linked list
void ptpusb_free_device_list(struct PtpDeviceEntry *e);

/// @brief Open and connect to a device from the PtpDeviceEntry structure
/// @note Sets kill switch to 0
int ptp_device_open(struct PtpRuntime *r, struct PtpDeviceEntry *entry);

/// @brief Connects to the first PTP device it finds
int ptp_device_connect(struct PtpRuntime *r);

/// @brief Send data over the raw command endpoint for USB backend
int ptp_cmd_write(struct PtpRuntime *r, void *to, unsigned int length);
/// @brief Receive raw data over the command endpoint for USB backend
int ptp_cmd_read(struct PtpRuntime *r, void *to, unsigned int length);

/// @brief Reset the USB endpoints if possible
int ptp_device_reset(struct PtpRuntime *r);

/// @brief Send packets in r->data
int ptp_send_packet(struct PtpRuntime *r, unsigned int length);

/// @brief Receive all packets into r->data
int ptp_receive_all_packets(struct PtpRuntime *r);

/// @brief Poll the interrupt endpoint
int ptp_read_int(struct PtpRuntime *r, void *to, unsigned int length);

/// @brief Disconnect from the current device
int ptp_device_close(struct PtpRuntime *r);

/// @brief Connect to a TCP port on the default network adapter
/// @note Sets kill switch to 0
int ptpip_connect(struct PtpRuntime *r, const char *addr, int port, int extra_tmout);
/// @brief Send data over the raw command endpoint for USB backend
int ptpip_cmd_write(struct PtpRuntime *r, void *data, unsigned int size);
/// @brief Receive raw data over the command endpoint for USB backend
int ptpip_cmd_read(struct PtpRuntime *r, void *data, unsigned int size);
/// @brief Connect PTP/IP event socket. Port depends on implementation.
int ptpip_connect_events(struct PtpRuntime *r, const char *addr, int port);
int ptpip_event_send(struct PtpRuntime *r, void *data, unsigned int size);
int ptpip_event_read(struct PtpRuntime *r, void *data, unsigned int size);
int ptpip_video_read(struct PtpRuntime *r, void *data, unsigned int size);
/// @brief Disconnect from a PTP/IP device
int ptpip_device_close(struct PtpRuntime *r);
/// @brief shutdown() all PTP/IP sockets so a blocked read/write on another thread returns.
/// Does not close the descriptors; call ptpip_device_close() afterwards.
int ptpip_shutdown_io(struct PtpRuntime *r);

/// @brief Backend must implement this to free the device_handle_ptr field in each device entry
void ptpusb_free_device_list_entry(void *);

/// @brief Get status of currently connected device
/// This is a quick and cheap way to check if the connection is still established, which
/// can be used if there isn't a vendor event polling mechanism
int ptpusb_get_status(struct PtpRuntime *r);

/// @brief Initializes backend context
/// @note On Windows this may run CoInitialize on the current thread
int ptp_comm_init(struct PtpRuntime *r);

/// @brief Deinits backend and frees all memory
void ptp_comm_deinit(struct PtpRuntime *r);

/// @brief Deinits PTP/IP backend and frees all memory
void ptpip_comm_deinit(struct PtpRuntime *r);

// old api
__attribute__((deprecated))
__attribute__((unused))
static int ptp_device_init(struct PtpRuntime *r) {
	return ptp_device_connect(r);
}

/// @}

#endif
