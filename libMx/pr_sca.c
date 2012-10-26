/*
 * Name:    pr_sca.c
 *
 * Purpose: Functions used to process MX sca record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2002, 2006, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_sca.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_sca_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_sca_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_SCA_LOWER_LEVEL:
		case MXLV_SCA_UPPER_LEVEL:
		case MXLV_SCA_GAIN:
		case MXLV_SCA_TIME_CONSTANT:
		case MXLV_SCA_MODE:
			record_field->process_function
					    = mx_sca_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_sca_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_sca_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_SCA *sca;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	sca = (MX_SCA *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_SCA_LOWER_LEVEL:
			mx_status = mx_sca_get_lower_level( record, NULL );
			break;
		case MXLV_SCA_UPPER_LEVEL:
			mx_status = mx_sca_get_upper_level( record, NULL );
			break;
		case MXLV_SCA_GAIN:
			mx_status = mx_sca_get_gain( record, NULL );
			break;
		case MXLV_SCA_TIME_CONSTANT:
			mx_status = mx_sca_get_time_constant( record, NULL );
			break;
		case MXLV_SCA_MODE:
			mx_status = mx_sca_get_mode( record, NULL );
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
		case MXLV_SCA_LOWER_LEVEL:
			mx_status = mx_sca_set_lower_level( record,
							sca->lower_level );
			break;
		case MXLV_SCA_UPPER_LEVEL:
			mx_status = mx_sca_set_upper_level( record,
							sca->upper_level );
			break;
		case MXLV_SCA_GAIN:
			mx_status = mx_sca_set_gain( record, sca->gain );
			break;
		case MXLV_SCA_TIME_CONSTANT:
			mx_status = mx_sca_set_time_constant( record,
							sca->time_constant );
			break;
		case MXLV_SCA_MODE:
			mx_status = mx_sca_set_mode( record, sca->sca_mode );
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

