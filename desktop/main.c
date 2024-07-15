// Test basic opcode, get device properties
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <poll.h>
	#include <arpa/inet.h>
#endif
#include <camlib.h>
#include <fuji.h>
#include <app.h>
#include <fujiptp.h>
#include "desktop.h"

static struct PtpRuntime *ptp;

struct PtpRuntime *ptp_get() {
	return ptp;
}

int fuji_test_filesystem(struct PtpRuntime *r);
int fuji_test_setup(struct PtpRuntime *r);

void ptp_report_error(struct PtpRuntime *r, const char *reason, int code) {
	plat_dbg("Kill switch: %d tid: %d\n", r->io_kill_switch, getpid());
	if (r->io_kill_switch) return;
	r->io_kill_switch = 1;

	if (r->connection_type == PTP_IP_USB) {
		ptpip_close(r);
	} else if (r->connection_type == PTP_USB) {
		ptp_close(r);
	}

	fuji_reset_ptp(r);

	if (reason == NULL) {
		if (code == PTP_IO_ERR) {
			app_print("Disconnected: IO Error");
		} else {
			app_print("Disconnected: Runtime error");
		}
	} else {
		app_print("Disconnected: %s", reason);
	}
}

void ptp_verbose_log(char *fmt, ...) {
	printf("PTP: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__ ((noreturn))
void ptp_panic(char *fmt, ...) {
	printf("PTP abort: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	abort();
}

void plat_dbg(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	putchar('\n');
}

void tester_log(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	printf("LOG: %s\n", buffer);
}

void tester_fail(char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	printf("FAIL: %s\n", buffer);
}

int app_bind_socket_wifi(int sockfd) {
	return 0;
}

void app_increment_progress_bar(int read) {
	
}

int fudge_test(struct PtpRuntime *r) {
	if (ptp_device_init(r)) {
		puts("Device connection error");
		return 0;
	}

	int rc;

	ptp_open_session(r);

#if 0
	struct PtpPropDesc pd;
	rc = ptp_get_prop_desc(r, 0xd20b, &pd);

	printf("Prop data type: %d\n", ptp_get_payload_length(r));

	rc = ptp_get_prop_value(r, 0xD21c);
	if (rc) return rc;

	printf("Prop code: %x\n", ptp_parse_prop_value(r));
#endif

/*
Fuji Wireless/USB Tether:
- PtpGetPropValue must be called after PtpSetPropValue
- If menu (or quick menu) is open, PtpSetPropValue will return DeviceBusy
- PtpDeviceInfo seems to include some data to indicate what properties have changed (bitmap?)
- PtpDeviceInfo FunctionalState is 0x02a8
- AFAIK gphoto people have not documented this mode
*/
		char buffer[90000];
		int i = 0;
		while (1) {
			ptp_set_prop_value(r, 0x5015, entries[i % 8]);
			i++;
			ptp_get_prop_value(r, 0x5015);
		
			struct PtpDeviceInfo di;
			rc = ptp_get_device_info(r, &di);
			if (rc) return rc;
			ptp_device_info_json(&di, buffer, sizeof(buffer));
			printf("%s\n", buffer);

			ptp_get_prop_value(r, 0xd20b);
			//printf("%X: %d\n", 0xd20b, ptp_parse_prop_value(r));
			ptp_get_prop_value(r, 0xd212);
			//printf("%X: %d\n", 0xd212, ptp_parse_prop_value(r));


			usleep(1000 * 1000 * 2);
		}

	return 0;
}

void *fudge_backup_test(void *arg) {
	struct PtpRuntime *r = ptp_get();
	int rc;
	if (ptp_device_init(r)) {
		puts("Device connection error");
		return 0;
	}

	/*
	For some bizarre reason, Fuji X Acquire ended up doing something like:
	struct PtpDeviceInfo di;
	for (int i = 0; i < 20; i++) {
		rc = ptp_get_device_info(r, &di);
		rc = ptp_get_prop_value(r, 0xd20b);
	}
	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(r, 0, &oi);
	rc = ptp_get_object_info(r, 0, &oi);
	Before running GetObject. I'm guessing they have a background thread running GetDeviceInfo
	and GetPropValue 0xd20b and then I must have actually downloaded my settings a while after.
	*/

	// struct PtpDeviceInfo di;
	// for (int i = 0; i < 20; i++) {
	// 	rc = ptp_get_device_info(r, &di);
	// 	rc = ptp_get_prop_value(r, 0xd20b);
	// }

	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(r, 0, &oi);
	rc = ptp_get_object_info(r, 0, &oi);

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));
	app_print(buffer);

	app_print("Hello, WOrld\n");

	rc = ptp_get_object(r, 0);
	app_print("Downloaded payload %d bytes\n", ptp_get_payload_length(r));

	pthread_exit(NULL);
	return NULL;
}

int fudge_test_wifi(struct PtpRuntime *r) {
#ifdef WIN32
	// Windows wants to init this thread for socket stuff
	WSADATA wsaData = {0};
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	int rc = 0;
	char *ip_addr = "192.168.1.39";
	
	r->connection_type = PTP_IP_USB;
	if (ptpip_connect(r, ip_addr, FUJI_CMD_IP_PORT)) {
		printf("Error connecting to %s:%d\n", ip_addr, FUJI_CMD_IP_PORT);
		return 0;
	}
	
	rc = fuji_test_setup(r);
	if (rc) return rc;
	
	rc = fuji_test_filesystem(r);
	if (rc) return rc;
	
	ptp_close_session(r);
	
	ptpip_close(r);

	return 0;
}

int _fuji_d228(struct PtpRuntime *r) {
	char buffer[64];
	int s = 0;
	s += ptp_write_u8(buffer + s, 6);
	s += ptp_write_u32(buffer + s, 0x0020);
	s += ptp_write_u32(buffer + s, 0x0030);
	s += ptp_write_u32(buffer + s, 0x002f);
	s += ptp_write_u32(buffer + s, 0x0036);
	s += ptp_write_u32(buffer + s, 0x0030);
	s += ptp_write_u32(buffer + s, 0x0000);
	return ptp_set_prop_value_data(r, 0xd228, buffer, s);
}

int fuji_autosave_thumb(struct PtpRuntime *r, int handle) {
	int rc;

	tester_log("Object #%d...", handle);
#if 0
	struct PtpObjectInfo oi;
	r->wait_for_response = 1;
	rc = ptp_get_object_info(r, handle, &oi);
	if (rc == PTP_CHECK_CODE) {
		return 0;
	}
	if (rc) {
		tester_fail("Failed to get object info: %d", rc);
		return 0;
	} else {
		tester_log("Got object info");
	}

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));
	app_print(buffer);
#endif

//	rc = ptp_get_partial_object(r, handle, 0, 0x0);
//	rc = ptp_get_partial_object(r, handle, 0, 0x0);

//	_fuji_d228(r);

	rc = ptp_set_prop_value16(r, PTP_PC_FUJI_NoCompression, 0);

	int max = 0x100000/32;

//	if (handle == 15 || handle == 8 || handle == 7) max = 0x100000;

	r->wait_for_response = 2;
	rc = ptp_get_partial_object(r, handle, 0, max);
	if (rc == PTP_IO_ERR) return rc;
	//rc = ptp_get_partial_object(r, handle, 0xfffffff, 0xfffffff);

	rc = fuji_get_events(r);
	if (rc) return rc;
	rc = fuji_get_events(r);
	if (rc) return rc;

	//

//	rc = ptp_get_partial_object(r, handle | (1 << 31), 0, 0x0);
//	rc = ptp_get_partial_object(r, handle | (1 << 31), 0, 0x0);

//	rc = fuji_get_events(r);
//	if (rc) return rc;

	//rc = ptp_get_partial_object(r, handle, 0x0fffffff, 0x0);

	//rc = ptp_get_partial_object(r, handle, oi.compressed_size - 0x100000, 0x100000);
	//if (rc == PTP_IO_ERR) return rc;

	rc = ptp_set_prop_value16(r, PTP_PC_FUJI_NoCompression, 0);

	return 0;
}

/*
- GetPartialObject and GetObjectInfo both refuse to send response if conditions are wrong
- Most of the time we can recover
- Is there a way to determine if we shouldn't download an image? No, ObjectInfo is basically the same.
- The max doesn't seem to matter.
- It works fine if we download the entire image. If we don't, or try and skip to the end, the next getpartialobject/getobjectinfo returns no
response.

Conclusion: GetPartialObject tracks the number of bytes read, and freaks out when do another object with it unfinished
*/

int fuji_discover_ask_connect(void *arg, struct DiscoverInfo *info) {
	// Ask if we want to connect?
	return 1;
}

int fuji_discovery_check_cancel(void *arg) {
	return 0;
}

int fuji_test_discovery(struct PtpRuntime *r) {
	struct DiscoverInfo info = {0};

#ifdef WIN32
	// Windows wants to init this thread for socket stuff
	WSADATA wsaData = {0};
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

	int rc = fuji_discover_thread(&info, "Fudge (desktop)", NULL);
	if (rc == FUJI_D_REGISTERED) {
		plat_dbg("Registered %s %s", info.camera_name, info.camera_model);
	} else if (rc == FUJI_D_GO_PTP) {
		usleep(100000); // TODO: Kind of have to wait before connecting

		fuji_reset_ptp(r);
		r->connection_type = PTP_IP_USB;
		fuji_get(r)->transport = info.transport;
		printf("connecting to %s:%d\n", info.camera_ip, info.camera_port);
		if (ptpip_connect(r, info.camera_ip, info.camera_port)) {
			printf("Error connecting to %s:%d\n", info.camera_ip, info.camera_port);
			return 0;
		}

		if (info.transport == FUJI_FEATURE_WIRELESS_COMM) {
			rc = fuji_setup(r, info.camera_ip);
		} else if (info.transport == FUJI_FEATURE_WIRELESS_TETHER) {
			rc = fujitether_setup(r);
		}

		if (rc) return rc;

//		ptp_get_thumbnail(r, 10);

//		for (int i = 0; i < 100; i++) {
//			ptp_get_prop_value(r, 0xD001 + i);
//			printf("%X %d\n", 0xD001 + i, ptp_parse_prop_value(r));
//		}

		struct PtpPropDesc pd;
		rc = ptp_get_prop_desc(r, 0x5015, &pd);
		if (rc) return rc;
		char buffer[70000];
		ptp_prop_desc_json(&pd, buffer, sizeof(buffer));
		printf("%s\n", buffer);

//		ptp_get_prop_value(r, 0xD154);
//		printf("%X: %d\n", 0xD154, ptp_parse_prop_value(r));
		//ptp_get_prop_value(r, 0xd023);
		//printf("%X: %d\n", 0xd023, ptp_parse_prop_value(r));
		//ptp_set_prop_value(r, 0xd023, 1);


		int16_t entries[] = {0, 10, 20, 30};

/*
Fuji Wireless Tether:
- PtpGetPropValue must be called after PtpSetPropValue
- If menu (or quick menu) is open, PtpSetPropValue will return DeviceBusy
- PtpDeviceInfo seems to include some data to indicate what properties have changed
*/

		int i = 0;
		while (1) {
			ptp_set_prop_value(r, 0x5015, entries[i % 8]);
			i++;
			ptp_get_prop_value(r, 0x5015);
		
			struct PtpDeviceInfo di;
			rc = ptp_get_device_info(r, &di);
			if (rc) return rc;
			ptp_device_info_json(&di, buffer, sizeof(buffer));
			printf("%s\n", buffer);

			ptp_get_prop_value(r, 0xd20b);
			//printf("%X: %d\n", 0xd20b, ptp_parse_prop_value(r));
			ptp_get_prop_value(r, 0xd212);
			//printf("%X: %d\n", 0xd212, ptp_parse_prop_value(r));


			usleep(1000 * 1000 * 2);
		}

		// 65526

//		for (int i = 51; i > 0; i--) {
//			rc = fuji_autosave_thumb(r, i);
//			if (rc) return rc;
//		}

		ptpip_close(r);
	} else {
		plat_dbg("Response code: %d\n", rc);
	}

	return -1;
}

int main(int argc, char **argv) {
	ptp = ptp_new(PTP_USB);

	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-tw")) {
			return fudge_test_wifi(ptp);
		} else if (!strcmp(argv[i], "-tu")) {
			return fudge_test(ptp);
		} else if (!strcmp(argv[i], "-d")) {
			fuji_test_discovery(ptp);
			return 1;
		}
	}

	return fudge_main_ui();
}
