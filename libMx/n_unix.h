/*
 * Name:     n_unix.h
 *
 * Purpose:  Header file for client interface to an MX
 *           Unix domain socket server.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2007, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __N_UNIX_H__
#define __N_UNIX_H__

#include "mx_socket.h"

typedef struct {
	char pathname[ MXU_FILENAME_LENGTH + 1 ];

	MX_SOCKET *socket;
	mx_bool_type first_attempt;

	long socket_fd;    /* Copied from MX_SOCKET above when requested. */
} MX_UNIX_SERVER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_unix_server_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_unix_server_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxn_unix_server_delete_record( MX_RECORD *record );
MX_API mx_status_type mxn_unix_server_open( MX_RECORD *record );
MX_API mx_status_type mxn_unix_server_close( MX_RECORD *record );
MX_API mx_status_type mxn_unix_server_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxn_unix_server_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxn_unix_server_receive_message(
				MX_NETWORK_SERVER *network_server,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer );

MX_API mx_status_type mxn_unix_server_send_message(
				MX_NETWORK_SERVER *network_server,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer );

MX_API mx_status_type mxn_unix_server_connection_is_up(
					MX_NETWORK_SERVER *network_server,
					mx_bool_type *connection_is_up );

MX_API mx_status_type mxn_unix_server_reconnect_if_down(
					MX_NETWORK_SERVER *network_server );

MX_API mx_status_type mxn_unix_server_message_is_available(
					MX_NETWORK_SERVER *network_server,
					mx_bool_type *message_is_available );

extern MX_RECORD_FUNCTION_LIST mxn_unix_server_record_function_list;
extern MX_NETWORK_SERVER_FUNCTION_LIST
			mxn_unix_server_network_server_function_list;

extern long mxn_unix_server_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxn_unix_server_rfield_def_ptr;

#define MXLV_UNIX_SERVER_SOCKET_FD	83901

#define MXN_UNIX_SERVER_STANDARD_FIELDS \
  {-1, -1, "pathname", MXFT_STRING, NULL, 1, { MXU_FILENAME_LENGTH }, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UNIX_SERVER, pathname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_UNIX_SERVER_SOCKET_FD, -1, "socket_fd", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UNIX_SERVER, socket_fd), \
	{0}, NULL, MXFF_READ_ONLY }


#endif /* __N_UNIX_H__ */

