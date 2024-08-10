// Object Info service
#include <stdlib.h>
#include <camlib.h>
#include <string.h>

struct ObjectCache {
	int range_start;
	int *range;
	int length;
	int curr;
	struct PtpObjectInfo **objects;
};

static void ptp_object_service_add_object(struct ObjectCache *oc, struct PtpObjectInfo *oi) {
	if (oc->curr > oc->length) {
		abort();
	}
	if (oc->curr == oc->length) {
		oc->length += 100;
		oc->objects = realloc(oc->objects, sizeof(struct PtpObjectInfo *) * oc->length);
	}

	oc->objects[oc->curr] = malloc(sizeof(struct PtpObjectInfo));
	memcpy(oc->objects[oc->curr], oi, sizeof(struct PtpObjectInfo));

	oc->range[oc->curr] = 1;

	oc->curr++;
}

int ptp_object_service_step(struct PtpRuntime *r, struct ObjectCache *oc) {
	ptp_mutex_lock(r);

	struct PtpObjectInfo oi;
	int rc = ptp_get_object_info(r, oc->range_start + oc->curr, &oi);
	if (rc) {
		ptp_mutex_unlock(r);
		return rc;
	}

	ptp_object_service_add_object(oc, &oi);

	ptp_mutex_unlock(r);
}

struct PtpObjectInfo *ptp_object_service_get(struct PtpRuntime *r, struct ObjectCache *oc, int handle) {
	if (handle >= oc->length) {
		abort();
	}

	if (oc->range[handle - oc->range_start] == 1) {
		return oc->objects[handle - oc->range_start];
	} else {
		return NULL;
	}
}

struct ObjectCache *ptp_create_object_service(int start, int stop) {
	struct ObjectCache *oc = malloc(sizeof(struct ObjectCache));
	oc->length = 100;
	oc->curr = 0;
	oc->objects = malloc(sizeof(struct PtpObjectInfo *) * oc->length);

	oc->range_start = start;
	oc->range = calloc(sizeof(int) * (stop - start), 1);
}