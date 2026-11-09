/** \file */
#ifndef CL_STUFF_H
#define CL_STUFF_H

/// @brief Prints out USB PTP devices currently connected with IDs in each entry
int ptp_list_devices(void);

/// @brief Connects and inits a session from an ID
struct PtpRuntime *ptp_connect_from_id(int id);

/// @brief Dump common PTP info on a device from ID
int ptp_dump_device(int dev_id);

#endif
