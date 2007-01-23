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
 * Copyright 2003-2004, 2006-2007 Illinois Institute of Technology
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
#include "mx_memory.h"
#include "mx_bit.h"
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
		case MXLV_LHD_REPORT:
		case MXLV_LHD_REPORTALL:
		case MXLV_LHD_SUMMARY:
		case MXLV_LHD_FIELDDEF:
		case MXLV_LHD_SHOW_HANDLE:
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
			if ( (strcmp("clients", list_head->status) == 0)
			  || (strcmp("users", list_head->status) == 0) )
			{
			    if ( list_head->is_server ) {
				    mx_status = 
				    	mx_list_head_print_clients( list_head );
			    } else {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"This process is not currently functioning as an active MX server." );
			    }
			} else
			if (strcmp("cpu_type", list_head->status) == 0) {
				mx_status = 
					mx_list_head_show_cpu_type( list_head );
			} else
			if (strcmp("process_memory", list_head->status) == 0) {
				mx_status =
				  mx_list_head_show_process_memory( list_head );
			} else
			if (strcmp("system_memory", list_head->status) == 0) {
				mx_status =
				  mx_list_head_show_system_memory( list_head );
			} else {
				return mx_error( MXE_ILLEGAL_ARGUMENT,
						fname,
				    "Unrecognized database status request '%s'",
				    		list_head->status );
			}
			break;
		case MXLV_LHD_REPORT:
			mx_status = mx_list_head_record_report( list_head );
			break;
		case MXLV_LHD_REPORTALL:
			mx_status = mx_list_head_record_reportall( list_head );
			break;
		case MXLV_LHD_SUMMARY:
			mx_status = mx_list_head_record_summary( list_head );
			break;
		case MXLV_LHD_FIELDDEF:
			mx_status = mx_list_head_record_fielddef( list_head );
			break;
		case MXLV_LHD_SHOW_HANDLE:
			mx_status = mx_list_head_record_show_handle(list_head);
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
	}

	return mx_status;
}

mx_status_type
mx_list_head_print_clients( MX_LIST_HEAD *list_head )
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

			mx_info( "%3d %3d   %s  %-16s %-8s %6ld %-8s", i,
				socket_handler->synchronous_socket->socket_fd,
				event_handler->name,
				socket_handler->client_address_string,
				socket_handler->username,
				(long) socket_handler->process_id,
				socket_handler->program_name );

		}
	}

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_list_head_show_cpu_type( MX_LIST_HEAD *list_head )
{
	char architecture_type[80];
	char architecture_subtype[80];
	char buffer[80];
	unsigned long byteorder;
	mx_status_type mx_status;

	mx_info( "Computer '%s':", list_head->hostname );

	mx_status = mx_get_os_version_string( buffer, sizeof(buffer) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info( "  OS version = '%s'", buffer );

	mx_status = mx_get_cpu_architecture( architecture_type,
					sizeof(architecture_type),
					architecture_subtype,
					sizeof(architecture_subtype) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strlen(architecture_subtype) > 0 ) {
		mx_info( "  CPU type = '%s', subtype = '%s'",
			architecture_type, architecture_subtype );
	} else {
		mx_info( "  CPU type = '%s'", architecture_type );
	}

	byteorder = mx_native_byteorder();

	switch( byteorder ) {
	case MX_DATAFMT_BIG_ENDIAN:
		snprintf( buffer, sizeof(buffer),
			"  %d-bit big-endian computer", MX_WORDSIZE );
		break;
	case MX_DATAFMT_LITTLE_ENDIAN:
		snprintf( buffer, sizeof(buffer),
			"  %d-bit little-endian computer", MX_WORDSIZE );
		break;
	default:
		snprintf( buffer, sizeof(buffer),
			"  %d-bit unknown-endian computer", MX_WORDSIZE );
		break;
	}

	mx_info( buffer );

#if ( MX_PROGRAM_MODEL == MX_PROGRAM_MODEL_LP32 )
		mx_info(
	"  Program model = LP32  (16-bit int, 32-bit long, 32-bit ptr)" );

#elif ( MX_PROGRAM_MODEL == MX_PROGRAM_MODEL_ILP32 )
		mx_info(
	"  Program model = ILP32  (32-bit int, 32-bit long, 32-bit ptr)" );

#elif ( MX_PROGRAM_MODEL == MX_PROGRAM_MODEL_LLP64 )
		mx_info(
	"  Program model = LLP64  (32-bit int, 32-bit long, 64-bit ptr)" );

#elif ( MX_PROGRAM_MODEL == MX_PROGRAM_MODEL_LP64 )
		mx_info(
	"  Program model = LP64  (32-bit int, 64-bit long, 64-bit ptr)" );

#elif ( MX_PROGRAM_MODEL == MX_PROGRAM_MODEL_ILP64 )
		mx_info(
	"  Program model = ILP64  (64-bit int, 64-bit long, 64-bit ptr)" );

#elif ( MX_PROGRAM_MODEL == MX_PROGRAM_MODEL_UNKNOWN )
		mx_info( "  Program model = unknown" );

#else
		mx_info( "  Program model = %#x  (unrecognized value)",
			MX_PROGRAM_MODEL );
#endif

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_list_head_show_process_memory( MX_LIST_HEAD *list_head )
{
	mx_status_type mx_status;

	unsigned long process_id;
	MX_PROCESS_MEMINFO meminfo;

	process_id = mx_process_id();

	mx_info( "Process %lu memory usage:", process_id );

	mx_status = mx_get_process_meminfo( process_id, &meminfo );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_display_process_meminfo( &meminfo );

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_list_head_show_system_memory( MX_LIST_HEAD *list_head )
{
	mx_status_type mx_status;

	MX_SYSTEM_MEMINFO meminfo;

	mx_info( "Computer '%s' memory usage:", list_head->hostname );

	mx_status = mx_get_system_meminfo( &meminfo );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_display_system_meminfo( &meminfo );

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_list_head_record_report( MX_LIST_HEAD *list_head )
{
	MX_RECORD *record;
	char *record_name;
	mx_status_type mx_status;

	record_name = list_head->report;

	record = mx_get_record( list_head->record, record_name );

	if ( record == NULL ) {
		fprintf( stderr, "Record '%s' not found.\n", record_name );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_print_structure( stderr, record,
				MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY );

	return mx_status;
}

mx_status_type
mx_list_head_record_reportall( MX_LIST_HEAD *list_head )
{
	MX_RECORD *record;
	char *record_name;
	mx_status_type mx_status;

	record_name = list_head->reportall;

	record = mx_get_record( list_head->record, record_name );

	if ( record == NULL ) {
		fprintf( stderr, "Record '%s' not found.\n", record_name );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_print_structure( stderr, record, MXFF_SHOW_ALL );

	return mx_status;
}

mx_status_type
mx_list_head_record_summary( MX_LIST_HEAD *list_head )
{
	MX_RECORD *record;
	char *record_name;
	mx_status_type mx_status;

	record_name = list_head->summary;

	record = mx_get_record( list_head->record, record_name );

	if ( record == NULL ) {
		fprintf( stderr, "Record '%s' not found.\n", record_name );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_print_summary( stderr, record );

	return mx_status;
}

mx_status_type
mx_list_head_record_fielddef( MX_LIST_HEAD *list_head )
{
	MX_RECORD *record;
	char *record_name;
	mx_status_type mx_status;

	record_name = list_head->fielddef;

	record = mx_get_record( list_head->record, record_name );

	if ( record == NULL ) {
		fprintf( stderr, "Record '%s' not found.\n", record_name );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_print_field_definitions( stderr, record );

	return mx_status;
}

mx_status_type
mx_list_head_record_show_handle( MX_LIST_HEAD *list_head )
{
	static const char fname[] = "mx_list_head_record_show_handle()";

	void *record_ptr;
	MX_RECORD *record;
	MX_RECORD_FIELD *field;
	long record_handle, field_handle;
	mx_status_type mx_status;

	record_handle = list_head->show_handle[0];
	field_handle  = list_head->show_handle[1];

	/* Get the record pointer. */

	mx_status = mx_get_pointer_from_handle( &record_ptr,
						list_head->handle_table,
						record_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	record = (MX_RECORD *) record_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer returned for network field "
		"(%ld,%ld) is NULL.", record_handle, field_handle );
	}

	/* Get the field pointer. */

	if ( field_handle < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested field handle %ld for network field "
		"(%ld,%ld) is a negative number for record '%s'.",
			field_handle, record_handle, field_handle,
			record->name );
	}

	if ( field_handle >= record->num_record_fields ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested field handle %ld for network field "
		"(%ld,%ld) is greater than or equal to the "
		"number of record fields %ld for record '%s'.",
			field_handle, record_handle, field_handle,
			record->num_record_fields, record->name );
	}

	field = &(record->record_field_array[ field_handle ]);

	if ( field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD_FIELD pointer returned for "
		"network field (%ld,%ld) is NULL.",
			record_handle, field_handle );
	}

	fprintf(stderr, "Network field (%ld,%ld) = '%s.%s'\n",
		record_handle, field_handle,
		record->name, field->name );

	return MX_SUCCESSFUL_RESULT;
}

