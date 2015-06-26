/*
 * Name:    pr_operation.c
 *
 * Purpose: Functions used to process MX operation record events.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_operation.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_operation_process_functions( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_OP_STATUS:
		case MXLV_OP_START:
		case MXLV_OP_STOP:
			record_field->process_function
					    = mx_operation_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_operation_process_function( void *record_ptr,
			void *record_field_ptr, int action )
{
	static const char fname[] = "mx_operation_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_OPERATION *operation;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	operation = (MX_OPERATION *) (record->record_class_struct);

	MXW_SUPPRESS_SET_BUT_NOT_USED( operation );

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( action ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_OP_STATUS:
			mx_status = mx_operation_get_status( record, NULL );
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
		case MXLV_OP_START:
			mx_status = mx_operation_start( record );
			break;
		case MXLV_OP_STOP:
			mx_status = mx_operation_stop( record );
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
			"Unknown operation code = %d", action );
	}

	return mx_status;
}

