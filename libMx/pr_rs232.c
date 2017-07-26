/*
 * Name:    pr_rs232.c
 *
 * Purpose: Functions used to process MX rs232 record events.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2005, 2010, 2012, 2014, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_rs232_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_rs232_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_232_DISCARD_UNREAD_INPUT:
		case MXLV_232_DISCARD_UNWRITTEN_OUTPUT:
		case MXLV_232_ECHO:
		case MXLV_232_GETCHAR:
		case MXLV_232_GETLINE:
		case MXLV_232_GET_CONFIGURATION:
		case MXLV_232_NUM_INPUT_BYTES_AVAILABLE:
		case MXLV_232_PUTCHAR:
		case MXLV_232_PUTLINE:
		case MXLV_232_SEND_BREAK:
		case MXLV_232_SERIAL_DEBUG:
		case MXLV_232_SET_CONFIGURATION:
		case MXLV_232_SIGNAL_STATE:
			record_field->process_function
					= mx_rs232_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_rs232_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_rs232_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_RS232 *rs232;
	unsigned long ulong_value;
	size_t bytes_read, bytes_written;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;

	record_field = (MX_RECORD_FIELD *) record_field_ptr;

	rs232 = (MX_RS232 *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( rs232->rs232_flags & MXF_232_NO_REMOTE_ACCESS ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"Direct remote access to RS-232 interface '%s' is currently disabled.",
			record->name );
	}

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_232_ECHO:
			mx_status = mx_rs232_get_echo( record, NULL );
			break;
		case MXLV_232_GETCHAR:
			mx_status = mx_rs232_getchar( record,
						&(rs232->getchar_value), 0 );
			break;
		case MXLV_232_GETLINE:
			mx_status = mx_rs232_getline( record,
						rs232->getline_buffer,
						MXU_RS232_BUFFER_LENGTH,
						&bytes_read, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			rs232->bytes_read = bytes_read;
			break;
		case MXLV_232_GET_CONFIGURATION:
			mx_status = mx_rs232_get_configuration( record,
						NULL, NULL, NULL, NULL,
						NULL, NULL, NULL, NULL );
			break;
		case MXLV_232_NUM_INPUT_BYTES_AVAILABLE:
			mx_status = mx_rs232_num_input_bytes_available( record,
								&ulong_value );

			rs232->num_input_bytes_available = ulong_value;
			break;
		case MXLV_232_SERIAL_DEBUG:
			mx_status = mx_rs232_get_serial_debug( record,
						&(rs232->serial_debug) );
			break;
		case MXLV_232_SIGNAL_STATE:
			mx_status = mx_rs232_get_signal_state( record, NULL );
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
		case MXLV_232_ECHO:
			mx_status = mx_rs232_set_echo( record, rs232->echo );
			break;
		case MXLV_232_PUTCHAR:
			mx_status = mx_rs232_putchar( record,
						rs232->putchar_value, 0 );
			break;
		case MXLV_232_PUTLINE:
			mx_status = mx_rs232_putline( record,
						rs232->putline_buffer,
						&bytes_written, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			rs232->bytes_written = bytes_written;
			break;
		case MXLV_232_DISCARD_UNREAD_INPUT:
			mx_status = mx_rs232_discard_unread_input( record, 0 );
			break;
		case MXLV_232_DISCARD_UNWRITTEN_OUTPUT:
			mx_status = mx_rs232_discard_unwritten_output(
								record, 0 );
			break;
		case MXLV_232_SEND_BREAK:
			mx_status = mx_rs232_send_break( record );
			break;
		case MXLV_232_SERIAL_DEBUG:
			mx_status = mx_rs232_set_serial_debug( record,
							rs232->serial_debug );
			break;
		case MXLV_232_SET_CONFIGURATION:
			mx_status = mx_rs232_set_configuration( record,
					rs232->speed, rs232->word_size,
					rs232->parity, rs232->stop_bits,
					rs232->flow_control,
					rs232->read_terminators,
					rs232->write_terminators,
					rs232->timeout );
			break;
		case MXLV_232_SIGNAL_STATE:
			mx_status = mx_rs232_set_signal_state( record,
							rs232->signal_state );
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

