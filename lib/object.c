// Camlib Object service
// This is mainly for file tables
#include <stdlib.h>
#include <camlib.h>
#include <string.h>
#include <pthread.h>
#include "object.h"

struct PtpObjectCache {
	/// @brief Should be locked while iterating the status list and other fields here
	pthread_mutex_t mutex;

	/// @brief Array of all objects to be downloaded
	struct CachedObject {
		/// @brief Has the ObjectInfo been downloaded yet
		int is_downloaded;
		/// @brief 1 if tried to get object info, but got error. Do not try to download again.
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
	return strcmp(((const struct CachedObject *)a)->info.date_created, ((const struct CachedObject *)a)->info.date_created);
}

static int sort_filename_a_z(const void *a, const void *b) {
	return strcmp(((const struct CachedObject *)a)->info.filename, ((const struct CachedObject *)a)->info.filename);
}

void ptp_object_service_sort(struct PtpRuntime *r, struct PtpObjectCache *oc, enum PtpSortBy s) {
	pthread_mutex_lock(&oc->mutex);

	if (s == PTP_SORT_BY_ALPHA_A_Z) {
		qsort(oc->status, sizeof(struct CachedObject), oc->status_length, sort_filename_a_z);
	}

	pthread_mutex_unlock(&oc->mutex);
}

void ptp_object_service_add_priority(struct PtpRuntime *r, struct PtpObjectCache *oc, int handle) {
	pthread_mutex_lock(&oc->mutex);

	for (int i = 0; i < oc->status_length; i++) {
		if (oc->status[i]->handle == handle) {
			oc->status[i]->is_priority = 1;
			break;
		}
	}

	pthread_mutex_unlock(&oc->mutex);
}

int ptp_object_service_step(struct PtpRuntime *r, struct PtpObjectCache *oc) {
	pthread_mutex_lock(&oc->mutex);

	if (oc->curr > oc->status_length) ptp_panic("Illegal state oc->curr");
	if (oc->curr == oc->status_length) {
		pthread_mutex_unlock(&oc->mutex);
		return 0;
	}

	int curr = oc->curr;
	for (int i = 0; i < oc->status_length; i++) {
		if (curr == i) {
			if (oc->status[i]->is_downloaded) {
				// If current object has already been downloaded, skip to next
				curr++;
				if (!(curr < oc->status_length)) {
					// If exhausted options, just give up
					pthread_mutex_unlock(&oc->mutex);
					return 0;
				}
			}
		}
		if (oc->status[i]->is_priority) {
			curr = i;
			break;
		}
	}

	// Save struct addr so the list can be modified while downloading
	struct CachedObject *obj = oc->status[curr];
	pthread_mutex_unlock(&oc->mutex);

	int rc = ptp_get_object_info(r, obj->handle, &obj->info);

	pthread_mutex_lock(&oc->mutex);
	if (rc == PTP_CHECK_CODE) {
		obj->is_downloaded = 0;
		obj->is_error = 1;
		oc->curr++;
		pthread_mutex_unlock(&oc->mutex);
		return 0;
	} else if (rc) {
		pthread_mutex_unlock(&oc->mutex);
		return rc;
	}

	obj->is_downloaded = 1;
	oc->num_downloaded++;

	if (oc->callback) {
		oc->callback(r, &obj->info, oc->arg);
	}

	oc->curr++;

	pthread_mutex_unlock(&oc->mutex);

	return obj->handle;
}

int ptp_object_service_length_filled(struct PtpRuntime *r, struct PtpObjectCache *oc) {
	return oc->num_downloaded;
}

int ptp_object_service_length(struct PtpRuntime *r, struct PtpObjectCache *oc) {
	return oc->status_length;
}

int ptp_object_service_get_handle_at(struct PtpRuntime *r, struct PtpObjectCache *oc, int index) {
	if (index > oc->status_length) return -1;
	return oc->status[index]->handle;
}

struct PtpObjectInfo *ptp_object_service_get_index(struct PtpRuntime *r, struct PtpObjectCache *oc, int req_i) {
	int count = 0;
	pthread_mutex_lock(&oc->mutex);
	for (int i = 0; i < oc->status_length; i++) {
		if (oc->status[i]->is_downloaded || oc->status[i]->is_error) {
			if (req_i == count) {
				pthread_mutex_unlock(&oc->mutex);
				// Not thread safe here
				if (oc->status[i]->is_error) return NULL;
				return &oc->status[i]->info;
			}
			count++;
		}
	}

	pthread_mutex_unlock(&oc->mutex);
	return NULL;
}

struct PtpObjectInfo *ptp_object_service_get(struct PtpRuntime *r, struct PtpObjectCache *oc, int handle) {
	pthread_mutex_lock(&oc->mutex);
	for (int i = 0; i < oc->status_length; i++) {
		if (oc->status[i]->handle == handle) {
			if (oc->status[i]->is_downloaded) {
				pthread_mutex_unlock(&oc->mutex);
				return &oc->status[i]->info;
			} else {
				pthread_mutex_unlock(&oc->mutex);
				return NULL;
			}
		}
	}
	pthread_mutex_unlock(&oc->mutex);
	return NULL;
}

void ptp_object_service_set(struct PtpRuntime *r, struct PtpObjectCache *oc, int handle, struct PtpObjectInfo *oi) {
	pthread_mutex_lock(&oc->mutex);
	for (int i = 0; i < oc->status_length; i++) {
		if (oc->status[i]->handle == handle) {
			memcpy(&oc->status[i]->info, oi, sizeof(struct PtpObjectInfo));
			if (oc->status[i]->is_downloaded == 0) {
				oc->num_downloaded++;
				oc->status[i]->is_downloaded = 1;
			}
			// This assumes the new object info is different, so update the list
			if (oc->callback) {
				oc->callback(r, &oc->status[i]->info, oc->arg);
			}
			break;
		}
	}
	pthread_mutex_unlock(&oc->mutex);
}

struct PtpObjectCache *ptp_create_object_service(int *handles, int length, ptp_object_found_callback *callback, void *arg) {
	struct PtpObjectCache *oc = malloc(sizeof(struct PtpObjectCache));
	oc->callback = callback;
	oc->status_length = length;
	oc->curr = 0;
	oc->num_downloaded = 0;
	oc->status = malloc(sizeof(struct CachedObject *) * length);
	for (int i = 0; i < length; i++) {
		struct CachedObject *os = (struct CachedObject *)malloc(sizeof(struct CachedObject));
		os->handle = handles[i];
		os->is_downloaded = 0;
		os->is_error = 0;
		os->is_priority = 0;
		oc->status[i] = os;
	}

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

	if (pthread_mutex_init(&oc->mutex, &attr)) {
		ptp_panic("Failed to init mutex\n");
	}

	return oc;
}

void ptp_free_object_service(struct PtpObjectCache *oc) {
	for (int i = 0; i < oc->status_length; i++) {
		free(oc->status[i]);
	}
	free(oc->status);
	pthread_mutex_destroy(&oc->mutex);
	free(oc);
}
