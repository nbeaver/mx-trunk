/*
 * Name:    ms_security.h
 *
 * Purpose: Access security functions for the MX server.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MS_SECURITY_H__
#define __MS_SECURITY_H__

typedef struct {
	long num_addresses;
	int *is_numerical_address;
	char **address_string_array;
} MXSRV_CONNECTION_ACL;

extern mx_status_type mxsrv_setup_connection_acl( MX_RECORD *record_list,
					char *connection_acl_filename );

extern mx_status_type mxsrv_check_socket_connection_acl_permissions(
					MX_RECORD *record_list,
					char *client_address_string,
					int *connection_allowed );

extern mx_status_type mxsrv_get_client_hostname( char *client_address_string,
					char *client_hostname );

#endif /* __MS_SECURITY_H__ */

