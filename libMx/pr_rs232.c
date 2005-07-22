/*
 * Name:    pr_rs232.c
 *
 * Purpose: Functions used to process MX rs232 record events.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2005 Illinois Institute of Technology
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
		case MXLV_232_GETCHAR:
		case MXLV_232_PUTCHAR:
		case MXLV_232_NUM_INPUT_BYTES_AVAILABLE:
		case MXLV_232_DISCARD_UNREAD_INPUT:
		case MXLV_232_DISCARD_UNWRITTEN_OUTPUT:
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
	char char_value;
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
		case MXLV_232_GETCHAR:
			mx_status = mx_rs232_getchar( record, &char_value, 0 );

			rs232->getchar_value = (int) char_value;
			break;
		case MXLV_232_NUM_INPUT_BYTES_AVAILABLE:
			mx_status = mx_rs232_num_input_bytes_available( record,
								&ulong_value );

			rs232->num_input_bytes_available = ulong_value;
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
		case MXLV_232_PUTCHAR:
			char_value = (char) rs232->putchar_value;

			mx_status = mx_rs232_putchar( record, char_value, 0 );
			break;
		case MXLV_232_DISCARD_UNREAD_INPUT:
			mx_status = mx_rs232_discard_unread_input( record, 0 );
			break;
		case MXLV_232_DISCARD_UNWRITTEN_OUTPUT:
			mx_status = mx_rs232_discard_unwritten_output(
								record, 0 );
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
		break;
	}

	return mx_status;
}

