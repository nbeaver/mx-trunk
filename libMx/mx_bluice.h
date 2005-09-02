/*
 * Name:    mx_bluice.h
 *
 * Purpose: Header file for communication with Blu-Ice DCSS and DHS
 *          servers by MX.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_BLUICE_H__
#define __MX_BLUICE_H__

#define MX_BLUICE_MSGHDR_TEXT_LENGTH	12
#define MX_BLUICE_MSGHDR_BINARY_LENGTH	13

#define MX_BLUICE_MSGHDR_LENGTH		( MX_BLUICE_MSGHDR_TEXT_LENGTH \
					  + MX_BLUICE_MSGHDR_BINARY_LENGTH + 1 )

#define MX_BLUICE_MSGHDR_TEXT		0
#define MX_BLUICE_MSGHDR_BINARY		MX_BLUICE_MSGHDR_TEXT_LENGTH
#define MX_BLUICE_MSGHDR_NULL		( MX_BLUICE_MSGHDR_LENGTH - 1 )

/* ----- */

#define MX_BLUICE_INITIAL_RECEIVE_BUFFER_LENGTH	1000

typedef struct {
	MX_RECORD *record;

	MX_SOCKET *socket;

	char *receive_buffer;
	size_t receive_buffer_length;
	size_t num_received_bytes;
} MX_BLUICE_SERVER;

/* ----- */

MX_API mx_status_type
mx_bluice_send_message( MX_RECORD *bluice_server_record,
			char *text_data,
			char *binary_data,
			size_t binary_data_length );

MX_API mx_status_type
mx_bluice_receive_message( MX_RECORD *bluice_server_record,
				char *data_buffer,
				size_t data_buffer_size,
				size_t *actual_data_length );

/* ----- */

MX_API mx_status_type
mx_bluice_num_unread_bytes_available( MX_RECORD *bluice_server_record,
					int *num_bytes_available );

MX_API mx_status_type
mx_bluice_discard_unread_bytes( MX_RECORD *bluice_server_record );

#endif /* __MX_BLUICE_H__ */

