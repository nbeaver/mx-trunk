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
 * Copyright 2003-2004, 2006-2009, 2011-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_inttypes.h"
#include "mx_socket.h"
#include "mx_io.h"
#include "mx_record.h"
#include "mx_memory.h"
#include "mx_bit.h"
#include "mx_list_head.h"
#include "mx_dynamic_library.h"
#include "mx_time.h"

#include "mx_callback.h"
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
		case MXLV_LHD_BREAKPOINT:
		case MXLV_LHD_BREAKPOINT_NUMBER:
		case MXLV_LHD_CALLBACKS_ENABLED:
		case MXLV_LHD_CFLAGS:
		case MXLV_LHD_CRASH:
		case MXLV_LHD_DEBUG_LEVEL:
		case MXLV_LHD_DEBUGGER_STARTED:
		case MXLV_LHD_FIELDDEF:
		case MXLV_LHD_NUMBERED_BREAKPOINT_STATUS:
		case MXLV_LHD_POSIX_TIME:
		case MXLV_LHD_REPORT:
		case MXLV_LHD_REPORT_ALL:
		case MXLV_LHD_SHORT_ERROR_CODES:
		case MXLV_LHD_SHOW_CALLBACKS:
		case MXLV_LHD_SHOW_CALLBACK_ID:
		case MXLV_LHD_SHOW_FIELD:
		case MXLV_LHD_SHOW_HANDLE:
		case MXLV_LHD_SHOW_OPEN_FDS:
		case MXLV_LHD_SHOW_RECORD_LIST:
		case MXLV_LHD_SHOW_SOCKET_HANDLERS:
		case MXLV_LHD_SHOW_SOCKET_ID:
		case MXLV_LHD_STATUS:
		case MXLV_LHD_SUMMARY:
		case MXLV_LHD_UPDATE_ALL:
		case MXLV_LHD_VM_REGION:
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
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
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
		case MXLV_LHD_CFLAGS:
			/* Just return the value in the cflags field
			 * of the list head.
			 */
			break;
		case MXLV_LHD_DEBUG_LEVEL:
			list_head->debug_level = mx_get_debug_level();
			break;
		case MXLV_LHD_DEBUGGER_STARTED:
			list_head->debugger_started =
					mx_get_debugger_started_flag();
			break;
		case MXLV_LHD_BREAKPOINT_NUMBER:
			break;
		case MXLV_LHD_NUMBERED_BREAKPOINT_STATUS:
			list_head->numbered_breakpoint_status
				= mx_get_numbered_breakpoint(
					list_head->breakpoint_number );
			break;
		case MXLV_LHD_POSIX_TIME:
			list_head->posix_time = mx_posix_time();
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_LHD_DEBUG_LEVEL:
			mx_set_debug_level( (int) list_head->debug_level );
			break;
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
			} else
			if (strcmp("clock", list_head->status) == 0) {
				mx_info( "clock ticks per second = %g",
					mx_clock_ticks_per_second() );
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
		case MXLV_LHD_REPORT_ALL:
			mx_status = mx_list_head_record_report_all( list_head );
			break;
		case MXLV_LHD_UPDATE_ALL:
			mx_status = mx_list_head_record_update_all( list_head );
			break;
		case MXLV_LHD_SUMMARY:
			mx_status = mx_list_head_record_summary( list_head );
			break;
		case MXLV_LHD_SHOW_RECORD_LIST:
			mx_status =
			    mx_list_head_record_show_record_list( list_head );
			break;
		case MXLV_LHD_FIELDDEF:
			mx_status = mx_list_head_record_fielddef( list_head );
			break;
		case MXLV_LHD_SHOW_FIELD:
			mx_status = mx_list_head_record_show_field( list_head );
			break;
		case MXLV_LHD_SHOW_HANDLE:
			mx_status = mx_list_head_record_show_handle(list_head);
			break;
		case MXLV_LHD_SHOW_CALLBACKS:
			mx_status =
			    mx_list_head_record_show_callbacks( list_head );
			break;
		case MXLV_LHD_SHOW_CALLBACK_ID:
			mx_status =
			    mx_list_head_record_show_clbk_id( list_head );
			break;
		case MXLV_LHD_SHOW_SOCKET_HANDLERS:
			mx_status =
			  mx_list_head_record_show_socket_handlers( list_head );
			break;
		case MXLV_LHD_SHOW_SOCKET_ID:
			mx_status =
			    mx_list_head_record_show_socket_id( list_head );
			break;
		case MXLV_LHD_VM_REGION:
			return mx_error( MXE_UNSUPPORTED, fname,
			"This feature currently does not work." );

#if 0
			fprintf( stderr,
				(void *) list_head->vm_region[0],
				list_head->vm_region[1] );
#endif
			break;
		case MXLV_LHD_BREAKPOINT:
			if ( list_head->remote_breakpoint_enabled ) {
				mx_breakpoint();
			} else {
				mx_warning(
				"Request for remote breakpoint ignored." );
			}
			break;
		case MXLV_LHD_CRASH:
			if ( list_head->remote_breakpoint_enabled ) {
				/* Cause a segmentation fault by
				 * dereferencing a NULL pointer.
				 */

				int *crash;
				crash = NULL;

				*crash = 1 + *crash;
			} else {
				mx_warning(
				"Request for remote crash ignored." );
			}
			break;
		case MXLV_LHD_BREAKPOINT_NUMBER:
			break;
		case MXLV_LHD_NUMBERED_BREAKPOINT_STATUS:
			if ( list_head->remote_breakpoint_enabled ) {
				mx_set_numbered_breakpoint(
					list_head->breakpoint_number,
					list_head->numbered_breakpoint_status );
			} else {
				mx_warning( "Request for numbered breakpoint "
						"change ignored." );
			}
			break;
		case MXLV_LHD_DEBUGGER_STARTED:
			if ( list_head->debugger_started ) {
				mx_set_debugger_started_flag( 1 );
			} else {
				mx_set_debugger_started_flag( 0 );
			}
			break;
		case MXLV_LHD_SHOW_OPEN_FDS:
			mx_info( "Open file descriptors:" );
			mx_show_fd_names( mx_process_id() );
#if defined(OS_WIN32)
			mx_info( "Open sockets:" );
			mx_win32_show_socket_names();
#endif
			break;
		case MXLV_LHD_SHORT_ERROR_CODES:
		case MXLV_LHD_CALLBACKS_ENABLED:
			/* Nothing to do here. */
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
				(int) socket_handler->mx_socket->socket_fd,
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

#if ( defined(OS_WIN32) && (MX_PROGRAM_MODEL == MX_PROGRAM_MODEL_LLP64) )
		snprintf( buffer, sizeof(buffer),
			"  64-bit little-endian computer" );
#else
		snprintf( buffer, sizeof(buffer),
			"  %d-bit little-endian computer", MX_WORDSIZE );
#endif
		break;
	default:
		snprintf( buffer, sizeof(buffer),
			"  %d-bit unknown-endian computer", MX_WORDSIZE );
		break;
	}

	mx_info( "%s", buffer );

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
mx_list_head_record_report_all( MX_LIST_HEAD *list_head )
{
	MX_RECORD *record;
	char *record_name;
	mx_status_type mx_status;

	record_name = list_head->report_all;

	record = mx_get_record( list_head->record, record_name );

	if ( record == NULL ) {
		fprintf( stderr, "Record '%s' not found.\n", record_name );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_print_structure( stderr, record, MXFF_SHOW_ALL );

	return mx_status;
}

mx_status_type
mx_list_head_record_update_all( MX_LIST_HEAD *list_head )
{
	MX_RECORD *record;
	char *record_name;
	mx_status_type mx_status;

	record_name = list_head->update_all;

	record = mx_get_record( list_head->record, record_name );

	if ( record == NULL ) {
		fprintf( stderr, "Record '%s' not found.\n", record_name );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_initialize_record_processing( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_print_structure( stderr, record,
				MXFF_SHOW_ALL | MXFF_UPDATE_ALL );

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
mx_list_head_record_show_record_list( MX_LIST_HEAD *list_head )
{
	MX_RECORD *list_head_record, *current_record;
	mx_status_type mx_status;

	list_head_record = list_head->record;

	current_record = list_head_record;

	do {
		mx_status = mx_print_summary( stderr, current_record );

		current_record = current_record->next_record;

	} while ( current_record != list_head_record );

	MXW_UNUSED( mx_status );

	return MX_SUCCESSFUL_RESULT;
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

	mx_status = mx_print_all_field_definitions( stderr, record );

	return mx_status;
}

mx_status_type
mx_list_head_record_show_field( MX_LIST_HEAD *list_head )
{
	static const char fname[] = "mx_list_head_record_show_field()";

	MX_RECORD *mx_record = NULL;
	MX_RECORD_FIELD *mx_field = NULL;
	char *name_copy = NULL;
	char *record_name = NULL;
	char *field_name = NULL;
	char *ptr = NULL;
	void *value_pointer = NULL;
	long i;
	mx_status_type mx_status;

	name_copy = strdup( list_head->show_field );

	if ( name_copy == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to copy the string '%s'.",
			list_head->show_field );
	}

	record_name = name_copy;

	ptr = strchr( name_copy, '.' );

	if ( ptr == NULL ) {
		field_name = "";
	} else {
		*ptr = '\0';
		field_name = ++ptr;
	}

	fprintf( stderr, "Record field '%s.%s'\n\n", record_name, field_name );

	mx_record = mx_get_record( list_head->record, record_name );

	if ( mx_record == (MX_RECORD *) NULL ) {
		mx_status = mx_error( MXE_NOT_FOUND, fname,
			"The record '%s' was not found in the MX database.",
			record_name );

		mx_free( name_copy );
		return mx_status;
	}

	mx_status = mx_find_record_field( mx_record, field_name, &mx_field );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( name_copy );
		return mx_status;
	}

	mx_free( name_copy );

	value_pointer = mx_get_field_value_pointer( mx_field );

	fprintf( stderr, "  record pointer = %p\n\n", mx_record );

	fprintf( stderr, "  field pointer = %p\n", mx_field );
	fprintf( stderr, "  label_value = %ld\n", mx_field->label_value );
	fprintf( stderr, "  field_number = %ld\n", mx_field->field_number );
	fprintf( stderr, "  datatype = %ld\n", mx_field->datatype );
	fprintf( stderr, "  num_dimensions = %ld\n", mx_field->num_dimensions );
	fprintf( stderr, "  dimension = [" );
	for ( i = 0; i < mx_field->num_dimensions; i++ ) {
		fprintf( stderr, " %ld", mx_field->dimension[i] );
	}
	fprintf( stderr, " ]\n" );
	fprintf( stderr, "  data_element_size = [" );
	for ( i = 0; i < mx_field->num_dimensions; i++ ) {
		fprintf( stderr,
			" %ld", (long) mx_field->data_element_size[i] );
	}
	fprintf( stderr, " ]\n" );
	fprintf( stderr, "  data_pointer = %p\n", mx_field->data_pointer );
	fprintf( stderr, "  value pointer = %p\n", value_pointer );
	fprintf( stderr, "  process_function = %p\n",
					mx_field->process_function );
	fprintf( stderr, "  flags = %#lx\n", mx_field->flags );
	fprintf( stderr, "  timer_interval = %ld\n", mx_field->timer_interval );
	fprintf( stderr, "  value_change_threshold = %g\n",
					mx_field->value_change_threshold );
	fprintf( stderr, "  last_value = %g\n", mx_field->last_value );
	fprintf( stderr, "  value_has_changed_manual_override = %d\n",
			(int) mx_field->value_has_changed_manual_override );
	fprintf( stderr, "  value_changed_test_function = %p\n",
					mx_field->value_changed_test_function );
	fprintf( stderr, "  callback_list = %p\n", mx_field->callback_list );
	fprintf( stderr, "  application_ptr = %p\n", mx_field->application_ptr);
	fprintf( stderr, "  record = %p\n", mx_field->record );
	fprintf( stderr, "  active = %d\n", (int) mx_field->active );
	fprintf( stderr, "\n  value = " );

	if ( (mx_field->num_dimensions > 1)
	  || ((mx_field->datatype != MXFT_STRING)
		  && (mx_field->num_dimensions > 0)) )
	{
		mx_status = mx_print_field_array( stderr,
				mx_record, mx_field, TRUE );
	} else {
		mx_status = mx_print_field_value( stderr,
				mx_record, mx_field, value_pointer, TRUE );
	}

	fprintf( stderr, "\n" );

	return MX_SUCCESSFUL_RESULT;
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

	record_handle = (long) list_head->show_handle[0];
	field_handle  = (long) list_head->show_handle[1];

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

mx_status_type
mx_list_head_record_show_callbacks( MX_LIST_HEAD *list_head )
{
	MX_HANDLE_TABLE *callback_handle_table;
	MX_HANDLE_STRUCT *handle_struct, *handle_struct_array;
	MX_CALLBACK *callback;
	MX_LIST *callback_socket_handler_list;
	unsigned long i, num_table_entries;

	callback_handle_table = list_head->server_callback_handle_table;

	if ( callback_handle_table == (MX_HANDLE_TABLE *) NULL ) {
		fprintf(stderr, "No callbacks are active.\n");

		return MX_SUCCESSFUL_RESULT;
	}
	if ( callback_handle_table->handles_in_use == 0 ) {
		fprintf(stderr, "Callback table is empty.\n" );

		return MX_SUCCESSFUL_RESULT;
	}

	fprintf(stderr, "Callback table:\n");

	num_table_entries = callback_handle_table->num_blocks
				* callback_handle_table->block_size;

	handle_struct_array = callback_handle_table->handle_struct_array;

	for ( i = 0; i < num_table_entries; i++ ) {
		handle_struct = &handle_struct_array[i];

		callback = handle_struct->pointer;

		if ( callback != (MX_CALLBACK *) NULL ) {

			/* Find the list of socket handlers for this callback.*/

			callback_socket_handler_list =
				callback->callback_argument;

			if ( callback_socket_handler_list == NULL ) {
				fprintf(stderr, "ID = %#lx\n",
					(unsigned long) callback->callback_id );
			} else {
				fprintf(stderr,
				"ID = %#lx, num socket handlers = %lu\n",
					(unsigned long) callback->callback_id,
				callback_socket_handler_list->num_list_entries);
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_list_head_record_show_clbk_id( MX_LIST_HEAD *list_head )
{
	MX_HANDLE_TABLE *callback_handle_table;
	MX_HANDLE_STRUCT *handle_struct, *handle_struct_array;
	MX_CALLBACK *callback = NULL;
	MX_LIST *callback_socket_handler_list;
	MX_LIST_ENTRY *list_start, *list_entry;
	MX_CALLBACK_SOCKET_HANDLER_INFO *csh_info;
	MX_SOCKET_HANDLER *socket_handler;
	unsigned long i, num_table_entries;
	uint32_t callback_id;
	char function_name[100];
	mx_status_type mx_status;

	callback_id = list_head->show_callback_id;

	callback_handle_table = list_head->server_callback_handle_table;

	if ( callback_handle_table == (MX_HANDLE_TABLE *) NULL ) {
		fprintf(stderr, "No callbacks are active.\n");

		return MX_SUCCESSFUL_RESULT;
	}
	if ( callback_handle_table->handles_in_use == 0 ) {
		fprintf(stderr, "Callback table is empty.\n" );

		return MX_SUCCESSFUL_RESULT;
	}

	num_table_entries = callback_handle_table->num_blocks
				* callback_handle_table->block_size;

	handle_struct_array = callback_handle_table->handle_struct_array;

	for ( i = 0; i < num_table_entries; i++ ) {
		handle_struct = &handle_struct_array[i];

		callback = handle_struct->pointer;

		if ( ( callback != (MX_CALLBACK *) NULL )
		  && ( callback->callback_id == callback_id ) )
		{
			break;
		}
	}

	if ( i >= num_table_entries ) {
		fprintf( stderr, "Callback ID %#lx not found.\n",
			(unsigned long) callback_id );

		return MX_SUCCESSFUL_RESULT;
	}

	if ( callback->callback_function == NULL ) {
		strlcpy( function_name, "<null>", sizeof(function_name) );
	} else {
		mx_status = mx_dynamic_library_get_function_name_from_address(
				(void *) callback->callback_function,
				function_name, sizeof(function_name) );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_NOT_FOUND:
			snprintf( function_name, sizeof(function_name),
			"<not found, addr = %p>", callback->callback_function );
			break;
		case MXE_NOT_YET_IMPLEMENTED:
			snprintf( function_name, sizeof(function_name),
			"<not yet implemented, addr = %p>",
				callback->callback_function );
			break;
		case MXE_UNSUPPORTED:
			snprintf( function_name, sizeof(function_name),
			"<unsupported, addr = %p>",
				callback->callback_function );
			break;
		default:
			snprintf( function_name, sizeof(function_name),
			"<MX error %lu, addr = %p>",
				mx_status.code, callback->callback_function );
			break;
		}
	}

	fprintf( stderr, "Callback ID = %#lx, callback = %p, class = %ld, "
			"type = %ld, function = '%s'\n",
		(unsigned long) callback->callback_id, callback,
		callback->callback_class, callback->callback_type,
		function_name );

	/* Find the list of socket handlers for this callback.*/

	callback_socket_handler_list = callback->callback_argument;

	if ( callback_socket_handler_list == (MX_LIST *) NULL ) {
		fprintf( stderr, "  No socket handlers.\n" );
		return MX_SUCCESSFUL_RESULT;
	}

	fprintf( stderr, "  Num socket handlers = %lu\n",
		callback_socket_handler_list->num_list_entries );

	list_start = callback_socket_handler_list->list_start;

	if ( list_start == NULL ) {
		fprintf(stderr, "  Socket handler list is empty.\n" );

		return MX_SUCCESSFUL_RESULT;
	}

	list_entry = list_start;

	do {
		csh_info = list_entry->list_entry_data;

		if ( csh_info == (MX_CALLBACK_SOCKET_HANDLER_INFO *) NULL ) {
			fprintf( stderr,
				    "  Callback socket handler info (null)\n");
		} else {
			socket_handler = csh_info->socket_handler;

			if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
				fprintf( stderr,
				    "  Socket handler (null)\n" );
			} else {
				fprintf( stderr,
				    "  Socket handler %p, socket %d\n",
				socket_handler,
				  (int) socket_handler->mx_socket->socket_fd );

				fprintf( stderr,
				    "    Client '%s', username '%s'\n",
				socket_handler->client_address_string,
				socket_handler->username );

				fprintf( stderr,
				    "    Program name '%s', process id = %lu\n",
				socket_handler->program_name,
				socket_handler->process_id );
			}
		}

		list_entry = list_entry->next_list_entry;
	} while ( list_entry != list_start );

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_list_head_record_show_socket_handlers( MX_LIST_HEAD *list_head )
{
	MX_SOCKET_HANDLER_LIST *socket_handler_list = NULL;
	MX_SOCKET_HANDLER *socket_handler;
	int i;

	fprintf( stderr, "MX server socket handlers:\n" );

	/* A pointer to an MX server's socket_handler_list is stored by
	 * the server's main() function in the MX_LIST_HEAD struct's
	 * application_ptr.
	 */

	socket_handler_list = (MX_SOCKET_HANDLER_LIST *)
					list_head->application_ptr;

	fprintf( stderr,
	"Socket handler list = %p,  num sockets in use = %d\n",
		socket_handler_list, socket_handler_list->num_sockets_in_use );

	for ( i = 0; i < socket_handler_list->max_sockets; i++ ) {
		socket_handler = socket_handler_list->array[i];

		if ( socket_handler != (MX_SOCKET_HANDLER *) NULL ) {
			fprintf( stderr,
		  "socket_handler(%d) = %p, socket id = %d,\n"
		  "        username = '%s', program = '%s', process_id = %lu\n",
				i, socket_handler,
				(int) socket_handler->mx_socket->socket_fd,
				socket_handler->username,
				socket_handler->program_name,
				socket_handler->process_id );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_list_head_record_show_socket_id( MX_LIST_HEAD *list_head )
{
	MX_SOCKET_HANDLER_LIST *socket_handler_list = NULL;
	MX_SOCKET_HANDLER *socket_handler = NULL;
	MX_EVENT_HANDLER *event_handler = NULL;
	int i, socket_fd;

	socket_fd = list_head->show_socket_id;

	/* A pointer to an MX server's socket_handler_list is stored by
	 * the server's main() function in the MX_LIST_HEAD struct's
	 * application_ptr.
	 */

	socket_handler_list = (MX_SOCKET_HANDLER_LIST *)
					list_head->application_ptr;

	for ( i = 0; i < socket_handler_list->max_sockets; i++ ) {
		socket_handler = socket_handler_list->array[i];

		if (socket_fd == socket_handler->mx_socket->socket_fd) {
			break;
		}
	}

	if ( i >= socket_handler_list->max_sockets ) {
		fprintf( stderr, "Socket fd %d not found.\n", socket_fd );
		return MX_SUCCESSFUL_RESULT;
	}

	fprintf( stderr, "\nSocket %d, socket_handler = %p\n",
				socket_fd, socket_handler );
	fprintf( stderr, "  handler_array_index = %ld, message_buffer = %p\n",
				socket_handler->handler_array_index,
				socket_handler->message_buffer );
	fprintf( stderr,
		"  username = '%s', program_name = '%s', process_id = %lu\n",
		socket_handler->username, socket_handler->program_name,
		socket_handler->process_id );
	fprintf( stderr, "  data_format = %lu, use_64bit_network_longs = %d\n",
		socket_handler->data_format,
		(int) socket_handler->use_64bit_network_longs );
	fprintf( stderr,
		"  network_debug_flags = %#lx, last_rpc_message_id = %#lx\n",
		socket_handler->network_debug_flags,
		socket_handler->last_rpc_message_id );
	fprintf( stderr,
		"  remote_header_length = %lu, remote_mx_version = %lu\n",
		socket_handler->remote_header_length,
		socket_handler->remote_mx_version );
	fprintf( stderr,
		"  remote_mx_version_time = %" PRIu64
		", authentication_type = %ld\n",
		socket_handler->remote_mx_version_time,
		socket_handler->authentication_type );

	event_handler = socket_handler->event_handler;

	fprintf( stderr,
		"  event_handler = %p, type = %ld, name = '%s'\n",
		event_handler, event_handler->type, event_handler->name );

	return MX_SUCCESSFUL_RESULT;
}

