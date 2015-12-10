/*
 * Name:    ms_mxserver.h
 *
 * Purpose: Header file for server side of MX network protocol.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2004, 2006-2008 2012, 2014-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MS_MXSERVER_H__
#define __MS_MXSERVER_H__

typedef struct {
	long field_type;
	mx_status_type (*process_function) (void *, void *, int);
} MX_PROCESS_FUNCTION_LIST_ENTRY;

struct mxsrv_tcp_server_socket {
	int port;
};

#if HAVE_UNIX_DOMAIN_SOCKETS
struct mxsrv_unix_server_socket {
	char pathname[ MXU_FILENAME_LENGTH + 1 ];
};
#endif

struct mxsrv_ascii_server_socket {
	int port;
};

/*----*/

typedef struct {
	int type;
	MX_EVENT_HANDLER *client_event_handler;
	union {
		struct mxsrv_tcp_server_socket tcp;
#if HAVE_UNIX_DOMAIN_SOCKETS
		struct mxsrv_unix_server_socket unix_domain;
#endif
		struct mxsrv_ascii_server_socket ascii;
	} u;
} MXSRV_MX_SERVER_SOCKET;

extern MXSRV_MX_SERVER_SOCKET mxsrv_tcp_server_socket_struct;

#if HAVE_UNIX_DOMAIN_SOCKETS
extern MXSRV_MX_SERVER_SOCKET mxsrv_unix_server_socket_struct;
#endif

extern MXSRV_MX_SERVER_SOCKET mxsrv_ascii_server_socket_struct;

extern void mxsrv_print_timestamp( void );

extern int mxserver_main( int argc, char *argv[] );

extern uint32_t mxsrv_get_returning_message_type(
					uint32_t message_type );

extern mx_status_type mxsrv_initialize_record_processing(
			MX_RECORD *record_list );

extern mx_status_type mxsrv_free_client_socket_handler(
			MX_SOCKET_HANDLER *socket_handler,
			MX_SOCKET_HANDLER_LIST *socket_handler_list );

extern mx_status_type mxsrv_mx_server_socket_init(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER_LIST *socket_handler_list,
			MX_EVENT_HANDLER *event_handler );

extern mx_status_type mxsrv_mx_server_socket_process_event(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_SOCKET_HANDLER_LIST *socket_handler_list,
			MX_EVENT_HANDLER *event_handler );

extern mx_status_type mxsrv_mx_client_socket_init(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER_LIST *socket_handler_list,
			MX_EVENT_HANDLER *event_handler );

extern mx_status_type mxsrv_mx_client_socket_process_event(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_SOCKET_HANDLER_LIST *socket_handler_list,
			MX_EVENT_HANDLER *event_handler );

extern mx_status_type mxsrv_mx_client_socket_proc_queued_event(
			MX_RECORD *record_list,
			MX_QUEUED_EVENT *queued_event );

extern mx_status_type mxsrv_send_field_value_to_client(
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer,
			uint32_t message_type_for_client,
			uint32_t message_id_for_client );

extern mx_status_type mxsrv_handle_get_array(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_put_array(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer,
			void *received_value_ptr );

extern mx_status_type mxsrv_handle_get_network_handle(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field );

extern mx_status_type mxsrv_handle_get_field_type(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_SOCKET *mx_socket,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_get_attribute(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_set_attribute(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_set_client_info(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_get_option(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_set_option(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_get_64bit_option(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_set_64bit_option(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_add_callback(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_handle_delete_callback(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer );

extern mx_status_type mxsrv_ascii_client_socket_process_event(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_SOCKET_HANDLER_LIST *socket_handler_list,
			MX_EVENT_HANDLER *event_handler );

/*---*/

extern void mxsrv_process_sockets_with_select( MX_RECORD *record_list,
				MX_SOCKET_HANDLER_LIST *socket_handler_list );

extern void mxsrv_update_select_fds(
				MX_SOCKET_HANDLER_LIST *socket_handler_list );

/*---*/

#if HAVE_UNIX_DOMAIN_SOCKETS

extern mx_status_type mxsrv_get_unix_domain_socket_credentials(
			MX_SOCKET_HANDLER *socket_handler );

#endif

#endif /* __MS_MXSERVER_H__ */
