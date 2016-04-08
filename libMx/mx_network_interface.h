/*
 * Name:     mx_network_interface.h
 *
 * Purpose:  Header file for network interfaces (typically Ethernet)
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_NETWORK_INTERFACE_H__
#define __MX_NETWORK_INTERFACE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_NETWORK_INTERFACE_NAME_LENGTH	40

typedef struct {
	char name[MXU_NETWORK_INTERFACE_NAME_LENGTH + 1];
	void *private;
} MX_NETWORK_INTERFACE;

MX_API mx_status_type mx_network_get_interface( MX_NETWORK_INTERFACE *ni,
						struct sockaddr *ip_address,
						struct_sockaddr *subnet_mask );
						

#ifdef __cplusplus
}
#endif

#endif /* __MX_NETWORK_INTERFACE_H__ */

