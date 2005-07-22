/*
 * Name:    pr_gpib.c
 *
 * Purpose: Functions used to process MX GPIB record events.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_gpib.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_gpib_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_gpib_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_GPIB_OPEN_DEVICE:
		case MXLV_GPIB_CLOSE_DEVICE:
		case MXLV_GPIB_READ:
		case MXLV_GPIB_WRITE:
		case MXLV_GPIB_GETLINE:
		case MXLV_GPIB_PUTLINE:
		case MXLV_GPIB_INTERFACE_CLEAR:
		case MXLV_GPIB_DEVICE_CLEAR:
		case MXLV_GPIB_SELECTIVE_DEVICE_CLEAR:
		case MXLV_GPIB_LOCAL_LOCKOUT:
		case MXLV_GPIB_REMOTE_ENABLE:
		case MXLV_GPIB_GO_TO_LOCAL:
		case MXLV_GPIB_TRIGGER:
		case MXLV_GPIB_WAIT_FOR_SERVICE_REQUEST:
		case MXLV_GPIB_SERIAL_POLL:
		case MXLV_GPIB_SERIAL_POLL_DISABLE:
			record_field->process_function
					= mx_gpib_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_gpib_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_gpib_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_GPIB *gpib;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;

	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( gpib->gpib_flags & MXF_GPIB_NO_REMOTE_ACCESS ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"Direct remote access to GPIB interface '%s' is currently disabled.",
			record->name );
	}

	if ( gpib->bytes_to_read > gpib->read_buffer_length ) {
		gpib->bytes_to_read = gpib->read_buffer_length;
	}
	if ( gpib->bytes_to_write > gpib->write_buffer_length ) {
		gpib->bytes_to_write = gpib->write_buffer_length;
	}

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_GPIB_READ:
			mx_status = mx_gpib_read( record, gpib->address,
						gpib->read_buffer,
						gpib->bytes_to_read,
						NULL, 0 );
			break;
		case MXLV_GPIB_GETLINE:
			mx_status = mx_gpib_getline( record, gpib->address,
						gpib->read_buffer,
						gpib->bytes_to_read,
						NULL, 0 );
			break;
		case MXLV_GPIB_SERIAL_POLL:
			mx_status = mx_gpib_serial_poll( record, gpib->address,
							&(gpib->serial_poll) );
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
		case MXLV_GPIB_OPEN_DEVICE:
			mx_status = mx_gpib_open_device( record,
							gpib->open_device );
			break;
		case MXLV_GPIB_CLOSE_DEVICE:
			mx_status = mx_gpib_close_device( record,
							gpib->close_device );
			break;
		case MXLV_GPIB_WRITE:
			mx_status = mx_gpib_write( record, gpib->address,
						gpib->write_buffer,
						gpib->bytes_to_write,
						NULL, 0 );
			break;
		case MXLV_GPIB_PUTLINE:
			mx_status = mx_gpib_putline( record, gpib->address,
						gpib->write_buffer,
						NULL, 0 );
			break;
		case MXLV_GPIB_INTERFACE_CLEAR:
			mx_status = mx_gpib_interface_clear( record );
			break;
		case MXLV_GPIB_DEVICE_CLEAR:
			mx_status = mx_gpib_device_clear( record );
			break;
		case MXLV_GPIB_SELECTIVE_DEVICE_CLEAR:
			mx_status = mx_gpib_selective_device_clear( record,
						gpib->selective_device_clear );
			break;
		case MXLV_GPIB_LOCAL_LOCKOUT:
			mx_status = mx_gpib_local_lockout( record );
			break;
		case MXLV_GPIB_REMOTE_ENABLE:
			mx_status = mx_gpib_remote_enable( record,
							gpib->remote_enable );
			break;
		case MXLV_GPIB_GO_TO_LOCAL:
			mx_status = mx_gpib_go_to_local( record,
							gpib->go_to_local );
			break;
		case MXLV_GPIB_TRIGGER:
			mx_status = mx_gpib_trigger( record, gpib->trigger );
			break;
		case MXLV_GPIB_WAIT_FOR_SERVICE_REQUEST:
			mx_status = mx_gpib_wait_for_service_request( record,
					    gpib->wait_for_service_request );
			break;
		case MXLV_GPIB_SERIAL_POLL_DISABLE:
			mx_status = mx_gpib_serial_poll_disable( record );
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

