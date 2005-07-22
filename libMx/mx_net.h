/*
 * Name:     mx_net.h
 *
 * Purpose:  Header file for the MX network protocol.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2003-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_NET_H__
#define __MX_NET_H__

#include "mx_types.h"
#include "mx_handle.h"

/* Used by mx_parse_network_field_id() and mx_find_mx_server_record() below. */

#define MXU_SERVER_ARGUMENTS_LENGTH	80

/* Current size of an MX network header. */

#define MX_NETWORK_NUM_HEADER_VALUES	5

#define MX_NETWORK_HEADER_LENGTH_VALUE	\
	( MX_NETWORK_NUM_HEADER_VALUES * sizeof(mx_uint32_type) )

/*
 * Maximum size of an MX network message.  This limit needs to be eliminated,
 * but for now we are stuck with it.
 */

#define MX_NETWORK_MAXIMUM_MESSAGE_SIZE  163840L

/*
 * Define the data type that contains MX network messages.
 *
 * The following union is defined so that the address of the first byte
 * will be correctly aligned on a 32-bit word boundary for those computer 
 * architectures that require such things.
 */

typedef union {
	mx_uint32_type uint32_buffer[MX_NETWORK_HEADER_LENGTH_VALUE];
	char           char_buffer[MX_NETWORK_MAXIMUM_MESSAGE_SIZE];
} MX_NETWORK_MESSAGE_BUFFER;

/*
 * MX network server data structures.
 */

struct mx_network_field_type {
	char nfname[ MXU_RECORD_FIELD_NAME_LENGTH + 1 ];
	MX_RECORD *server_record;
	long record_handle;
	long field_handle;
};

typedef struct {
	MX_RECORD *record;

	unsigned long server_flags;

	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	unsigned long remote_mx_version;
	unsigned long data_format;
	int server_supports_network_handles;

	int network_handles_are_valid;
	unsigned long network_field_array_block_size;

	unsigned long num_network_fields;
	struct mx_network_field_type **network_field_array;
} MX_NETWORK_SERVER;

typedef struct mx_network_field_type MX_NETWORK_FIELD;

typedef struct {
	mx_status_type ( *receive_message ) ( MX_NETWORK_SERVER *server,
						unsigned long buffer_length,
						void *buffer );
	mx_status_type ( *send_message ) ( MX_NETWORK_SERVER *server,
						void *buffer );
	mx_status_type ( *connection_is_up ) ( MX_NETWORK_SERVER *server,
						int *connection_is_up );
	mx_status_type ( *reconnect_if_down ) ( MX_NETWORK_SERVER *server );
} MX_NETWORK_SERVER_FUNCTION_LIST;

#define MX_NETWORK_SERVER_STANDARD_FIELDS \
  {-1, -1, "server_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_NETWORK_SERVER, server_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

/* Values for the server_flags field. */

#define MXF_NETWORK_SERVER_NO_AUTO_RECONNECT	0x1
#define MXF_NETWORK_SERVER_DISABLED		0x2

#define MXF_NETWORK_SERVER_USE_ASCII_FORMAT	0x100
#define MXF_NETWORK_SERVER_USE_RAW_FORMAT	0x200
#define MXF_NETWORK_SERVER_USE_XDR_FORMAT	0x400

/* Definition of network data formats. */

#define MX_NETWORK_DATAFMT_ASCII	1
#define MX_NETWORK_DATAFMT_RAW		2
#define MX_NETWORK_DATAFMT_XDR		3

/* The following definition is used to request automatic network
 * format negotiation.
 */

#define MX_NETWORK_DATAFMT_NEGOTIATE	0xffffffff

/* Definition of the fields in a header. */

#define MX_NETWORK_MAGIC		0
#define MX_NETWORK_HEADER_LENGTH	1
#define MX_NETWORK_MESSAGE_LENGTH	2
#define MX_NETWORK_MESSAGE_TYPE		3
#define MX_NETWORK_STATUS_CODE		4

/* Definition of network message type flags. */

#define MX_NETMSG_ERROR_FLAG		0x8000000
#define MX_NETMSG_SERVER_RESPONSE_FLAG	0x4000000
#define MX_NETMSG_SERVER_CALLBACK_FLAG	0x2000000

#define mx_server_response(x)	((x) | MX_NETMSG_SERVER_RESPONSE_FLAG)
#define mx_server_callback(x)	((x) | MX_NETMSG_SERVER_CALLBACK_FLAG)

/* Definition of network message types. */

#define MX_NETMSG_UNEXPECTED_ERROR	0x1

/*
 * Messages from clients to servers.
 *
 * The response from the server will have the same value with the
 * MX_NETMSG_SERVER_RESPONSE_FLAG bit set.
 *
 */

#define MX_NETMSG_GET_ARRAY_BY_NAME	0x1001
#define MX_NETMSG_PUT_ARRAY_BY_NAME	0x1002

#define MX_NETMSG_GET_ARRAY_BY_HANDLE	0x1003
#define MX_NETMSG_PUT_ARRAY_BY_HANDLE	0x1004

#define MX_NETMSG_GET_NETWORK_HANDLE	0x2001
#define MX_NETMSG_GET_FIELD_TYPE	0x2005

#define MX_NETMSG_SET_CLIENT_INFO	0x3001
#define MX_NETMSG_GET_OPTION		0x3002
#define MX_NETMSG_SET_OPTION		0x3003

/* The MX_NETWORK_MAGIC header field should always contain the
 * MX_NETWORK_MAGIC_VALUE.  If it does not, there has been
 * a protocol error somewhere.
 */

#define MX_NETWORK_MAGIC_VALUE		0xe7e9fcfe


/* Option values for MX_NETMSG_GET_OPTION and MX_NETMSG_SET_OPTION. */

	/* MX_NETWORK_OPTION_DATAFMT and MX_NETWORK_OPTION_NATIVE_DATAFMT
	 * currently can have the values MX_NETWORK_DATAFMT_ASCII,
	 * MX_NETWORK_DATAFMT_RAW, and MX_NETWORK_DATAFMT_XDR.
	 */

#define MX_NETWORK_OPTION_DATAFMT		0x1
#define MX_NETWORK_OPTION_NATIVE_DATAFMT	0x2

MX_API mx_status_type mx_network_receive_message( MX_RECORD *server_record,
				unsigned long buffer_length, void *buffer );

MX_API mx_status_type mx_network_send_message( MX_RECORD *server_record,
					void *buffer );

MX_API mx_status_type mx_network_connection_is_up( MX_RECORD *server_record,
						int *connection_is_up );

MX_API mx_status_type mx_network_reconnect_if_down( MX_RECORD *server_record );

MX_API mx_status_type mx_network_mark_handles_as_invalid(
						MX_RECORD *server_record );

MX_API void mx_network_display_message_buffer( void *buffer );

/*---*/

#define mx_make_network_handle_invalid( nf ) \
	do { \
		(nf)->record_handle = MX_ILLEGAL_HANDLE; \
		(nf)->field_handle = MX_ILLEGAL_HANDLE; \
	} while (0)

MX_API mx_status_type mx_need_to_get_network_handle( MX_NETWORK_FIELD *nf,
						int *new_handle_needed );

MX_API mx_status_type mx_network_field_init( MX_NETWORK_FIELD *nf,
						MX_RECORD *server_record,
						char *name_format, ... );

MX_API mx_status_type mx_network_field_connect( MX_NETWORK_FIELD *nf );

/*---*/

#define mx_get( nf, t, v ) \
		mx_get_array( (nf), (t), 0, NULL, (v) )

#define mx_put( nf, t, v ) \
		mx_put_array( (nf), (t), 0, NULL, (v) )

MX_API mx_status_type mx_get_array( MX_NETWORK_FIELD *nf,
				long datatype,
				long num_dimensions,
				long *dimension,
				void *value );

MX_API mx_status_type mx_put_array( MX_NETWORK_FIELD *nf,
				long datatype,
				long num_dimensions,
				long *dimension,
				void *value );

/*---*/

#define mx_get_by_name( s, r, t, v ) \
		mx_internal_get_array( (s), (r), NULL, (t), 0, NULL, (v) )

#define mx_put_by_name( s, r, t, v ) \
		mx_internal_put_array( (s), (r), NULL, (t), 0, NULL, (v) )

#define mx_get_array_by_name( s, r, t, n, d, v ) \
		mx_internal_get_array( (s), (r), NULL, (t), (n), (d), (v) )

#define mx_put_array_by_name( s, r, t, n, d, v ) \
		mx_internal_put_array( (s), (r), NULL, (t), (n), (d), (v) )

/*---*/

MX_API mx_status_type mx_internal_get_array( MX_RECORD *server_record,
			char *remote_record_field_name,
			MX_NETWORK_FIELD *nf,
			long datatype, 
			long num_dimensions,
			long *dimension,
			void *value );

MX_API mx_status_type mx_internal_put_array( MX_RECORD *server_record,
			char *remote_record_field_name,
			MX_NETWORK_FIELD *nf,
			long datatype, 
			long num_dimensions,
			long *dimension,
			void *value );

MX_API mx_status_type mx_get_field_array( MX_RECORD *server_record,
			char *remote_record_field_name,
			MX_NETWORK_FIELD *nf,
			MX_RECORD_FIELD *local_field,
			void *value_ptr );

MX_API mx_status_type mx_put_field_array( MX_RECORD *server_record,
			char *remote_record_field_name,
			MX_NETWORK_FIELD *nf,
			MX_RECORD_FIELD *local_field,
			void *value_ptr );

MX_API mx_status_type mx_get_field_type( MX_RECORD *server_record,
			char *remote_record_field_name,
			long max_dimensions,
			long *datatype,
			long *num_dimensions,
			long *dimension_array );

MX_API mx_status_type mx_set_client_info( MX_RECORD *server_record,
			char *username,
			char *program_name );

MX_API mx_status_type mx_network_get_option( MX_RECORD *server_record,
			unsigned long option_number,
			unsigned long *option_value );

MX_API mx_status_type mx_network_set_option( MX_RECORD *server_record,
			unsigned long option_number,
			unsigned long option_value );

MX_API mx_status_type mx_network_request_data_format(
			MX_RECORD *server_record,
			unsigned long requested_format );

MX_API mx_status_type mx_parse_network_field_id( char *network_field_id,
		char *server_name, size_t max_server_name_length,
		char *server_arguments, size_t max_server_arguments_length,
		char *record_name, size_t max_record_name_length,
		char *field_name, size_t max_field_name_length );

MX_API mx_status_type mx_get_mx_server_record( MX_RECORD *record_list,
			char *server_name, char *server_arguments,
			MX_RECORD **server_record );

#endif /* __MX_NET_H__ */
