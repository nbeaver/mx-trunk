/*
 * Name:    pr_clock.c
 *
 * Purpose: Functions used to process MX time-of-day clock record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_clock.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_clock_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_clock_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_CLK_TIME:
		case MXLV_CLK_TIME_OFFSET:
		case MXLV_CLK_TIMESPEC:
		case MXLV_CLK_TIMESPEC_OFFSET:
			record_field->process_function
					    = mx_clock_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_clock_process_function( void *record_ptr,
			void *record_field_ptr,
			void *socket_handler_ptr,
			int operation )
{
	static const char fname[] = "mx_clock_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_CLOCK *clock;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	clock = (MX_CLOCK *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_CLK_TIMESPEC:
		case MXLV_CLK_TIME:
			mx_status = mx_clock_get_timespec( record, NULL );
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
		case MXLV_CLK_TIME_OFFSET:
			mx_status = mx_clock_set_time_offset( record,
							clock->time_offset);
			break;
		case MXLV_CLK_TIMESPEC_OFFSET:
			mx_status = mx_clock_set_timespec_offset( record,
							clock->timespec_offset);
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

