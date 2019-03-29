/*
 * Name:    pr_timer.c
 *
 * Purpose: Functions used to process MX timer record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006, 2012, 2019
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_timer.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_timer_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_timer_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_TIM_VALUE:
		case MXLV_TIM_BUSY:
		case MXLV_TIM_STOP:
		case MXLV_TIM_CLEAR:
		case MXLV_TIM_MODE:
		case MXLV_TIM_LAST_MEASUREMENT_TIME:
			record_field->process_function
					    = mx_timer_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_timer_process_function( void *record_ptr,
			void *record_field_ptr,
			void *socket_handler_ptr,
			int operation )
{
	static const char fname[] = "mx_timer_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_TIMER *timer;
	mx_status_type status;

	double seconds;
	long mode;
	mx_bool_type busy;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	timer = (MX_TIMER *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_TIM_VALUE:
			status = mx_timer_read( record, &seconds );
		case MXLV_TIM_BUSY:
			status = mx_timer_is_busy( record, &busy );
			break;
		case MXLV_TIM_MODE:
			status = mx_timer_get_mode( record, &mode );
			break;
		case MXLV_TIM_LAST_MEASUREMENT_TIME:
			status = mx_timer_get_last_measurement_time( record,
									NULL );
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
		case MXLV_TIM_VALUE:
			status = mx_timer_start( record, timer->value );
			break;
		case MXLV_TIM_STOP:
			status = mx_timer_stop( record, &seconds );
			break;
		case MXLV_TIM_CLEAR:
			status = mx_timer_clear( record );
			break;
		case MXLV_TIM_MODE:
			status = mx_timer_set_mode( record, timer->mode );
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

