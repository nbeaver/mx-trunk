/*
 * Name:     n_tcpip.h
 *
 * Purpose:  Header file for client interface to an MX TCP/IP server.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2007, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __N_TCPIP_H__
#define __N_TCPIP_H__

#include "mx_socket.h"

/* ===== MX TCP/IP server record data structures ===== */

typedef struct {
	char hostname[ MXU_HOSTNAME_LENGTH + 1 ];
	long port;

	MX_SOCKET *socket;
	mx_bool_type first_attempt;

	long socket_fd;    /* Copied from MX_SOCKET above when requested. */
} MX_TCPIP_SERVER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_tcpip_server_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_tcpip_server_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxn_tcpip_server_delete_record( MX_RECORD *record );
MX_API mx_status_type mxn_tcpip_server_open( MX_RECORD *record );
MX_API mx_status_type mxn_tcpip_server_close( MX_RECORD *record );
MX_API mx_status_type mxn_tcpip_server_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxn_tcpip_server_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxn_tcpip_server_receive_message(
				MX_NETWORK_SERVER *network_server,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer );

MX_API mx_status_type mxn_tcpip_server_send_message(
				MX_NETWORK_SERVER *network_server,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer );

MX_API mx_status_type mxn_tcpip_server_connection_is_up(
					MX_NETWORK_SERVER *network_server,
					mx_bool_type *connection_is_up );

MX_API mx_status_type mxn_tcpip_server_reconnect_if_down(
					MX_NETWORK_SERVER *network_server );

MX_API mx_status_type mxn_tcpip_server_message_is_available(
					MX_NETWORK_SERVER *network_server,
					mx_bool_type *message_is_available );

extern MX_RECORD_FUNCTION_LIST mxn_tcpip_server_record_function_list;
extern MX_NETWORK_SERVER_FUNCTION_LIST
			mxn_tcpip_server_network_server_function_list;

extern long mxn_tcpip_server_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxn_tcpip_server_rfield_def_ptr;

#define MXLV_TCPIP_SERVER_SOCKET_FD		83801

#define MXN_TCPIP_SERVER_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, { MXU_HOSTNAME_LENGTH }, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TCPIP_SERVER, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TCPIP_SERVER, port), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_TCPIP_SERVER_SOCKET_FD, -1, "socket_fd", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TCPIP_SERVER, socket_fd), \
	{0}, NULL, (MXFF_IN_SUMMARY | MXFF_READ_ONLY) }


#endif /* __N_TCPIP_H__ */

