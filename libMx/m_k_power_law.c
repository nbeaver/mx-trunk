/*
 * Name:     m_k_power_law.c
 *
 * Purpose: Implements xafs wavenumber scans for which the counting
 *          time follows a power law.
 *
 *                                      [ (    k    )                        ]
 *          counting_time = base_time * [ ( ------- ) ^ (power_law_exponent) ]
 *                                      [ ( k_start )                        ]
 *
 *          In C code, this can be written as
 *
 *            counting_time = base_time * pow((k/k_start), power_law_exponent);
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXM_K_POWER_LAW_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_scan.h"

#include "mx_measurement.h"
#include "m_k_power_law.h"

MX_MEASUREMENT_FUNCTION_LIST mxm_k_power_law_function_list = {
			mxm_k_power_law_configure,
			mxm_k_power_law_deconfigure,
			NULL,
			NULL,
			NULL,
			NULL,
			mxm_k_power_law_acquire_data,
};

MX_EXPORT mx_status_type
mxm_k_power_law_configure( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_k_power_law_configure()";

	MX_MEASUREMENT_K_POWER_LAW *k_power_law_struct;
	MX_SCAN *scan;
	MX_RECORD *timer_record;
	MX_RECORD *input_device_record;
	char timer_record_name[MXU_RECORD_NAME_LENGTH + 1];
	char format_buffer[40];
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

	k_power_law_struct = (MX_MEASUREMENT_K_POWER_LAW *)
			malloc( sizeof(MX_MEASUREMENT_K_POWER_LAW) );

	if ( k_power_law_struct == (MX_MEASUREMENT_K_POWER_LAW *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"a MX_MEASUREMENT_K_POWER_LAW measurement structure." );
	}

	k_power_law_struct->measurement_number = 0;

	measurement->measurement_type_struct = k_power_law_struct;

	/* Parse the measurement arguments string. */

	snprintf( format_buffer, sizeof(format_buffer),
			"%%lg %%lg %%lg %%lg %%%ds", MXU_RECORD_NAME_LENGTH );

#if MXM_K_POWER_LAW_DEBUG
	MX_DEBUG(-2,("%s: format_buffer = '%s', measurement_arguments = '%s'",
		fname, format_buffer, measurement->measurement_arguments ));
#endif

	num_items = sscanf( measurement->measurement_arguments,
				format_buffer,
				&(k_power_law_struct->base_time),
				&(k_power_law_struct->k_start),
				&(k_power_law_struct->delta_k),
				&(k_power_law_struct->exponent),
				timer_record_name );

	if ( num_items != 5 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"The 'measurement_arguments' field '%s' for scan '%s' does not contain "
	"a base time, a k start, a delta k setting, an exponent, "
	"and a timer record name.",
			measurement->measurement_arguments,
			scan->record->name );
	}

	timer_record = mx_get_record( scan->record, timer_record_name );

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"The timer record '%s' does not exist.",
			timer_record_name );
	}

	if ( (timer_record->mx_superclass != MXR_DEVICE )
	  || (timer_record->mx_class != MXC_TIMER ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' is not a timer record.",
			timer_record->name );
	}

	k_power_law_struct->timer_record = timer_record;

	/* Set the timer to preset mode. */

	mx_status = mx_timer_set_mode( timer_record, MXCM_PRESET_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure all other devices that can affect the gate signal
	 * are set to counter mode.
	 */

	mx_status = mx_timer_set_modes_of_associated_counters( timer_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the rest of the scalers and timers to counter mode. */

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
mxm_k_power_law_deconfigure( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_k_power_law_deconfigure()";

	MX_MEASUREMENT_K_POWER_LAW *k_power_law_struct;
	MX_SCAN *scan;
	MX_RECORD *timer_record;
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

	k_power_law_struct = ( MX_MEASUREMENT_K_POWER_LAW * )
				measurement->measurement_type_struct;

	if ( k_power_law_struct == ( MX_MEASUREMENT_K_POWER_LAW *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The k_power_law_struct pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	timer_record = k_power_law_struct->timer_record;

	/* Restore the timer back to preset mode. */

	mx_status = mx_timer_set_mode( timer_record, MXCM_PRESET_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the modes of any associated timers to preset mode and the modes
	 * of any assoicated scalers to count mode.
	 */

	mx_status = mx_timer_set_modes_of_associated_counters( timer_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Restore other timers to preset mode
	 * and other scalers to counter mode.
	 */

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
			if ( input_device_record != timer_record ) {
				mx_status = mx_timer_set_mode(
						input_device_record,
						MXCM_PRESET_MODE );
			}
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( k_power_law_struct != NULL ) {

		k_power_law_struct->timer_record = NULL;

		k_power_law_struct->base_time = 0.0;

		k_power_law_struct->k_start = 0.0;

		k_power_law_struct->delta_k = 0.0;

		free( k_power_law_struct );

		measurement->measurement_type_struct = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_k_power_law_acquire_data( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_k_power_law_acquire_data()";

	MX_MEASUREMENT_K_POWER_LAW *k_power_law_struct;
	MX_SCAN *scan;
	MX_RECORD *timer_record;
	double base_time, k_start, delta_k, exponent;
	double current_k, k_ratio, measurement_time;
	long measurement_number;
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

	k_power_law_struct = (MX_MEASUREMENT_K_POWER_LAW *)
				measurement->measurement_type_struct;

	if ( k_power_law_struct == (MX_MEASUREMENT_K_POWER_LAW *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MEASUREMENT_K_POWER_LAW pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	timer_record = k_power_law_struct->timer_record;

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Timer record pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	/* Clear the input devices. */

	mx_status = mx_clear_scan_input_devices( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the measurement time for this measurement. */

	base_time = k_power_law_struct->base_time;
	k_start   = k_power_law_struct->k_start;
	delta_k   = k_power_law_struct->delta_k;
	exponent  = k_power_law_struct->exponent;

	measurement_number = k_power_law_struct->measurement_number;

#if MXM_K_POWER_LAW_DEBUG
	MX_DEBUG(-2,
	("%s: (%ld)  base_time = %g, k_start = %g, delta_k = %g, exponent = %g",
		fname, measurement_number, base_time, k_start,
		delta_k, exponent));
#endif

	current_k = k_start + measurement_number * delta_k;

	k_ratio = mx_divide_safely( current_k, k_start );

	measurement_time = base_time * pow( k_ratio, exponent );

#if MXM_K_POWER_LAW_DEBUG
	MX_DEBUG(-2,
	("%s: (%ld)  current_k = %g, k_ratio = %g, measurement_time = %g",
	    fname, measurement_number, current_k, k_ratio, measurement_time));
#endif

	k_power_law_struct->measurement_number++;

	/* Start the measurement. */

	mx_status = mx_timer_start( timer_record, measurement_time );

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
			(void) mx_timer_stop( timer_record, NULL );

			return mx_error( MXE_PAUSE_REQUESTED, fname,
			"Pause requested by user." );

			break;
		default:
			(void) mx_timer_stop( timer_record, NULL );

			return mx_error( MXE_INTERRUPTED, fname,
			"Measurement was interrupted.");
		}

		mx_status = mx_timer_is_busy( timer_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(1);	/* Wait at least 1 millisecond. */
	}

	return MX_SUCCESSFUL_RESULT;
}

