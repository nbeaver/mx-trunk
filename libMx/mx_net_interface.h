/*
 * Name:     mx_net_interface.h
 *
 * Purpose:  Header file for network interfaces such as Ethernet.
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

#ifndef __MX_NET_INTERFACE_H__
#define __MX_NET_INTERFACE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(OS_VMS)
#include "mx_namefix.h"
#endif

#define MXU_NETWORK_INTERFACE_NAME_LENGTH	80

typedef struct {
	char name[MXU_NETWORK_INTERFACE_NAME_LENGTH + 1];
	char raw_name[MXU_NETWORK_INTERFACE_NAME_LENGTH + 1];
	unsigned long ipv4_address;
	unsigned long ipv4_subnet_mask;
	unsigned long mtu;
	void *net_private;
} MX_NETWORK_INTERFACE;

MX_API mx_status_type mx_network_get_interface_from_host_address(
						MX_NETWORK_INTERFACE **ni,
						struct sockaddr *host_address );

MX_API mx_status_type mx_network_get_interface_from_hostname(
						MX_NETWORK_INTERFACE **ni,
						char *hostname );

#ifdef __cplusplus
}
#endif

#endif /* __MX_NET_INTERFACE_H__ */

