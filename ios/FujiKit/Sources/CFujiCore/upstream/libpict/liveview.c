// Liveview wrappers - headers defined in cl_ops.h
// Copyright 2022 by Daniel C (https://github.com/petabyt/libpict)
// Also see ml.c for Magic Lantern liveview implementation

// TODO: ptp_liveview_jpeg will return pointer to jpeg and length

#include <stdlib.h>
#include <stdio.h>
#include <libpict.h>
#include <ptp.h>

int ptp_liveview_type(struct PtpRuntime *r) {
	int type = ptp_device_type(r);
	if (type == PTP_DEV_CANON || type == PTP_DEV_EOS) {
		// TODO: allow changing of EOS/ML liveview priority
		if (ptp_check_opcode(r, PTP_OC_ML_Live360x240)) {
			return PTP_LV_ML;
		}

		if (ptp_check_opcode(r, PTP_OC_ML_LiveBmpRam)) {
			return PTP_LV_EOS_ML_BMP;
		}

		if (ptp_check_opcode(r, PTP_OC_EOS_GetViewFinderData)) {
			return PTP_LV_EOS;
		}

		if (ptp_check_opcode(r, PTP_OC_CANON_GetViewFinderImage)) {
			return PTP_LV_CANON;
		}
	}

	return PTP_LV_NONE;
}

int ptp_liveview_params(struct PtpRuntime *r, struct PtpLiveviewParams *params) {
	switch (ptp_liveview_type(r)) {
	case PTP_LV_EOS:
		params->payload_offset_to_data = 8;
		params->format = PTP_LV_JPEG;
		break;
	}

	return 0;
}

int ptp_liveview_size(struct PtpRuntime *r) {
	ptp_panic("deprecated");
}

int ptp_liveview_init(struct PtpRuntime *r) {
	int x;
	switch (ptp_liveview_type(r)) {
	case PTP_LV_ML:
		return 0;
	case PTP_LV_EOS_ML_BMP:
		x = ptp_ml_init_bmp_lv(r);
		if (x) return x;
	case PTP_LV_EOS:
		x = ptp_eos_set_prop_value(r, PTP_DPC_EOS_VF_Output, 3);
		if (x) return x;
		x = ptp_eos_set_prop_value(r, PTP_DPC_EOS_CaptureDestination, 4);
		if (x) return x;
		//if (ptp_eos_set_prop_value(r, PTP_DPC_EOS_EVFMode, 1)) return PTP_CAM_ERR;
		return 0;
	}

	return PTP_RUNTIME_ERR;
}

int ptp_liveview_deinit(struct PtpRuntime *r) {
	int x;
	switch (ptp_liveview_type(r)) {
	case PTP_LV_ML:
	case PTP_LV_EOS_ML_BMP:
		return 0;
	case PTP_LV_EOS:
		// x = ptp_eos_set_prop_value(r, PTP_DPC_EOS_VF_Output, 0);
		// if (x) return x;
		x = ptp_eos_set_prop_value(r, PTP_DPC_EOS_CaptureDestination, 2);
		if (x) return x;
		x = ptp_eos_hdd_capacity_push(r);
		if (x) return x;
		return 0;
	}

	return PTP_RUNTIME_ERR;
}

int ptp_liveview_frame(struct PtpRuntime *r, void *buffer) {
	ptp_panic("deprecated");
}
