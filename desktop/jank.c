// Jank code from testing Fuji PTP
// Their protocols SUCK!
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
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

// Trying to set camera properties like Fuji tetherapp does.
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
- PtpDeviceInfo FunctionalState is 0x02a8 (can't remember, vendor?)
- AFAIK gphoto people have not documented this mode. There were a few github issues about brokenness.
*/

// 		char buffer[90000];
// 		int i = 0;
// 		while (1) {
// 			ptp_set_prop_value(r, 0x5015, entries[i % 8]);
// 			i++;
// 			ptp_get_prop_value(r, 0x5015);
// 		
// 			struct PtpDeviceInfo di;
// 			rc = ptp_get_device_info(r, &di);
// 			if (rc) return rc;
// 			ptp_device_info_json(&di, buffer, sizeof(buffer));
// 			printf("%s\n", buffer);
// 
// 			ptp_get_prop_value(r, 0xd20b);
// 			//printf("%X: %d\n", 0xd20b, ptp_parse_prop_value(r));
// 			ptp_get_prop_value(r, 0xd212);
// 			//printf("%X: %d\n", 0xd212, ptp_parse_prop_value(r));
// 
// 
// 			usleep(1000 * 1000 * 2);
// 		}

	return 0;
}

// Fuji sets this property while doing pc autosave
// guess: client tells camera what photos are already downloaded
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

/*
- GetPartialObject and GetObjectInfo both refuse to send response if conditions are wrong
- Most of the time we can recover
- Is there a way to determine if we shouldn't download an image? No, ObjectInfo is basically the same.
- The max doesn't seem to matter.
- It works fine if we download the entire image. If we don't, or try and skip to the end, the next getpartialobject/getobjectinfo returns no
response.

Conclusion: GetPartialObject tracks the number of bytes read, and freaks out when attemping another object with previous unfinished
*/
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

	//rc = ptp_set_prop_value16(r, PTP_PC_FUJI_NoCompression, 1);

	 rc = fuji_get_events(r);
	 if (rc) return rc;


	int max = 0x100000/128;

//	if (handle == 15 || handle == 8 || handle == 7) max = 0x100000;

	r->wait_for_response = 2;
	rc = ptp_get_partial_object(r, handle, 0, max);
	if (rc == PTP_IO_ERR) return rc;
	//rc = ptp_get_partial_object(r, handle, 0xfffffff, 0xfffffff);

	rc = ptp_get_partial_object(r, handle, max, max * 2);
	if (rc == PTP_IO_ERR) return rc;

	 rc = fuji_get_events(r);
	 if (rc) return rc;
	 rc = fuji_get_events(r);
	 if (rc) return rc;
	 rc = fuji_get_events(r);
	 if (rc) return rc;
	 rc = fuji_get_events(r);
	 if (rc) return rc;

	/*
	300-200
	get event x1
	max = 0x100000/32
	partial object x2
	get event x4
	Took 67.956009 seconds
	- x4 -> x1: crash
	- x1/x3: 63.188999
	- x1/x2: 56.750000
	- x1/x1: crash
	- getobjinfo+x1/x3: 73... without getobjectinfo, 62.955002
	- get event x1/x1, usleep 100ms: 61.487000
	- 0 get event, 300ms: crash
	*/

	//

//	rc = ptp_get_partial_object(r, handle | (1 << 31), 0, 0x0);
//	rc = ptp_get_partial_object(r, handle | (1 << 31), 0, 0x0);

//	rc = fuji_get_events(r);
//	if (rc) return rc;

	//rc = ptp_get_partial_object(r, handle, 0x0fffffff, 0x0);

	//rc = ptp_get_partial_object(r, handle, oi.compressed_size - 0x100000, 0x100000);
	//if (rc == PTP_IO_ERR) return rc;

	//rc = ptp_set_prop_value16(r, PTP_PC_FUJI_NoCompression, 1);
	//rc = fuji_get_events(r);

	return 0;
}

int bench_exif_thumb(struct PtpRuntime *r) {
	float startTime = (float)clock()/CLOCKS_PER_SEC;

	int max = fuji_get(r)->num_objects;
	for (int i = max; i != max - 20; i--) {
		int rc = fuji_autosave_thumb(r, i);
		if (rc) {
			plat_dbg("Failed autosave thumb");
			return rc;
		}
	}

	float endTime = (float)clock()/CLOCKS_PER_SEC;
	
	plat_dbg("Took %f seconds", endTime - startTime);

	return 0;
}

int handle_add(void *arg, void *data, int size) {
	fwrite(data, size, 1, (FILE *)arg);
	return 0;
}

int try_download(struct PtpRuntime *r) {
	//plat_dbg("%d", fuji_get(r)->num_objects);
	int id = 1;

	bench_exif_thumb(r);

	struct PtpObjectInfo oi;
	int rc = ptp_get_object_info(r, id, &oi);
	if (rc) {
		return rc;
	}

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));
	app_print(buffer);

	FILE *f = fopen("TEST", "w");
	fuji_download_file(r, id, oi.compressed_size, handle_add, (void *)f);

	plat_dbg("Done downloading file");

	return 0;
}

/*
Fuji Wireless Tether property setting:
- PtpGetPropValue must be called after PtpSetPropValue
- If menu (or quick menu) is open, PtpSetPropValue will return DeviceBusy
- PtpDeviceInfo seems to include some data to indicate what properties have changed
*/
static int dump_prop(struct PtpRuntime *r) {
	struct PtpPropDesc pd;
	int rc = ptp_get_prop_desc(r, 0x5015, &pd);
	if (rc) return rc;
	char buffer[70000];
	ptp_prop_desc_json(&pd, buffer, sizeof(buffer));
	printf("%s\n", buffer);
	return 0;
}

int fuji_test_discovery(struct PtpRuntime *r) {
	struct DiscoverInfo info = {0};
	network_init();

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

	int rc = fuji_discover_thread(&info, "Fudge (desktop)", NULL);
	if (rc == FUJI_D_REGISTERED) {
		plat_dbg("Registered %s %s", info.camera_name, info.camera_model);
	} else if (rc == FUJI_D_GO_PTP) {
		usleep(100000); // TODO: Kind of have to wait before connecting

		printf("connecting to %s:%d\n", info.camera_ip, info.camera_port);
		if (ptpip_connect(r, info.camera_ip, info.camera_port)) {
			printf("Error connecting to %s:%d\n", info.camera_ip, info.camera_port);
			return 0;
		}

		fuji_reset_ptp(r);
		r->connection_type = PTP_IP_USB;
		fuji_get(r)->transport = info.transport;
		strcpy(fuji_get(ptp_get())->ip_address, info.camera_ip);

		//rc = fujitether_setup(r);
		rc = fuji_setup(r);
		if (rc) return rc;

		try_download(r);

		ptpip_close(r);
	} else {
		plat_dbg("Response code: %d\n", rc);
	}

	return -1;
}
