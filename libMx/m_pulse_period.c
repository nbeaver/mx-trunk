/*
 * Name:     m_pulse_period.c
 *
 * Purpose:  Implements preset pulse period measurements with pulse generators
 *           for MX scan records.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006, 2008, 2012, 2015, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_scan.h"
#include "mx_pulse_generator.h"

#include "mx_measurement.h"
#include "m_pulse_period.h"

MX_MEASUREMENT_FUNCTION_LIST mxm_preset_pulse_period_function_list = {
			mxm_preset_pulse_period_configure,
			mxm_preset_pulse_period_deconfigure,
			NULL,
			NULL,
			NULL,
			NULL,
			mxm_preset_pulse_period_acquire_data,
};

MX_EXPORT mx_status_type
mxm_preset_pulse_period_configure( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_pulse_period_configure()";

	MX_MEASUREMENT_PRESET_PULSE_PERIOD *preset_pulse_period_struct;
	MX_SCAN *scan;
	MX_RECORD *pulse_generator_record;
	MX_RECORD *input_device_record;
	char pulse_generator_record_name[MXU_RECORD_NAME_LENGTH + 1];
	double pulse_period;
	char format_buffer[20];
	int i, num_items;
	mx_status_type mx_status;

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MEASUREMENT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) measurement->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for measurement is NULL." );
	}

	preset_pulse_period_struct = (MX_MEASUREMENT_PRESET_PULSE_PERIOD *)
			malloc( sizeof(MX_MEASUREMENT_PRESET_PULSE_PERIOD) );

	if ( preset_pulse_period_struct ==
			(MX_MEASUREMENT_PRESET_PULSE_PERIOD *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a preset pulse period "
		"measurement structure." );
	}

	measurement->measurement_type_struct = preset_pulse_period_struct;

	/* Parse the measurement arguments string. */

	snprintf( format_buffer, sizeof(format_buffer),
			"%%lg %%%ds", MXU_RECORD_NAME_LENGTH );

	num_items = sscanf( measurement->measurement_arguments,
				format_buffer,
				&pulse_period,
				pulse_generator_record_name );

	if ( num_items != 2 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The 'measurement_arguments' field '%s' for scan '%s' does not contain "
	"an pulse period followed by a pulse_generator record name.",
			measurement->measurement_arguments,
			scan->record->name );
	}

	preset_pulse_period_struct->pulse_period = pulse_period;

	pulse_generator_record =
		mx_get_record( scan->record, pulse_generator_record_name );

	if ( pulse_generator_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"The pulse_generator record '%s' does not exist.",
			pulse_generator_record_name );
	}

	if ( mx_verify_driver_type( pulse_generator_record,
			MXR_DEVICE, MXC_PULSE_GENERATOR, MXT_ANY ) == FALSE )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' is not a pulse generator record.",
			pulse_generator_record->name );
	}

	preset_pulse_period_struct->pulse_generator_record
					= pulse_generator_record;

	/* Set the pulse generator to pulse mode, just in case it isn't
	 * in pulse mode already.
	 */

	mx_status = mx_pulse_generator_set_function_mode(
						pulse_generator_record,
						MXF_PGN_PULSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the scalers and timers to counter mode. */

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		input_device_record = ( scan->input_device_array )[i];

		if ( input_device_record->mx_superclass != MXR_DEVICE ) {
			continue;	/* Skip this record. */
		}

		mx_status = MX_SUCCESSFUL_RESULT;

		switch( input_device_record->mx_class ) {
		case MXC_SCALER:
			mx_status = mx_scaler_set_mode( input_device_record,
							MXCM_COUNTER_MODE );
			break;
		case MXC_TIMER:
			mx_status = mx_timer_set_mode( input_device_record,
							MXCM_COUNTER_MODE );
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_pulse_period_deconfigure( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_pulse_period_deconfigure()";

	MX_MEASUREMENT_PRESET_PULSE_PERIOD *preset_pulse_period_struct;
	MX_SCAN *scan;
	MX_RECORD *pulse_generator_record;
	MX_RECORD *input_device_record;
	int i;
	mx_status_type mx_status;

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MEASUREMENT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) measurement->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for measurement is NULL." );
	}

	preset_pulse_period_struct = ( MX_MEASUREMENT_PRESET_PULSE_PERIOD * )
				measurement->measurement_type_struct;

	if ( preset_pulse_period_struct ==
			( MX_MEASUREMENT_PRESET_PULSE_PERIOD *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The preset_pulse_period_struct pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	pulse_generator_record =
		preset_pulse_period_struct->pulse_generator_record;

	/* Restore the timers to preset mode and the scalers to counter mode. */

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		input_device_record = ( scan->input_device_array )[i];

		if ( input_device_record->mx_superclass != MXR_DEVICE ) {
			continue;	/* Skip this record. */
		}

		mx_status = MX_SUCCESSFUL_RESULT;

		switch( input_device_record->mx_class ) {
		case MXC_SCALER:
			mx_status = mx_scaler_set_mode( input_device_record,
							MXCM_COUNTER_MODE );
			break;
		case MXC_PULSE_GENERATOR:
			if ( input_device_record != pulse_generator_record ) {
				mx_status = mx_timer_set_mode( input_device_record,
							MXCM_PRESET_MODE );
			}
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( preset_pulse_period_struct != NULL ) {

		preset_pulse_period_struct->pulse_generator_record = NULL;

		preset_pulse_period_struct->pulse_period = 0.0;

		free( preset_pulse_period_struct );

		measurement->measurement_type_struct = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_pulse_period_acquire_data( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_pulse_period_acquire_data()";

	MX_MEASUREMENT_PRESET_PULSE_PERIOD *preset_pulse_period_struct;
	MX_SCAN *scan;
	MX_RECORD *pulse_generator_record;
	int interrupt;
	mx_bool_type busy;
	mx_status_type mx_status;

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MEASUREMENT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) measurement->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for measurement is NULL." );
	}

	preset_pulse_period_struct = (MX_MEASUREMENT_PRESET_PULSE_PERIOD *)
				measurement->measurement_type_struct;

	if ( preset_pulse_period_struct ==
			(MX_MEASUREMENT_PRESET_PULSE_PERIOD *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MEASUREMENT_PRESET_PULSE_PERIOD pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	pulse_generator_record =
		preset_pulse_period_struct->pulse_generator_record;

	if ( pulse_generator_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Pulse generator record pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	/* Clear the input devices. */

	mx_status = mx_clear_scan_input_devices( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the pulse generator pulse period and number of pulses.
	 * The pulse width and the pulse delay are left alone.
	 */

	mx_status = mx_pulse_generator_set_pulse_period( pulse_generator_record,
				preset_pulse_period_struct->pulse_period );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This function is intended for use by step scans, so we set the
	 * number of pulses to 1.
	 */

	mx_status = mx_pulse_generator_set_num_pulses(
						pulse_generator_record, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the measurement. */

	mx_status = mx_pulse_generator_start( pulse_generator_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the measurement to be over. */

	busy = TRUE;

	while ( busy ) {
		interrupt = mx_user_requested_interrupt_or_pause();

		switch ( interrupt ) {
		case MXF_USER_INT_NONE:
			break;

		case MXF_USER_INT_PAUSE:
			(void) mx_pulse_generator_stop(pulse_generator_record);

			return mx_error( MXE_PAUSE_REQUESTED, fname,
			"Pause requested by user." );

			break;
		default:
			(void) mx_pulse_generator_stop(pulse_generator_record);

			return mx_error( MXE_INTERRUPTED, fname,
			"Measurement was interrupted.");
		}

		mx_status = mx_pulse_generator_is_busy( pulse_generator_record,
							&busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(1);	/* Wait at least 1 millisecond. */
	}

	return MX_SUCCESSFUL_RESULT;
}

