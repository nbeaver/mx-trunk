/*
 * Name:    pr_mce.c
 *
 * Purpose: Functions used to process MX mce record events.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2004, 2006, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_mce.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_mce_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_mce_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_MCE_CURRENT_NUM_VALUES:
		case MXLV_MCE_VALUE_ARRAY:
		case MXLV_MCE_NUM_MOTORS:
		case MXLV_MCE_MOTOR_RECORD_ARRAY:
		case MXLV_MCE_SELECTED_MOTOR_NAME:
			record_field->process_function
					= mx_mce_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_mce_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_mce_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MCE *mce;
	MX_RECORD *motor_record;
	mx_status_type status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	mce = (MX_MCE *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_MCE_CURRENT_NUM_VALUES:
			status = mx_mce_get_current_num_values( record, NULL );
			break;
		case MXLV_MCE_VALUE_ARRAY:
			status = mx_mce_read( record, NULL, NULL );
			break;
		case MXLV_MCE_NUM_MOTORS:
		case MXLV_MCE_MOTOR_RECORD_ARRAY:
			status = mx_mce_get_motor_record_array( record,
								NULL, NULL );
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
		case MXLV_MCE_SELECTED_MOTOR_NAME:
			motor_record = mx_get_record( record->list_head,
						mce->selected_motor_name );

			if ( motor_record == (MX_RECORD *) NULL ) {
				status = mx_error( MXE_NOT_FOUND, fname,
				  "Record '%s' was not found in this database.",
					mce->selected_motor_name );
			} else {
				status = mx_mce_connect_mce_to_motor( record,
							motor_record );
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

