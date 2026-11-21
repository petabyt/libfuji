// libpict USB backend for iOS: there is no libusb on iOS and third-party apps
// cannot claim arbitrary USB endpoints. USB access in the app goes through
// ImageCaptureCore (Swift side) instead. libpict's own src/no_usb.c at the
// pinned commit lacks ptpusb_device_list/ptp_device_open/
// ptpusb_free_device_list_entry, which libfuji's fuji_usb.c references.
#include <stddef.h>
#include <libpict.h>

struct PtpDeviceEntry *ptpusb_device_list(struct PtpRuntime *r) {
	(void)r;
	return NULL;
}

void ptpusb_free_device_list_entry(void *ptr) {
	(void)ptr;
}

int ptp_device_open(struct PtpRuntime *r, struct PtpDeviceEntry *entry) {
	(void)r; (void)entry;
	return PTP_UNSUPPORTED;
}

int ptp_device_connect(struct PtpRuntime *r) {
	(void)r;
	return PTP_UNSUPPORTED;
}

int ptp_device_close(struct PtpRuntime *r) {
	(void)r;
	return 0;
}

int ptp_device_reset(struct PtpRuntime *r) {
	(void)r;
	return PTP_UNSUPPORTED;
}

int ptp_cmd_write(struct PtpRuntime *r, void *to, unsigned int length) {
	(void)r; (void)to; (void)length;
	return PTP_IO_ERR;
}

int ptp_cmd_read(struct PtpRuntime *r, void *to, unsigned int length) {
	(void)r; (void)to; (void)length;
	return PTP_IO_ERR;
}

int ptp_read_int(struct PtpRuntime *r, void *to, unsigned int length) {
	(void)r; (void)to; (void)length;
	return PTP_IO_ERR;
}

int ptpusb_get_status(struct PtpRuntime *r) {
	(void)r;
	return PTP_UNSUPPORTED;
}

// Called by ptp_close(); free the PTP/IP backend state there.
void ptp_comm_deinit(struct PtpRuntime *r) {
	ptpip_comm_deinit(r);
}
