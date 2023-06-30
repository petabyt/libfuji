// Test basic opcode, get device properties
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <camlib/camlib.h>

int ptp_set_prop_value_16(struct PtpRuntime *r, int code, int value) {
	struct PtpCommand cmd;
	cmd.code = PTP_OC_SetDevicePropValue;
	cmd.param_length = 1;
	cmd.params[0] = code;

	uint16_t dat[] = {(uint16_t)value};

	return ptp_generic_send_data(r, &cmd, dat, sizeof(dat));
}

int main() {
	struct PtpRuntime r;
	ptp_generic_init(&r);
	r.connection_type = PTP_IP;
	if (ptpip_connect(&r, "192.168.0.1", FUJI_CMD_IP_PORT)) {
		puts("Device connection error");
		return 0;
	}

	if (ptpip_fuji_init(&r, "camlib")) {
		puts("Error on initialize");
	}

	struct PtpFujiInitResp resp;
	ptp_fuji_get_init_info(&r, &resp);

	printf("Connecting to %s\n", resp.cam_name);

	ptp_open_session(&r);

	ptpip_fuji_wait_unlocked(&r);

	ptp_set_prop_value(&r, PTP_PC_FUJI_Mode, 2); // set 16 bit
	ptp_set_prop_value(&r, PTP_PC_FUJI_FunctionVersion, 2); // set 32 bit

	ptpip_close(&r);

	free(r.data);
	return 0;
}

