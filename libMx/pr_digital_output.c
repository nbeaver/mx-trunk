/*
 * Name:    pr_digital_output.c
 *
 * Purpose: Functions used to process MX digital output record events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004 Illinois Institute of Technology
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
#include "mx_digital_output.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_digital_output_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_digital_output_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_DOU_VALUE:
			record_field->process_function
					= mx_digital_output_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_digital_output_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_digital_output_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_DIGITAL_OUTPUT *digital_output;
	unsigned long value;
	mx_status_type status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	digital_output = (MX_DIGITAL_OUTPUT *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_DOU_VALUE:
			status = mx_digital_output_read(record, &value);
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
		case MXLV_DOU_VALUE:
			status = mx_digital_output_write(record,
							digital_output->value);
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

	return status;
}

