// Generic device-independent interface for cameras
#include <stdlib.h>
#include <string.h>
#include <libpict.h>

int ptp_validate_property_value(struct PtpRuntime *r, int prop_code, uint32_t value) {
	struct PtpPropAvail *n;
	for (n = r->avail; n != NULL; n = n->prev) {
		if (n->code == prop_code) break;
	}

	if (n == NULL) return 1;

	for (int i = 0; i < n->memb_cnt; i++) {
		uint32_t cur_val;
		switch (n->memb_size) {
		case 4:
			cur_val = ((uint32_t *)n->data)[i];
			break;
		case 2:
			cur_val = ((uint16_t *)n->data)[i];
			break;
		case 1:
			cur_val = ((uint8_t *)n->data)[i];
			break;
		default:
			ptp_panic("Unsupported PTP prop length %X\n", prop_code);
			return 0;
		}

		if (value == cur_val) {
			ptp_verbose_log(r, "Found valid prop value %X for 0x%X\n", value, prop_code);
			return 0;
		}
	}

	return 2;
}

static int ptp_eos_set_validate_prop(struct PtpRuntime *r, int prop_code, uint32_t value) {
	int rc = ptp_validate_property_value(r, prop_code, value);

	// If invalid value, don't set. Else... we don't have any info, and we assume it's valid
	if (rc == 2) {
		return rc;
	}

	return ptp_eos_set_prop_value(r, prop_code, value);
}

int ptp_set_generic_property(struct PtpRuntime *r, const char *name, int value) {
	int dev = ptp_device_type(r);
	int rc = 0;
	if (!strcmp(name, "aperture")) {
		if (dev == PTP_DEV_EOS) {
			if (ptp_eos_get_aperture(value, &value, 1)) return PTP_RUNTIME_ERR;
			rc = ptp_eos_set_validate_prop(r, PTP_DPC_EOS_Aperture, value);
		}
	} else if (!strcmp(name, "iso")) {
		if (dev == PTP_DEV_EOS) {
			if (ptp_eos_get_iso(value, &value, 1)) return PTP_RUNTIME_ERR;
			rc = ptp_eos_set_validate_prop(r, PTP_DPC_EOS_ISOSpeed, value);
		}
	} else if (!strcmp(name, "shutter speed")) {
		if (dev == PTP_DEV_EOS) {
			if (ptp_eos_get_shutter(value, &value, 1)) return PTP_RUNTIME_ERR;
			rc = ptp_eos_set_validate_prop(r, PTP_DPC_EOS_ShutterSpeed, value);
		}
	} else if (!strcmp(name, "white balance")) {
		if (dev == PTP_DEV_EOS) {
			if (ptp_eos_get_white_balance(value, &value, 1)) return PTP_RUNTIME_ERR;
			rc = ptp_eos_set_prop_value(r, PTP_DPC_EOS_WhiteBalance, value);
			rc = ptp_eos_set_prop_value(r, PTP_DPC_EOS_EVFWBMode, value);
		}
	} else if (!strcmp(name, "destination")) {
		return PTP_UNSUPPORTED;
	} else {
		return PTP_UNSUPPORTED;
	}

	return rc;
}

// Required for ptp_take_picture
int ptp_pre_take_picture(struct PtpRuntime *r) {
	if (ptp_device_type(r) == PTP_DEV_EOS) {
		// Shutter half down, wait up to 10s for focus 
		r->wait_for_response = 10;
		int rc = ptp_eos_remote_release_on(r, 1);
		if (rc) return rc;
	}

	return 0;	
}

int ptp_take_picture(struct PtpRuntime *r) {
	if (ptp_device_type(r) == PTP_DEV_EOS) {
		// Shutter fully down, wait up to 3s for flash to pop up
		r->wait_for_response = 3;
		int rc = ptp_eos_remote_release_on(r, 2);
		if (rc) return rc;

		rc = ptp_eos_remote_release_off(r, 2);
		if (rc) return rc;

		rc = ptp_eos_remote_release_off(r, 1);
		if (rc) return rc;

		return 0;
	}

	return PTP_UNSUPPORTED;
}

int ptp_events_json(struct PtpRuntime *r, char *buffer, int max) {
	return 0;
}

int ptp_get_all_known(struct PtpRuntime *r, struct PtpGenericEvent **s, unsigned int *length) {
	uint16_t *props = r->di->props_supported;
	int plength = r->di->props_supported_length;
	(*length) = plength;

	(*s) = malloc(sizeof(struct PtpGenericEvent) * plength);

	for (int i = 0; i < plength; i++) {
		struct PtpGenericEvent *cur = &((*s)[i]);
		memset(cur, 0, sizeof(struct PtpGenericEvent));

		cur->code = props[i];

		int rc = ptp_get_prop_value(r, props[i]);
		if (rc) return rc;

		int v = ptp_parse_prop_value(r);
		cur->value = v;
		if (v == -1) {
			continue;
		}

		// TODO: Get more props
		switch (props[i]) {
		case PTP_DPC_BatteryLevel:
			cur->name = "battery";
			cur->value = v;
			break;
		}
	}

	return 0;
}
