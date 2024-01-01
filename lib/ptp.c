#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <camlib.h>

#include "myjni.h"
#include "backend.h"
#include "fuji.h"
#include "fujiptp.h"

JNI_FUNC(jstring, cGetObjectInfo)(JNIEnv *env, jobject thiz, jint handle) {
	backend.env = env;

	struct PtpObjectInfo oi;
	rc = ptp_get_object_info(&backend.r, (int)handle, &oi);
	if (rc) {
		return NULL;
	}

	char buffer[1024];
	ptp_object_info_json(&oi, buffer, sizeof(buffer));

	jstring ret = (*env)->NewStringUTF(env, buffer);
	return ret;
}
