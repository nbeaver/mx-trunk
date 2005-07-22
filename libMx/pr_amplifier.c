/*
 * Name:    pr_amplifier.c
 *
 * Purpose: Functions used to process MX amplifier record events.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
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
#include "mx_amplifier.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_amplifier_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_amplifier_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_AMP_GAIN:
		case MXLV_AMP_OFFSET:
		case MXLV_AMP_TIME_CONSTANT:
		case MXLV_AMP_GAIN_RANGE:
			record_field->process_function
					    = mx_amplifier_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_amplifier_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_amplifier_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AMPLIFIER *amplifier;
	mx_status_type status;

	double double_value;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	amplifier = (MX_AMPLIFIER *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AMP_GAIN:
			status = mx_amplifier_get_gain( record, &double_value );
			break;
		case MXLV_AMP_OFFSET:
			status = mx_amplifier_get_offset(
						record, &double_value );
			break;
		case MXLV_AMP_TIME_CONSTANT:
			status = mx_amplifier_get_time_constant(
						record, &double_value );
			break;
		case MXLV_AMP_GAIN_RANGE:
			status = mx_amplifier_get_gain_range( record, NULL );
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
		case MXLV_AMP_GAIN:
			status = mx_amplifier_set_gain(
						record, amplifier->gain );
			break;
		case MXLV_AMP_OFFSET:
			status = mx_amplifier_set_offset(
						record, amplifier->offset );
			break;
		case MXLV_AMP_TIME_CONSTANT:
			status = mx_amplifier_set_time_constant(
					record, amplifier->time_constant );
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

