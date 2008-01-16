/*
 * Name:    mx_process.h
 *
 * Purpose: Header file for MX event processing functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PROCESS_H__
#define __MX_PROCESS_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_osdef.h"
#include "mx_net.h"

#define MXU_ADDRESS_STRING_LENGTH	20

#define MXF_SRV_TCP_SERVER_TYPE		1
#define MXF_SRV_UNIX_SERVER_TYPE	2
#define MXF_SRV_MX_CLIENT_TYPE		3

#define MXF_SRVAUTH_NONE		1
#define MXF_SRVAUTH_UNIX_DOMAIN		2

struct mx_no_auth {
	int dummy;
};

#if HAVE_UNIX_DOMAIN_SOCKETS
struct mx_unix_domain_auth {
	uid_t real_uid;
	uid_t real_gid;
	uid_t effective_uid;
	uid_t effective_gid;
};
#endif

typedef struct {
	MX_SOCKET *synchronous_socket;
	MX_LIST_HEAD *list_head;
	long handler_array_index;
	struct mx_event_handler_type *event_handler;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	char client_address_string[MXU_ADDRESS_STRING_LENGTH + 1];
	char username[MXU_USERNAME_LENGTH + 1];
	char program_name[MXU_PROGRAM_NAME_LENGTH + 1];
	unsigned long process_id;
	unsigned long data_format;
	mx_bool_type truncate_64bit_longs;
	mx_bool_type network_debug;
	unsigned long last_rpc_message_id;
	unsigned long remote_header_length;

	long authentication_type;
	union {
		struct mx_no_auth none;
#if HAVE_UNIX_DOMAIN_SOCKETS
		struct mx_unix_domain_auth unix_domain;
#endif
	} auth;

} MX_SOCKET_HANDLER;

typedef struct {
	int max_sockets;
	int num_sockets_in_use;
	int handler_array_size;
	MX_SOCKET_HANDLER **array;
} MX_SOCKET_HANDLER_LIST;

/* Define values for the 'event_type' member of MX_QUEUED_EVENT. */

#define MXQ_NETWORK_MESSAGE	1

struct mx_queued_event_type {
	MX_SOCKET_HANDLER *socket_handler;
	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	long event_type;
	void *event_data;
	struct mx_queued_event_type *next_event;
};

typedef struct mx_queued_event_type MX_QUEUED_EVENT;

struct mx_event_handler_type {
	long type;
	char name[MXU_RECORD_NAME_LENGTH + 1];
	void *type_struct;

	mx_status_type ( *init )(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER_LIST *socket_handler_list,
			struct mx_event_handler_type *event_handler);

	mx_status_type ( *process_event )(
			MX_RECORD *record_list,
			MX_SOCKET_HANDLER *socket_handler,
			MX_SOCKET_HANDLER_LIST *socket_handler_list,
			struct mx_event_handler_type *event_handler);

	mx_status_type ( *process_queued_event )(
			MX_RECORD *record_list,
			MX_QUEUED_EVENT *queued_event);
};

typedef struct mx_event_handler_type MX_EVENT_HANDLER;

/*---*/

/* An MX process function is invoked when an MX_RECORD_FIELD is read or
 * written, unles the pointer to the function in the MX_RECORD_FIELD array
 * is NULL, or the pointer to the MX_RECORD_FIELD array is NULL.
 *
 * MX process functions are declared as follows:
 *
 * extern mx_status_type ( *mx_process_function )
 *	( void *record,			! MX_RECORD ptr
 *	  void *record_field,		! MX_RECORD_FIELD ptr
 *	  int operation );
 *
 * I would like to make the above a typedef, but there doesn't seem to be
 * a way to usefully make use of such a typedef, so all we're left with
 * is this comment.
 */

#define MX_PROCESS_GET    1
#define MX_PROCESS_PUT    2

/*---*/

MX_API mx_status_type mx_initialize_database_processing(
						MX_RECORD *record_list );

MX_API mx_status_type mx_initialize_record_processing( MX_RECORD *record );

MX_API mx_status_type mx_process_record_field(
				MX_RECORD *record,
				MX_RECORD_FIELD *record_field,
				int direction,
				mx_bool_type *value_changed );

MX_API mx_status_type mx_test_for_value_changed(
				MX_RECORD_FIELD *record_field,
				mx_bool_type *value_changed );

MX_API mx_status_type mx_default_test_for_value_changed(
				MX_RECORD_FIELD *record_field,
				mx_bool_type *value_changed );

MX_API mx_status_type mx_update_next_allowed_event_time(
				MX_RECORD *record,
				MX_RECORD_FIELD *record_field );

#ifdef __cplusplus
}
#endif

#endif /* __MX_PROCESS_H__ */

