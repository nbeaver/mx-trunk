/*
 * Name:    pr_pulser.c
 *
 * Purpose: Functions used to process MX pulse generator record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2002, 2004, 2006, 2012, 2015-2016, 2018
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
		case MXLV_PGN_ABORT:
		case MXLV_PGN_ARM:
		case MXLV_PGN_BUSY:
		case MXLV_PGN_FUNCTION_MODE:
		case MXLV_PGN_LAST_PULSE_NUMBER:
		case MXLV_PGN_NUM_PULSES:
		case MXLV_PGN_PULSE_DELAY:
		case MXLV_PGN_PULSE_PERIOD:
		case MXLV_PGN_PULSE_WIDTH:
		case MXLV_PGN_SETUP:
		case MXLV_PGN_START:
		case MXLV_PGN_STATUS:
		case MXLV_PGN_STOP:
		case MXLV_PGN_TRIGGER:
		case MXLV_PGN_TRIGGER_MODE:
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
	double double_value;
	long long_value;
	unsigned long ulong_value;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	pulse_generator = (MX_PULSE_GENERATOR *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_PGN_BUSY:
			mx_status = mx_pulse_generator_is_busy( record, NULL );
			break;
		case MXLV_PGN_FUNCTION_MODE:
			mx_status = mx_pulse_generator_get_function_mode(
								record, NULL );
			break;
		case MXLV_PGN_LAST_PULSE_NUMBER:
			mx_status = mx_pulse_generator_get_last_pulse_number(
								record, NULL );
			break;
		case MXLV_PGN_NUM_PULSES:
			mx_status = mx_pulse_generator_get_num_pulses(
								record, NULL );
			break;
		case MXLV_PGN_PULSE_DELAY:
			mx_status = mx_pulse_generator_get_pulse_delay(
								record, NULL );
			break;
		case MXLV_PGN_PULSE_PERIOD:
			mx_status = mx_pulse_generator_get_pulse_period(
								record, NULL );
			break;
		case MXLV_PGN_PULSE_WIDTH:
			mx_status = mx_pulse_generator_get_pulse_width(
								record, NULL );
			break;
		case MXLV_PGN_SETUP:
			mx_status = mx_pulse_generator_get_pulse_period( record,
								&double_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			pulse_generator->setup[MXSUP_PGN_PULSE_PERIOD]
								= double_value;

			mx_status = mx_pulse_generator_get_pulse_width( record,
								&double_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			pulse_generator->setup[MXSUP_PGN_PULSE_WIDTH]
								= double_value;

			mx_status = mx_pulse_generator_get_num_pulses( record,
								&ulong_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			pulse_generator->setup[MXSUP_PGN_NUM_PULSES]
								= ulong_value;

			mx_status = mx_pulse_generator_get_pulse_delay( record,
								&double_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			pulse_generator->setup[MXSUP_PGN_PULSE_DELAY]
								= double_value;

			mx_status = mx_pulse_generator_get_function_mode(
							record, &long_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			pulse_generator->setup[MXSUP_PGN_FUNCTION_MODE]
								= long_value;

			mx_status = mx_pulse_generator_get_trigger_mode(
							record, &long_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			pulse_generator->setup[MXSUP_PGN_TRIGGER_MODE]
								= long_value;
			break;
		case MXLV_PGN_STATUS:
			mx_status = mx_pulse_generator_get_status( record,
								NULL );
			break;
		case MXLV_PGN_TRIGGER_MODE:
			mx_status = mx_pulse_generator_get_trigger_mode(
								record, NULL );
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
		case MXLV_PGN_ABORT:
			mx_status = mx_pulse_generator_abort( record );
			break;
		case MXLV_PGN_ARM:
			mx_status = mx_pulse_generator_arm( record );
			break;
		case MXLV_PGN_FUNCTION_MODE:
			mx_status = mx_pulse_generator_set_function_mode(
					record, pulse_generator->function_mode);
			break;
		case MXLV_PGN_NUM_PULSES:
			mx_status = mx_pulse_generator_set_num_pulses( record,
						pulse_generator->num_pulses );
			break;
		case MXLV_PGN_PULSE_DELAY:
			mx_status = mx_pulse_generator_set_pulse_delay( record,
						pulse_generator->pulse_delay );
			break;
		case MXLV_PGN_PULSE_PERIOD:
			mx_status = mx_pulse_generator_set_pulse_period( record,
						pulse_generator->pulse_period );
			break;
		case MXLV_PGN_PULSE_WIDTH:
			mx_status = mx_pulse_generator_set_pulse_width( record,
						pulse_generator->pulse_width );
			break;
		case MXLV_PGN_SETUP:
			mx_status = mx_pulse_generator_setup( record,
		    pulse_generator->setup[MXSUP_PGN_PULSE_PERIOD],
		    pulse_generator->setup[MXSUP_PGN_PULSE_WIDTH],
		    mx_round( pulse_generator->setup[MXSUP_PGN_NUM_PULSES] ),
		    pulse_generator->setup[MXSUP_PGN_PULSE_DELAY],
		    mx_round( pulse_generator->setup[MXSUP_PGN_FUNCTION_MODE] ),
		    mx_round( pulse_generator->setup[MXSUP_PGN_TRIGGER_MODE] ));
			break;
		case MXLV_PGN_START:
			mx_status = mx_pulse_generator_start( record );
			break;
		case MXLV_PGN_STOP:
			mx_status = mx_pulse_generator_stop( record );
			break;
		case MXLV_PGN_TRIGGER:
			mx_status = mx_pulse_generator_trigger( record );
			break;
		case MXLV_PGN_TRIGGER_MODE:
			mx_status = mx_pulse_generator_set_trigger_mode(
					record, pulse_generator->trigger_mode );
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

