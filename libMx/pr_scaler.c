/*
 * Name:    pr_scaler.c
 *
 * Purpose: Functions used to process MX scaler record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
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
#include "mx_scaler.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_scaler_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_scaler_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_SCL_VALUE:
		case MXLV_SCL_RAW_VALUE:
		case MXLV_SCL_DARK_CURRENT:
		case MXLV_SCL_CLEAR:
		case MXLV_SCL_OVERFLOW_SET:
		case MXLV_SCL_BUSY:
		case MXLV_SCL_STOP:
		case MXLV_SCL_MODE:
			record_field->process_function
					    = mx_scaler_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_scaler_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_scaler_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_SCALER *scaler;
	mx_status_type status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	scaler = (MX_SCALER *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_SCL_VALUE:
			status = mx_scaler_read( record, NULL );
			break;
		case MXLV_SCL_RAW_VALUE:
			status = mx_scaler_read_raw( record, NULL );
			break;
		case MXLV_SCL_OVERFLOW_SET:
			status = mx_scaler_overflow_set( record, NULL );
			break;
		case MXLV_SCL_BUSY:
			status = mx_scaler_is_busy( record, NULL );
			break;
		case MXLV_SCL_MODE:
			status = mx_scaler_get_mode( record, NULL );
			break;
		case MXLV_SCL_DARK_CURRENT:
			status = mx_scaler_get_dark_current( record, NULL );
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
		case MXLV_SCL_VALUE:
			status = mx_scaler_start( record, scaler->value );
			break;
		case MXLV_SCL_CLEAR:
			status = mx_scaler_clear( record );
			break;
		case MXLV_SCL_STOP:
			status = mx_scaler_stop( record, NULL );
			break;
		case MXLV_SCL_MODE:
			status = mx_scaler_set_mode( record, scaler->mode );
			break;
		case MXLV_SCL_DARK_CURRENT:
			status = mx_scaler_set_dark_current( record,
							scaler->dark_current );
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

