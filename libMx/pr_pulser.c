/*
 * Name:    pr_pulser.c
 *
 * Purpose: Functions used to process MX pulse generator record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2002, 2004, 2006 Illinois Institute of Technology
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
#include "mx_pulse_generator.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_pulser_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_pulser_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_PGN_PULSE_PERIOD:
		case MXLV_PGN_PULSE_WIDTH:
		case MXLV_PGN_NUM_PULSES:
		case MXLV_PGN_PULSE_DELAY:
		case MXLV_PGN_MODE:
		case MXLV_PGN_BUSY:
		case MXLV_PGN_START:
		case MXLV_PGN_STOP:
			record_field->process_function
					    = mx_pulser_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_pulser_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_pulser_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_PULSE_GENERATOR *pulse_generator;
	mx_status_type status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	pulse_generator = (MX_PULSE_GENERATOR *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_PGN_PULSE_PERIOD:
			status = mx_pulse_generator_get_pulse_period(
								record, NULL );
			break;
		case MXLV_PGN_PULSE_WIDTH:
			status = mx_pulse_generator_get_pulse_width(
								record, NULL );
			break;
		case MXLV_PGN_NUM_PULSES:
			status = mx_pulse_generator_get_num_pulses(
								record, NULL );
			break;
		case MXLV_PGN_PULSE_DELAY:
			status = mx_pulse_generator_get_pulse_delay(
								record, NULL );
			break;
		case MXLV_PGN_MODE:
			status = mx_pulse_generator_get_mode( record, NULL );
			break;
		case MXLV_PGN_BUSY:
			status = mx_pulse_generator_is_busy( record, NULL );
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
		case MXLV_PGN_PULSE_PERIOD:
			status = mx_pulse_generator_set_pulse_period( record,
						pulse_generator->pulse_period );
			break;
		case MXLV_PGN_PULSE_WIDTH:
			status = mx_pulse_generator_set_pulse_width( record,
						pulse_generator->pulse_width );
			break;
		case MXLV_PGN_NUM_PULSES:
			status = mx_pulse_generator_set_num_pulses( record,
						pulse_generator->num_pulses );
			break;
		case MXLV_PGN_PULSE_DELAY:
			status = mx_pulse_generator_set_pulse_delay( record,
						pulse_generator->pulse_delay );
			break;
		case MXLV_PGN_MODE:
			status = mx_pulse_generator_set_mode( record,
						pulse_generator->mode );
			break;
		case MXLV_PGN_START:
			status = mx_pulse_generator_start( record );
			break;
		case MXLV_PGN_STOP:
			status = mx_pulse_generator_stop( record );
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

