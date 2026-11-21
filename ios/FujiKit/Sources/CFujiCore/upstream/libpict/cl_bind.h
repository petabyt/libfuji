/** \file */ 
// Bindings for usage in IPC, etc. Niche and not very well supported or implemented.
#ifndef CL_BIND_H
#define CL_BIND_H

// Recommended buffer size for bind_run
#define PTP_BIND_DEFAULT_SIZE 5000000

#define BIND_MAX_NAME 64

struct BindReq {
	// @brief Argument passed to out or out_bytes
	void *arg;
	// @brief Output JSON data
	int (*out)(struct BindReq *bind, char *fmt, ...);
	/// @brief Output raw binary data
	int (*out_bytes)(struct BindReq *bind, void *bytes, size_t length);

	/// @brief Name of command to be run
	char name[BIND_MAX_NAME];

	/// @brief Parameters for command
	/// @note These are parsed by command handlers in bind.c, not always passed to the PTP operation
	int params[5];
	int params_length;
	/// @brief String argument, NULL for none
	char *string;
	/// @brief Data argument, NULL for none
	uint8_t *bytes;
	int bytes_length;
};

// @brief Run a binding directly from an instance of struct BindReq
int bind_run_req(struct PtpRuntime *r, struct BindReq *bind);

#endif
