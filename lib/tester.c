// Test suite for Android - this is dev code, it doesn't need to be tidy perfect
// Copyright 2023 (c) Unofficial fujiapp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <jni.h>
#include <camlib.h>
#include <android/log.h>

#include "jni.h"
#include "backend.h"
#include "fuji.h"
#include "models.h"

// Init commands 
JNI_FUNC(void, cTesterInit)(JNIEnv *env, jobject thiz, jobject tester) {
    backend.env = env;
    jclass thizClass = (*env)->GetObjectClass(env, thiz);
    jclass testerClass = (*env)->GetObjectClass(env, tester);
    backend.tester = (*env)->NewGlobalRef(env, tester);

    backend.tester_log = (*backend.env)->GetMethodID(backend.env, testerClass, "log", "(Ljava/lang/String;)V");
    backend.tester_fail = (*backend.env)->GetMethodID(backend.env, testerClass, "fail", "(Ljava/lang/String;)V");
}

void tester_log(char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    jni_verbose_log(buffer);

    (*backend.env)->CallVoidMethod(backend.env, backend.tester, backend.tester_log, (*backend.env)->NewStringUTF(backend.env, buffer));
}

void tester_fail(char *fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

	jni_verbose_log(buffer);

    (*backend.env)->CallVoidMethod(backend.env, backend.tester, backend.tester_fail, (*backend.env)->NewStringUTF(backend.env, buffer));
}

static void log_payload(struct PtpRuntime *r) {
	char buffer[512];
	uint8_t *data = ptp_get_payload(r);
	int length = ptp_get_payload_length(r);

	if (length == 0) {
		tester_log("No payload");
		return;
	}

	int c = sprintf(buffer, "Payload: ");
	for (int i = 0; i < length; i++) {
		c += sprintf(buffer + c, "%02X ", data[i]);
		if (c - 10 > sizeof(buffer)) return;
	}

	tester_log(buffer);
}

int fuji_test_get_props(struct PtpRuntime *r) {
	uint16_t test_props[] = {
		PTP_PC_FUJI_Unlocked,
		PTP_PC_FUJI_Mode,
		PTP_PC_FUJI_PhotoGetVersion,
		PTP_PC_FUJI_FunctionVersion,
		PTP_PC_FUJI_Unknown10,
		PTP_PC_FUJI_RemoteVersion,
		PTP_PC_FUJI_RemotePhotoViewVersion,
		PTP_PC_FUJI_PhotoRecieveReservedVersion,
		PTP_PC_FUJI_VersionGPS,
		PTP_PC_FUJI_PartialSize,
	};

	for (int i = 0; i < (int)(sizeof(test_props) / sizeof(uint16_t)); i++) {
		int rc = ptp_get_prop_value(r, test_props[i]);
		if (rc) {
			tester_fail("Err getting prop 0x%X - rc: %d", test_props[i], rc);
			return rc;
		} else {
			tester_log("Read property 0x%X %s (%d bytes)", test_props[i], ptp_get_enum_all(test_props[i]), ptp_get_payload_length(r));
		}

		log_payload(r);
	}

	return 0;
}

int fuji_test_init_access(struct PtpRuntime *r) {
	tester_log("Waiting for device access...");
	int rc = fuji_wait_for_access(r);
	if (rc) {
		tester_fail("Error trying to gain device access: %d", rc);
		return rc;
	} else {
		tester_log("Gained access to device (or already have access)");
	}

	return 0;
}

int fuji_init_setup(struct PtpRuntime *r) {
	tester_log("Configuring mode property");

	int rc = fuji_config_init_mode(r);
	if (rc) {
		tester_fail("Failed to setup mode: %d", rc);
		return rc;
	} else {
		tester_log("Mode property is configured.");
	}

	tester_log("Configuring FunctionMode");
	rc = fuji_config_version(r);
	if (rc) {
		tester_fail("Failed to configure FunctionMode: %d", rc);
		return rc;
	} else {
		tester_log("Configured FunctionMode, no errors detected (yet)");
	}

	rc = fuji_config_device_info_routine(r);
	if (rc == PTP_CHECK_CODE) {
		tester_fail("Camera didn't like the device info test, will keep on going");
	} else if (rc) {
		tester_fail("Failed to try device info routine");
		return rc;
	} else {
		tester_log("Device passed device info routine");
	}

	rc = fuji_config_remote_photo_viewer(r);
	if (rc) {
		tester_fail("Failed to config remote photo viewier");
		return rc;
	} else {
		tester_log("Configured remote photo viewer");
	}

	return 0;
}

int fuji_test_filesystem(struct PtpRuntime *r) {
	tester_log("Trying to get SD card info...");

	struct UintArray *arr;
	int rc = ptp_get_storage_ids(r, &arr);
	if (rc) {
		tester_fail("Failed to get storage devices: %d", rc);
		return rc;
	}

	if (arr->length == 0) {
		tester_fail("No storage devices found!");
		return 0;
	} else {
		tester_log("Found %d storage device(s).", arr->length);
	}

	int id = arr->data[0];

	struct PtpStorageInfo so;
	rc = ptp_get_storage_info(r, id, &so);
	if (rc) {
		tester_fail("Failed to obtain storage info for %X: %d", id, rc);
		return rc;
	}

	char buffer[512];
	ptp_storage_info_json(&so, buffer, sizeof(buffer));

	tester_log("storage info: %s\n", buffer);

	tester_log("Attempting to get object info for 1...");
	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(r, 1, &oi);
	if (rc) {
		tester_fail("Failed to get object info: %d", rc);
		return rc;
	}

	log_payload(r);

	ptp_object_info_json(&oi, buffer, sizeof(buffer));

	tester_log("Object info: %s\n", buffer);

	return 0;
}

// Portable Fujifilm test suite
int fuji_test_suite(struct PtpRuntime *r) {
	tester_log("Running test suite from C");

    int rc = ptpip_fuji_init(&backend.r, "fujiapp-test");
    if (rc) {
    	tester_fail("Failed to initialize command socket");
    } else {
    	tester_log("Initialized command socket");
    }

    struct PtpFujiInitResp resp;
    ptp_fuji_get_init_info(r, &resp);

    tester_log("Connected to %s", resp.cam_name);

    fuji_known.info = fuji_get_model_info(resp.cam_name);
	if (fuji_known.info == NULL) {
		tester_fail("Couldn't get model info from database");
	} else {
		if (fuji_known.info->gps_support)
			tester_log("- Supports gps");
		if (fuji_known.info->has_bluetooth)
			tester_log("- Has bluetooth");
		if (fuji_known.info->capture_support)
			tester_log("- Supports remote capture");
		if (fuji_known.info->firm_update_support)
			tester_log("- Supports firmware updates");
	}

	tester_log("sleep 500ms for good measure...");
    CAMLIB_SLEEP(500);

	rc = ptp_open_session(r);
	if (rc) {
		tester_fail("Failed to open session");
		return rc;
	} else {
		tester_log("Opened session");
	}
	
	rc = ptpip_fuji_get_events(r);
	if (rc) {
		tester_fail("Failed to get events after opening session: %d", rc);
		return rc;
	} else {
		tester_log("Recieved events after opening session.");
		log_payload(r);
	}

	rc = fuji_test_get_props(r);
	if (rc) return rc;

	rc = fuji_test_init_access(r);
	if (rc) return rc;

	rc = fuji_init_setup(r);
	if (rc) return rc;

	rc = fuji_test_filesystem(r);
	if (rc) return rc;

	tester_log("We got to the end of the test, and nothing broke :)");
	tester_log("Ending test...");

	return 0;
}

JNI_FUNC(jint, cFujiTestSuite)(JNIEnv *env, jobject thiz) {
	backend.env = env;
	return fuji_test_suite(&backend.r);
}
