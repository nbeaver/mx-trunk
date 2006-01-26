/*
 * Name:    pr_list_head.c
 *
 * Purpose: Functions used to process events involving the MX database
 *          list head record.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_record.h"
#include "mx_list_head.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_list_head_process_functions( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_LHD_STATUS:
			record_field->process_function
					    = mx_list_head_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_list_head_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_list_head_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_LIST_HEAD *list_head;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	list_head = (MX_LIST_HEAD *) (record->record_superclass_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_LHD_STATUS:
			if ( list_head->is_server ) {
				if ( (strcmp("clients", list_head->status) == 0)
				  || (strcmp("users", list_head->status) == 0) )
				{
				    mx_status = mx_list_head_print_clients(
						record, list_head );
				} else {
				    return mx_error( MXE_ILLEGAL_ARGUMENT,
						fname,
				    "Unrecognized database status request '%s'",
				    		list_head->status );
				}
			} else {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"This process is not currently functioning as an active MX server." );
			}
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return mx_status;
}

mx_status_type
mx_list_head_print_clients( MX_RECORD *record, MX_LIST_HEAD *list_head )
{
	static const char fname[] = "mx_list_head_print_clients()";

	MX_SOCKET_HANDLER_LIST *socket_handler_list;
	MX_SOCKET_HANDLER *socket_handler;
	struct mx_event_handler_type *event_handler;
	int i;

	if ( list_head->application_ptr == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
    "The database list head application pointer has not been initialized." );
	}

	socket_handler_list = list_head->application_ptr;

	mx_info(
	"Num sock   type       host              user     pid  program" );
	mx_info(
	"---------------------------------------------------------------" );

	for ( i = 0; i < socket_handler_list->handler_array_size; i++ ) {
		socket_handler = socket_handler_list->array[i];

		if ( socket_handler != NULL ) {

			event_handler = socket_handler->event_handler;

			if ( event_handler == NULL ) {
				(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE,
		fname, "Event_handler pointer for client %d is NULL.", i );

				continue;
			}

			mx_info( "%3d %3d   %s  %-16s %-8s %6lu %-8s", i,
				socket_handler->synchronous_socket->socket_fd,
				event_handler->name,
				socket_handler->client_address_string,
				socket_handler->username,
				socket_handler->process_id,
				socket_handler->program_name );

		}
	}

	return MX_SUCCESSFUL_RESULT;
}

