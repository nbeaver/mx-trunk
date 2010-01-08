/*
 * Name:     m_time.c
 *
 * Purpose:  Implements preset time measurements for MX scan records.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_CONFIGURE_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_hrt_debug.h"
#include "mx_scan.h"
#include "mx_timer.h"

#include "mx_measurement.h"
#include "m_time.h"

MX_MEASUREMENT_FUNCTION_LIST mxm_preset_time_function_list = {
			mxm_preset_time_configure,
			mxm_preset_time_deconfigure,
			mxm_preset_time_prescan_processing,
			mxm_preset_time_postscan_processing,
			mxm_preset_time_preslice_processing,
			mxm_preset_time_postslice_processing,
			mxm_preset_time_acquire_data,
};

MX_EXPORT mx_status_type
mxm_preset_time_configure( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_time_configure()";

	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	MX_SCAN *scan;
	MX_RECORD *timer_record;
	MX_RECORD *input_device_record;
	char timer_record_name[MXU_RECORD_NAME_LENGTH + 1];
	double integration_time;
	char format_buffer[20];
	int i, num_items;
	mx_status_type mx_status;

#if DEBUG_CONFIGURE_TIMING
	MX_HRT_TIMING set_mode_measurement;
	MX_HRT_TIMING associated_measurement;
	MX_HRT_TIMING input_device_measurement;
#endif

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MEASUREMENT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) measurement->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for measurement is NULL." );
	}

	preset_time_struct = (MX_MEASUREMENT_PRESET_TIME *)
				malloc( sizeof(MX_MEASUREMENT_PRESET_TIME) );

	if ( preset_time_struct == (MX_MEASUREMENT_PRESET_TIME *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Out of memory trying to allocate a preset time measurement structure." );
	}

	measurement->measurement_type_struct = preset_time_struct;

	/* Parse the measurement arguments string. */

	sprintf( format_buffer, "%%lg %%%ds", MXU_RECORD_NAME_LENGTH );

	num_items = sscanf( measurement->measurement_arguments,
				format_buffer,
				&integration_time,
				timer_record_name );

	if ( num_items != 2 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
"The 'measurement_arguments' field '%s' for scan '%s' does not contain "
"an integration time followed by a timer record name.",
			measurement->measurement_arguments,
			scan->record->name );
	}

	preset_time_struct->integration_time = integration_time;

	timer_record = mx_get_record( scan->record, timer_record_name );

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"The timer record '%s' does not exist.",
			timer_record_name );
	}

	if ( (timer_record->mx_superclass != MXR_DEVICE)
	  || (timer_record->mx_class != MXC_TIMER) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' is not a timer record.",
			timer_record->name );
	}

	preset_time_struct->timer_record = timer_record;

	/* Set the timer to preset mode. */

#if DEBUG_CONFIGURE_TIMING
	MX_DEBUG(-2,("%s: Invoking mx_timer_set_mode()", fname));
	MX_HRT_START( set_mode_measurement );
#endif

	mx_status = mx_timer_set_mode( timer_record, MXCM_PRESET_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if DEBUG_CONFIGURE_TIMING
	MX_HRT_END( set_mode_measurement );
	MX_HRT_RESULTS( set_mode_measurement, fname,
			"mx_timer_set_mode() complete." );

	MX_DEBUG(-2,
	("%s: Invoking mx_timer_set_modes_of_associated_counters()", fname));
	MX_HRT_START( associated_measurement );
#endif

	/* Make sure all other devices that can affect the gate signal
	 * are set to counter mode.
	 */

	mx_status = mx_timer_set_modes_of_associated_counters( timer_record );

#if DEBUG_CONFIGURE_TIMING
	MX_HRT_END( associated_measurement );
	MX_HRT_RESULTS( associated_measurement, fname,
		"mx_timer_set_modes_of_associated_counters() complete." );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the rest of the scalers and timers to counter mode. */

	for ( i = 0; i < scan->num_input_devices; i++ ) {

#if DEBUG_CONFIGURE_TIMING
		MX_HRT_START( input_device_measurement );
#endif
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
							MXCM_COUNTER_MODE );
			}
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if DEBUG_CONFIGURE_TIMING
		MX_HRT_END( input_device_measurement );
		MX_HRT_RESULTS( input_device_measurement, fname,
			"Input device %d", i );
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_time_deconfigure( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_time_deconfigure()";

	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	MX_SCAN *scan;
	MX_RECORD *timer_record;
	MX_RECORD *input_device_record;
	int i;
	mx_status_type mx_status;

#if DEBUG_CONFIGURE_TIMING
	MX_HRT_TIMING set_mode_measurement;
	MX_HRT_TIMING associated_measurement;
	MX_HRT_TIMING input_device_measurement;
#endif

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MEASUREMENT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) measurement->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for measurement is NULL." );
	}

	preset_time_struct = ( MX_MEASUREMENT_PRESET_TIME * )
				measurement->measurement_type_struct;

	if ( preset_time_struct == ( MX_MEASUREMENT_PRESET_TIME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The preset_time_struct pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	timer_record = preset_time_struct->timer_record;

	/* Restore the timer back to preset mode. */

#if DEBUG_CONFIGURE_TIMING
	MX_DEBUG(-2,("%s: Invoking mx_timer_set_mode()", fname));
	MX_HRT_START( set_mode_measurement );
#endif

	mx_status = mx_timer_set_mode( timer_record, MXCM_PRESET_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if DEBUG_CONFIGURE_TIMING
	MX_HRT_END( set_mode_measurement );
	MX_HRT_RESULTS( set_mode_measurement, fname,
			"mx_timer_set_mode() complete." );

	MX_DEBUG(-2,
	("%s: Invoking mx_timer_set_modes_of_associated_counters()", fname));
	MX_HRT_START( associated_measurement );
#endif

	/* Set the modes of any assocated timers to preset mode and the modes
	 * of any associated scalers to count mode.
	 */

	mx_status = mx_timer_set_modes_of_associated_counters( timer_record );

#if DEBUG_CONFIGURE_TIMING
	MX_HRT_END( associated_measurement );
	MX_HRT_RESULTS( associated_measurement, fname,
		"mx_timer_set_modes_of_associated_counters() complete." );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Restore other timers to preset mode 
	 * and other scalers to counter mode.
	 */

	for ( i = 0; i < scan->num_input_devices; i++ ) {

#if DEBUG_CONFIGURE_TIMING
		MX_HRT_START( input_device_measurement );
#endif
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

#if DEBUG_CONFIGURE_TIMING
		MX_HRT_END( input_device_measurement );
		MX_HRT_RESULTS( input_device_measurement, fname,
			"Input device %d", i );
#endif
	}

	if ( preset_time_struct != NULL ) {

		preset_time_struct->timer_record = NULL;

		preset_time_struct->integration_time = 0.0;

		free( preset_time_struct );

		measurement->measurement_type_struct = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_time_prescan_processing( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_time_postscan_processing( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_time_preslice_processing( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_time_postslice_processing( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_time_acquire_data( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_time_acquire_data()";

	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	MX_SCAN *scan;
	MX_RECORD *timer_record;
	double remaining_time;
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

	preset_time_struct = (MX_MEASUREMENT_PRESET_TIME *)
				measurement->measurement_type_struct;

	if ( preset_time_struct == (MX_MEASUREMENT_PRESET_TIME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MEASUREMENT_PRESET_TIME pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	timer_record = preset_time_struct->timer_record;

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Timer record pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	/* Clear the input devices. */

	mx_status = mx_clear_scan_input_devices( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the measurement. */

	mx_status = mx_timer_start( timer_record,
					preset_time_struct->integration_time );

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
			(void) mx_timer_stop( timer_record, &remaining_time );

			return mx_error( MXE_PAUSE_REQUESTED, fname,
			"Pause requested by user." );

			break;
		default:
			(void) mx_timer_stop( timer_record, &remaining_time );

			return mx_error( MXE_INTERRUPTED, fname,
			"Measurement integration time was interrupted.");
		}

		mx_status = mx_timer_is_busy( timer_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(1);	/* Wait at least 1 millisecond. */
	}

	return MX_SUCCESSFUL_RESULT;
}

