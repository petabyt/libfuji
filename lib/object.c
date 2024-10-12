// Camlib Object service
// This is mainly for file tables
#include <stdlib.h>
#include <camlib.h>
#include <string.h>
#include "object.h"

struct ObjectCache {
	/// @brief Array of all objects to be downloaded
	struct ObjectStatus {
		/// @brief Has the object been downloaded yet
		int is_downloaded;
		/// @brief Tried to get object info, but got error. Do not try to download again.
		int is_error;
		/// @brief Object handle
		int handle;
		/// @brief Check if this object has a priority. TODO: Move to list
		int is_priority;
		/// @brief Not filled in if is_downloaded is 0
		struct PtpObjectInfo info;
	}**status;
	int status_length;

	/// @brief Current number of objects that have been downloaded
	int num_downloaded;
	/// @brief Current Object index to work on
	int curr;

	ptp_object_found_callback *callback;
	void *arg;
};

static int sort_date_newest(const void *a, const void *b) {
	// TODO: check null
	return strcmp(((const struct ObjectStatus *)a)->info.date_created, ((const struct ObjectStatus *)a)->info.date_created);
}

static int sort_filename_a_z(const void *a, const void *b) {
	// TODO: check null
	return strcmp(((const struct ObjectStatus *)a)->info.filename, ((const struct ObjectStatus *)a)->info.filename);
}

void ptp_object_service_sort(struct PtpRuntime *r, struct ObjectCache *oc, enum PtpSortBy s) {
	ptp_mutex_lock(r);

	if (s == PTP_SORT_BY_ALPHA_A_Z) {
		qsort(oc->status, sizeof(struct ObjectStatus), oc->status_length, sort_filename_a_z);
	}

	ptp_mutex_unlock(r);
}

void ptp_object_service_add_priority(struct PtpRuntime *r, struct ObjectCache *oc, int handle) {
	ptp_mutex_lock(r);

	for (int i = 0; i < oc->status_length; i++) {
		if (oc->status[i]->handle == handle) {
			oc->status[i]->is_priority = 1;
			break;
		}
	}

	ptp_mutex_unlock(r);
}

int ptp_object_service_step(struct PtpRuntime *r, struct ObjectCache *oc) {
	ptp_mutex_lock(r);

	if (oc->curr >= oc->status_length) abort();
	if (oc->curr == oc->status_length - 1) {
		ptp_mutex_unlock(r);
		return 0;
	}

	int curr = oc->curr;
	for (int i = 0; i < oc->status_length; i++) {
		if (oc->status[i]->is_priority) {
			curr = i;
			break;
		}
	}

	int handle = oc->status[curr]->handle;

	int rc = ptp_get_object_info(r, handle, &oc->status[curr]->info);
	if (rc == PTP_CHECK_CODE) {
		oc->status[curr]->is_downloaded = 0;
		oc->status[curr]->is_error = 1;
		oc->curr++;
		ptp_mutex_unlock(r);
		return 0;
	}
	if (rc) {
		ptp_mutex_unlock(r);
		return rc;
	}

	oc->status[curr]->is_downloaded = 1;
	oc->num_downloaded++;

	if (oc->callback) {
		oc->callback(r, &oc->status[curr]->info, oc->arg);
	}

	oc->curr++;

	ptp_mutex_unlock(r);

	return 1; // downloaded 1 object
}

int ptp_object_service_length(struct PtpRuntime *r, struct ObjectCache *oc) {
	return oc->num_downloaded;
}

struct PtpObjectInfo *ptp_object_service_get_index(struct PtpRuntime *r, struct ObjectCache *oc, int req_i) {
	int count = 0;
	ptp_mutex_lock(r);
	for (int i = 0; i < oc->status_length; i++) {
		if (oc->status[i]->is_downloaded || oc->status[i]->is_error) {
			if (req_i == count) {
				ptp_mutex_unlock(r);
				// Not thread safe here
				if (oc->status[i]->is_error) return NULL;
				return &oc->status[i]->info;
			}
			count++;
		}
	}

	ptp_mutex_unlock(r);
	return NULL;
}

struct PtpObjectInfo *ptp_object_service_get(struct PtpRuntime *r, struct ObjectCache *oc, int handle) {
	ptp_mutex_lock(r);
	for (int i = 0; i < oc->status_length; i++) {
		if (oc->status[i]->handle == handle) {
			if (oc->status[i]->is_downloaded) {
				ptp_mutex_unlock(r);
				return &oc->status[i]->info;
			} else {
				ptp_mutex_unlock(r);
				return NULL;
			}
		}
	}
	ptp_mutex_unlock(r);
	return NULL;
}

struct ObjectCache *ptp_create_object_service(int *handles, int length, ptp_object_found_callback *callback, void *arg) {
	struct ObjectCache *oc = malloc(sizeof(struct ObjectCache));
	oc->callback = callback;
	oc->status_length = length;
	oc->curr = 0;
	oc->num_downloaded = 0;
	oc->status = malloc(sizeof(struct ObjectStatus *) * length);
	for (int i = 0; i < length; i++) {
		struct ObjectStatus *os = (struct ObjectStatus *)malloc(sizeof(struct ObjectStatus));
		os->handle = handles[i];
		os->is_downloaded = 0;
		os->is_error = 0;
		os->is_priority = 0;
		oc->status[i] = os;
	}

	return oc;
}

void ptp_free_object_service(struct ObjectCache *oc) {
	for (int i = 0; i < oc->status_length; i++) {
		free(oc->status[i]);
	}
	free(oc->status);
	free(oc);
}
