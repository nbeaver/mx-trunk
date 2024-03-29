/*
 * Name:     mx_net.h
 *
 * Purpose:  Header file for the MX network protocol.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2003-2015, 2017, 2021-2023
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_NET_H__
#define __MX_NET_H__

#include "mx_stdint.h"
#include "mx_namefix.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_handle.h"
#include "mx_list.h"

/* Used by mx_parse_network_field_id() and mx_find_mx_server_record() below. */

#define MXU_SERVER_ARGUMENTS_LENGTH	80

/* Current size of an MX network header. */

#define MXU_NETWORK_NUM_HEADER_VALUES	7

#define MXU_NETWORK_HEADER_LENGTH \
	( MXU_NETWORK_NUM_HEADER_VALUES * sizeof(uint32_t) )

/* Initial size of an MX network message buffer. */

#define MXU_NETWORK_INITIAL_MESSAGE_BUFFER_LENGTH	1000

/* Minimum allowed size of an MX network message_buffer. */

#define MXU_NETWORK_MINIMUM_MESSAGE_BUFFER_LENGTH \
	(MXU_NETWORK_HEADER_LENGTH + 1)

/* Maximum length of a remote MX version string. */

#define MXU_NETWORK_REMOTE_MX_VERSION_NAME_LENGTH	80

/* Bitmasks used with network message ids.  Message ids are always 32-bits. */

#define MX_NETWORK_MESSAGE_ID_MASK	0x7fffffff

#define MX_NETWORK_MESSAGE_IS_CALLBACK	0x80000000

/* Flag bits used by the 'connection_status' member of MX_NETWORK_SERVER. */

#define MXCS_CONNECTED		0x1
#define MXCS_CONNECTION_LOST	0x2
#define MXCS_RECONNECTED	0x4

/*
 * Define the data type that contains MX network messages.
 *
 * The union 'u' in the following data structure is defined so that the
 * address of the first byte will be correctly aligned on a 32-bit word
 * boundary for * those computer architectures that require such things.
 */

/*
 * The union 's' contains an MX_NETWORK_SERVER pointer when used in an
 * MX client role or an MX_SOCKET_HANDLER when using in an MX server role.
 *
 * Please note that an MX server can be a client of another MX server,
 * so you need to keep track of whether a given socket is acting as 
 * a server or as a client.
 */

struct mx_socket_handler_type;    /* Complete definition in mx_process.h */

typedef struct mx_network_message_buffer {
	union {
		uint32_t *uint32_buffer;
		char     *char_buffer;
	} u;
	size_t buffer_length;
	unsigned long data_format;

	mx_bool_type used_by_socket_handler;

	union {
		struct mx_network_server_type *network_server;
		struct mx_socket_handler_type *socket_handler;
	} s;

} MX_NETWORK_MESSAGE_BUFFER;

/* nf_flags bit definitions */

#define MXF_NF_USE_OLD_ARRAY_COPY	0x100000

/**** Structure used by MX_NETWORK_FIELD typedef further down. ****/

struct mx_network_field_type {
	char nfname[ MXU_RECORD_FIELD_NAME_LENGTH + 1 ];
	MX_RECORD *server_record;
	long record_handle;
	long field_handle;

	long remote_datatype;
	long network_datatype;

	long num_dimensions;
	long *dimension;

	unsigned long num_elements;

	size_t network_element_size;
	size_t network_max_bytes;

	unsigned long nf_flags;

	MX_RECORD_FIELD *local_field;

	mx_bool_type must_free_local_field_on_delete;
	mx_bool_type do_not_copy_buffer_on_callback;

	void *application_ptr;
};

/*---*/

/* We initially set the value of the remote MX server version to 
 * MXT_REMOTE_MX_VERSION_UNKNOWN until we have determined the
 * actual remote version number.
 */

#define MXT_REMOTE_MX_VERSION_UNKNOWN	ULONG_MAX

/*---*/

typedef struct mx_network_server_type {
	MX_RECORD *record;

	unsigned long server_flags;
	double timeout;			/* in seconds */

	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	unsigned long remote_mx_version;
	char          remote_mx_version_name
				[MXU_NETWORK_REMOTE_MX_VERSION_NAME_LENGTH+1];
	uint64_t      remote_mx_version_time;
	unsigned long remote_native_data_format;
	unsigned long remote_wordsize;
	unsigned long data_format;

	mx_bool_type server_supports_network_handles;
	mx_bool_type network_handles_are_valid;
	mx_bool_type use_64bit_network_longs;

	unsigned long connection_status;

	/* Note that currently_running_callback is actually a pointer to
	 * an MX_CALLBACK structure.  But we do not prototype it as such
	 * here to prevent MX clients from having to include "mx_callback.h"
	 * if they would otherwise not have to do this.
	 */

	mx_bool_type callback_in_progress;
	void *currently_running_callback;

	unsigned long num_callbacks_received;
	unsigned long num_callbacks_invoked;

	unsigned long last_rpc_message_id;

	unsigned long remote_header_length;
	unsigned long last_data_type;

	unsigned long network_field_array_block_size;

	unsigned long num_network_fields;
	struct mx_network_field_type **network_field_array;

	MX_LIST *callback_list;
} MX_NETWORK_SERVER;

typedef struct mx_network_field_type MX_NETWORK_FIELD;

typedef struct {
	mx_status_type ( *receive_message ) ( MX_NETWORK_SERVER *server,
					MX_NETWORK_MESSAGE_BUFFER *buffer );

	mx_status_type ( *send_message ) ( MX_NETWORK_SERVER *server,
					MX_NETWORK_MESSAGE_BUFFER *buffer );

	mx_status_type ( *connection_is_up ) ( MX_NETWORK_SERVER *server,
					mx_bool_type *connection_is_up );

	mx_status_type ( *reconnect_if_down ) ( MX_NETWORK_SERVER *server );

	mx_status_type ( *message_is_available ) ( MX_NETWORK_SERVER *server,
					mx_bool_type *message_is_available );
} MX_NETWORK_SERVER_FUNCTION_LIST;

#define MX_NETWORK_SERVER_STANDARD_FIELDS \
  {-1, -1, "server_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_NETWORK_SERVER, server_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, offsetof(MX_NETWORK_SERVER, timeout), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "remote_mx_version", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, offsetof(MX_NETWORK_SERVER, remote_mx_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "remote_mx_version_name", MXFT_STRING, NULL, 1, \
	  		{ MXU_NETWORK_REMOTE_MX_VERSION_NAME_LENGTH }, \
        MXF_REC_CLASS_STRUCT, \
	  		offsetof(MX_NETWORK_SERVER, remote_mx_version_name), \
	{sizeof(char)}, NULL, ( MXFF_READ_ONLY | MXFF_IN_SUMMARY ) }, \
  \
  {-1, -1, "remote_mx_version_time", MXFT_UINT64, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, remote_mx_version_time), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "remote_native_data_format", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, \
	  	offsetof(MX_NETWORK_SERVER, remote_native_data_format), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "remote_wordsize", MXFT_ULONG, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, offsetof(MX_NETWORK_SERVER, remote_wordsize), \
	{0}, NULL, (MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "data_format", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, offsetof(MX_NETWORK_SERVER, data_format), \
	{0}, NULL, ( MXFF_READ_ONLY | MXFF_IN_SUMMARY ) }, \
  \
  {-1, -1, "server_supports_network_handles", MXFT_BOOL, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, server_supports_network_handles), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "network_handles_are_valid", MXFT_BOOL, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, network_handles_are_valid), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "use_64bit_network_longs", MXFT_BOOL, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, use_64bit_network_longs), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "connection_status", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_CLASS_STRUCT, offsetof(MX_NETWORK_SERVER, connection_status), \
	{0}, NULL, ( MXFF_READ_ONLY | MXFF_IN_SUMMARY ) }, \
  \
  {-1, -1, "callback_in_progress", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, callback_in_progress),\
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "num_callbacks_received", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, num_callbacks_received), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "num_callbacks_invoked", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, num_callbacks_invoked), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "last_rpc_message_id", MXFT_HEX, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, last_rpc_message_id), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "remote_header_length", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, remote_header_length), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "last_data_type", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_CLASS_STRUCT, \
		offsetof(MX_NETWORK_SERVER, last_data_type), \
	{0}, NULL, MXFF_READ_ONLY }

/* Values for the server_flags field. */

#define MXF_NETWORK_SERVER_NO_AUTO_RECONNECT	0x1
#define MXF_NETWORK_SERVER_DISABLED		0x2

#define MXF_NETWORK_SERVER_USE_ASCII_FORMAT	0x100
#define MXF_NETWORK_SERVER_USE_RAW_FORMAT	0x200
#define MXF_NETWORK_SERVER_USE_XDR_FORMAT	0x400
#define MXF_NETWORK_SERVER_USE_BYTESWAP_FORMAT	0x800

#define MXF_NETWORK_SERVER_USE_64BIT_LONGS	0x10000

#define MXF_NETWORK_SERVER_USE_OLD_ARRAY_COPY	0x100000

#define MXF_NETWORK_SERVER_DEBUG_MESSAGE_IDS	0x8000000

#define MXF_NETWORK_SERVER_QUIET_RECONNECTION	0x10000000
#define MXF_NETWORK_SERVER_BLOCKING_IO		0x20000000
#define MXF_NETWORK_SERVER_DEBUG_VERBOSE	0x40000000
#define MXF_NETWORK_SERVER_DEBUG_SUMMARY	0x80000000

#define MXF_NETWORK_SERVER_DEBUG_ANY   (MXF_NETWORK_SERVER_DEBUG_SUMMARY \
    | MXF_NETWORK_SERVER_DEBUG_MESSAGE_IDS | MXF_NETWORK_SERVER_DEBUG_VERBOSE)

/* list_head->network_debug_flags bit definitions. */

#define MXF_NETDBG_SUMMARY	0x1
#define MXF_NETDBG_MSG_IDS	0x2
#define MXF_NETDBG_VERBOSE	0x4
#define MXF_NETDBG_DUMP		0x8

/* Definition of network data formats. */

#define MX_NETWORK_DATAFMT_ASCII	1
#define MX_NETWORK_DATAFMT_RAW		2
#define MX_NETWORK_DATAFMT_XDR		3
#define MX_NETWORK_DATAFMT_BYTESWAP	4

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
#define MX_NETWORK_DATA_TYPE		5
#define MX_NETWORK_MESSAGE_ID		6

/* Definition of network message type flags. */

#define MX_NETMSG_ERROR_FLAG		0x8000000
#define MX_NETMSG_SERVER_RESPONSE_FLAG	0x4000000

#define mx_server_response(x)	((x) | MX_NETMSG_SERVER_RESPONSE_FLAG)

#define mx_server_supports_message_ids(s) \
((s)->remote_header_length >= ((MX_NETWORK_MESSAGE_ID+1) * sizeof(uint32_t)))

#define mx_client_supports_message_ids(h) \
((h)->remote_header_length >= ((MX_NETWORK_MESSAGE_ID+1) * sizeof(uint32_t)))

#define mx_remote_header_length(r) \
	( (r)->remote_header_length ? \
		(r)->remote_header_length : MXU_NETWORK_HEADER_LENGTH )

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

#define MX_NETMSG_GET_ATTRIBUTE		0x2101
#define MX_NETMSG_SET_ATTRIBUTE		0x2102

#define MX_NETMSG_SET_CLIENT_INFO	0x3001
#define MX_NETMSG_GET_OPTION		0x3002
#define MX_NETMSG_SET_OPTION		0x3003

#define MX_NETMSG_ADD_CALLBACK		0x4001
#define MX_NETMSG_DELETE_CALLBACK	0x4002

#define MX_NETMSG_CALLBACK		0x9001

/* The MX_NETWORK_MAGIC header field should always contain the
 * MX_NETWORK_MAGIC_VALUE.  If it does not, there has been
 * a protocol error somewhere.
 */

#define MX_NETWORK_MAGIC_VALUE		0xe7e9fcfe


/* Option values for MX_NETMSG_GET_OPTION and MX_NETMSG_SET_OPTION. */

	/* MX_NETWORK_OPTION_DATAFMT and MX_NETWORK_OPTION_NATIVE_DATAFMT
	 * currently can have the values MX_NETWORK_DATAFMT_ASCII,
	 * MX_NETWORK_DATAFMT_RAW, MX_NETWORK_DATAFMT_XDR, and
	 * MX_NETWORK_DATAFMT_BYTESWAP.
	 */

#define MX_NETWORK_OPTION_DATAFMT		1
#define MX_NETWORK_OPTION_NATIVE_DATAFMT	2
#define MX_NETWORK_OPTION_64BIT_LONG		3
#define MX_NETWORK_OPTION_WORDSIZE		4
#define MX_NETWORK_OPTION_CLIENT_VERSION	5
#define MX_NETWORK_OPTION_CLIENT_VERSION_TIME	6

/*---*/

/* Attribute ids for MX network field. */

#define MXNA_VALUE_CHANGE_THRESHOLD		1
#define MXNA_POLL				2
#define MXNA_READ_ONLY				3
#define MXNA_NO_ACCESS				4

/*---*/

/* If all you plan to do is make raw lowlevel mx_gets() and mx_puts() to
 * a single MX server, then mx_connect_to_mx_server() is all you need.
 * However, for most situations mx_setup_database() is a better choice.
 */

MX_API mx_status_type mx_connect_to_mx_server( MX_RECORD **server_record,
						char *server_name,
						int server_port,
						double timeout_in_seconds,
						unsigned long server_flags );

MX_API mx_status_type mx_create_network_field( MX_NETWORK_FIELD **nf,
						MX_RECORD *server_record,
						char *record_name,
						char *field_name,
						MX_RECORD_FIELD *local_field );

/* --- */

MX_API mx_status_type mx_allocate_network_buffer(
				MX_NETWORK_MESSAGE_BUFFER **message_buffer,
				struct mx_network_server_type *network_server,
				struct mx_socket_handler_type *socket_handler,
				size_t initial_length );

MX_API mx_status_type mx_reallocate_network_buffer(
				MX_NETWORK_MESSAGE_BUFFER *message_buffer,
				size_t new_length );

MX_API void mx_free_network_buffer( MX_NETWORK_MESSAGE_BUFFER *message_buffer );

MX_API mx_status_type mx_network_receive_message( MX_RECORD *server_record,
					MX_NETWORK_MESSAGE_BUFFER *buffer );

MX_API mx_status_type mx_network_send_message( MX_RECORD *server_record,
					MX_NETWORK_MESSAGE_BUFFER *buffer );

MX_API mx_status_type mx_network_server_discover_header_length(
					MX_RECORD *server_record );

MX_API void mx_network_update_message_id( unsigned long *message_id );

MX_API mx_status_type mx_network_wait_for_message_id( MX_RECORD *server_record,
					MX_NETWORK_MESSAGE_BUFFER *buffer,
					uint32_t message_id,
					double timeout_in_seconds );

MX_API mx_status_type mx_network_wait_for_messages_from_server(
					MX_RECORD *server_record,
					double timeout_in_seconds );

MX_API mx_status_type mx_network_wait_for_messages( MX_RECORD *record_list,
						double timeout_in_seconds );

MX_API mx_status_type mx_network_connection_is_up( MX_RECORD *server_record,
					mx_bool_type *connection_is_up );

MX_API mx_status_type mx_network_reconnect_if_down( MX_RECORD *server_record );

MX_API mx_status_type mx_network_message_is_available( MX_RECORD *server_record,
					mx_bool_type *message_is_available );

MX_API mx_status_type mx_network_mark_handles_as_invalid(
						MX_RECORD *server_record );

MX_API mx_status_type mx_network_restore_callbacks( MX_RECORD *server_record );

/* mx_network_dump_message() displays the raw bytes and addresses in the
 * specified network message.
 */

MX_API void mx_network_dump_message( MX_NETWORK_MESSAGE_BUFFER *message_buffer,
					unsigned long network_data_format,
					long max_network_dump_bytes );

MX_API void mx_network_dump_value( uint32_t *value_buffer,
					unsigned long network_data_format,
					long value_datatype,
					long num_bytes_in_value );

/* mx_network_display_message() is used to display the contents of
 * a network message.
 */

MX_API void mx_network_display_message( MX_NETWORK_MESSAGE_BUFFER *buffer,
					MX_RECORD_FIELD *record_field,
					mx_bool_type use_64bit_network_longs );

/* mx_network_display_summary() gives a terser description of the message. */

MX_API void mx_network_display_summary( MX_NETWORK_MESSAGE_BUFFER *buffer,
					MX_NETWORK_FIELD *network_field,
					MX_RECORD *server_record,
					char *remote_record_field_name,
					MX_RECORD_FIELD *record_field,
					mx_bool_type use_64bit_network_longs );

/* mx_network_buffer_show_value() is a lower level function that only
 * shows the contents of the value array at the end of a network message.
 */

MX_API void mx_network_buffer_show_value( void *value_buffer,
					unsigned long data_format,
					uint32_t datatype,
					uint32_t message_type,
					uint32_t message_length,
					mx_bool_type use_64bit_network_longs );

/* mx_network_display_message_binary() is a function that understands the
 * structure of MX network headers, but otherwise displays the contents of
 * the message as binary bytes in hexadecimal.
 */

MX_API void mx_network_display_message_binary(
		MX_NETWORK_MESSAGE_BUFFER *buffer );

/*---*/

#define mx_make_network_handle_invalid( nf ) \
	do { \
		(nf)->record_handle = MX_ILLEGAL_HANDLE; \
		(nf)->field_handle = MX_ILLEGAL_HANDLE; \
	} while (0)

MX_API mx_status_type mx_network_field_is_connected( MX_NETWORK_FIELD *nf,
					mx_bool_type *connected );

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

/* Flag bits for mx_get_field_type(). 
 *
 * MXF_GFT_OVERDIM_QUIET means suppress the display of the error message
 *                       that would otherwise occur if max_dimensions
 *                       turned out to be smaller than the actual dimensions
 *                       so that the caller can transparently dynamically
 *                       resize the array 'dimension_array'.
 *
 * MXFT_GFT_SHOW_STACK_TRACEBACK displays the stack traceback if the above
 *                       error occurs.
 */

#define MXF_GFT_OVERDIM_QUIET		0x1
#define MXF_GFT_SHOW_STACK_TRACEBACK	0x2

MX_API mx_status_type mx_get_field_type( MX_RECORD *server_record,
			char *remote_record_field_name,
			long max_dimensions,
			long *datatype,
			long *num_dimensions,
			long *dimension_array,
	       		unsigned long debug_flags );

MX_API mx_status_type mx_set_client_info( MX_RECORD *server_record,
			const char *username,
			const char *program_name );

MX_API mx_status_type mx_network_get_option( MX_RECORD *server_record,
			unsigned long option_number,
			unsigned long *option_value );

MX_API mx_status_type mx_network_set_option( MX_RECORD *server_record,
			unsigned long option_number,
			unsigned long option_value );

MX_API mx_status_type mx_network_field_get_attribute( MX_NETWORK_FIELD *nf,
						unsigned long attribute_number,
						double *attribute_value );

MX_API mx_status_type mx_network_field_set_attribute( MX_NETWORK_FIELD *nf,
						unsigned long attribute_number,
						double attribute_value );

MX_API mx_status_type mx_network_field_get_attribute_by_name(
						MX_NETWORK_FIELD *nf,
						char *attribute_name,
						double *attribute_value );

MX_API mx_status_type mx_network_field_set_attribute_by_name(
						MX_NETWORK_FIELD *nf,
						char *attribute_name,
						double attribute_value );

MX_API mx_status_type mx_network_field_get_attribute_number(
						MX_NETWORK_FIELD *nf,
						char *attribute_name,
					    unsigned long *attribute_number );

MX_API mx_status_type mx_network_request_data_format(
				MX_RECORD *server_record,
				unsigned long requested_format );

MX_API mx_status_type mx_network_request_64bit_longs(
				MX_RECORD *server_record,
				mx_bool_type use_64bit_network_longs );

MX_API mx_status_type mx_network_send_client_version(
				MX_RECORD *server_record );

MX_API mx_status_type mx_network_send_client_version_time(
				MX_RECORD *server_record );

MX_API char *mx_network_get_nf_label( MX_RECORD *server_record,
				char *remote_record_field_name,
				char *label, size_t max_label_length );

MX_API char *mx_network_get_remote_field_label(
				MX_NETWORK_FIELD *nf,
				MX_RECORD *server_record,
				char *remote_record_field_name,
				MX_RECORD_FIELD *record_field,
				char *label, size_t max_label_length );

MX_API mx_status_type mx_parse_network_field_id( char *network_field_id,
		char *server_name, size_t max_server_name_length,
		char *server_arguments, size_t max_server_arguments_length,
		char *record_name, size_t max_record_name_length,
		char *field_name, size_t max_field_name_length );

MX_API mx_status_type mx_get_mx_server_record( MX_RECORD *record_list,
			char *server_name, char *server_arguments,
			MX_RECORD **server_record,
			double timeout );

MX_API mx_status_type mx_network_copy_message_to_field(
				MX_RECORD *source_server_record,
				MX_RECORD_FIELD *destination_field );

#ifdef __cplusplus
}
#endif

#endif /* __MX_NET_H__ */
