/*
 * Name:    pr_analog_output.c
 *
 * Purpose: Functions used to process MX analog_output record events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2012, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_analog_output.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_analog_output_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_analog_output_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_AOU_VALUE:
			record_field->process_function
					= mx_analog_output_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_analog_output_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mx_analog_output_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_ANALOG_OUTPUT *analog_output;
	double value;
	mx_status_type status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	analog_output = (MX_ANALOG_OUTPUT *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AOU_RAW_VALUE:
		case MXLV_AOU_VALUE:
			status = mx_analog_output_read(record, &value);
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
		case MXLV_AOU_VALUE:
			status = mx_analog_output_write(record,
						analog_output->value);
			break;
		case MXLV_AOU_RAW_VALUE:
			switch( analog_output->subclass ) {
			case MXT_AOU_LONG:
				status = mx_analog_output_write_raw_long(
					record,
					analog_output->raw_value.long_value );
				break;
			case MXT_AOU_DOUBLE:
				status = mx_analog_output_write_raw_double(
					record,
					analog_output->raw_value.double_value );
				break;
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
	}

	return status;
}

