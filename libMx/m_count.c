/*
 * Name:     m_count.c
 *
 * Purpose:  Implements preset count measurements for MX scan records.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2008, 2012 Illinois Institute of Technology
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
#include "mx_scaler.h"

#include "mx_measurement.h"
#include "m_count.h"

MX_MEASUREMENT_FUNCTION_LIST mxm_preset_count_function_list = {
			mxm_preset_count_configure,
			mxm_preset_count_deconfigure,
			mxm_preset_count_prescan_processing,
			mxm_preset_count_postscan_processing,
			mxm_preset_count_preslice_processing,
			mxm_preset_count_postslice_processing,
			mxm_preset_count_acquire_data,
};

MX_EXPORT mx_status_type
mxm_preset_count_configure( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_count_configure()";

	MX_MEASUREMENT_PRESET_COUNT *preset_count_struct;
	MX_SCAN *scan;
	MX_RECORD *scaler_record;
	MX_RECORD *input_device_record;
	char scaler_record_name[MXU_RECORD_NAME_LENGTH + 1];
	long preset_count;
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

	preset_count_struct = (MX_MEASUREMENT_PRESET_COUNT *)
				malloc( sizeof(MX_MEASUREMENT_PRESET_COUNT) );

	if ( preset_count_struct == (MX_MEASUREMENT_PRESET_COUNT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Out of memory trying to allocate a preset time measurement structure." );
	}

	measurement->measurement_type_struct = preset_count_struct;

	/* Parse the measurement arguments string. */

	sprintf( format_buffer, "%%lu %%%ds", MXU_RECORD_NAME_LENGTH );

	num_items = sscanf( measurement->measurement_arguments,
				format_buffer,
				&preset_count,
				scaler_record_name );

	if ( num_items != 2 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
"The 'measurement_arguments' field '%s' for scan '%s' does not contain "
"a preset count value followed by a scaler record name.",
			measurement->measurement_arguments,
			scan->record->name );
	}

	preset_count_struct->preset_count = preset_count;

	scaler_record = mx_get_record( scan->record, scaler_record_name );

	if ( scaler_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"The scaler record '%s' does not exist.",
			scaler_record_name );
	}

	if ( (scaler_record->mx_superclass != MXR_DEVICE)
	  || (scaler_record->mx_class != MXC_SCALER) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' is not a scaler record.",
			scaler_record->name );
	}

	preset_count_struct->scaler_record = scaler_record;

	/* Set the scaler to preset mode. */

	mx_status = mx_scaler_set_mode( scaler_record, MXCM_PRESET_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure all other devices that can affect the gate signal
	 * are set to counter mode.
	 */

	mx_status = mx_scaler_set_modes_of_associated_counters( scaler_record );

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
			if ( input_device_record != scaler_record ) {

				mx_status = mx_scaler_set_mode(input_device_record,
							MXCM_COUNTER_MODE );
			}
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
mxm_preset_count_deconfigure( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_count_deconfigure()";

	MX_MEASUREMENT_PRESET_COUNT *preset_count_struct;
	MX_SCAN *scan;
	MX_RECORD *scaler_record;
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

	preset_count_struct = (MX_MEASUREMENT_PRESET_COUNT *)
				measurement->measurement_type_struct;

	if ( preset_count_struct == ( MX_MEASUREMENT_PRESET_COUNT * ) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The preset_count_struct pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	scaler_record = preset_count_struct->scaler_record;

	/* Restore the scaler back to counter mode. */

	mx_status = mx_scaler_set_mode( scaler_record, MXCM_COUNTER_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the modes of any associated timers to preset mode and the modes
	 * of any associated scalers to count mode.
	 */

	mx_status = mx_scaler_set_modes_of_associated_counters( scaler_record );

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
			if ( input_device_record != scaler_record ) {

				mx_status = mx_scaler_set_mode(
						input_device_record,
						MXCM_COUNTER_MODE );
			}
			break;
		case MXC_TIMER:
			mx_status = mx_timer_set_mode( input_device_record,
							MXCM_PRESET_MODE );
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( preset_count_struct != NULL ) {

		preset_count_struct->scaler_record = NULL;

		preset_count_struct->preset_count = 0;

		free( preset_count_struct );

		measurement->measurement_type_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_count_prescan_processing( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_count_postscan_processing( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_count_preslice_processing( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_count_postslice_processing( MX_MEASUREMENT *measurement )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxm_preset_count_acquire_data( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxm_preset_count_acquire_data()";

	MX_MEASUREMENT_PRESET_COUNT *preset_count_struct;
	MX_SCAN *scan;
	MX_RECORD *scaler_record;
	long current_counts;
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

	preset_count_struct = (MX_MEASUREMENT_PRESET_COUNT *)
				measurement->measurement_type_struct;

	if ( preset_count_struct == (MX_MEASUREMENT_PRESET_COUNT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MEASUREMENT_PRESET_COUNT pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	scaler_record = preset_count_struct->scaler_record;

	if ( scaler_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Scaler record pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	/* Clear the input devices. */

	mx_status = mx_clear_scan_input_devices( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the measurement. */

	mx_status = mx_scaler_start( scaler_record,
					preset_count_struct->preset_count );

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
			(void) mx_scaler_stop( scaler_record, &current_counts );

			return mx_error( MXE_PAUSE_REQUESTED, fname,
			"Pause requested by user." );

			break;
		default:
			(void) mx_scaler_stop( scaler_record, &current_counts );

			return mx_error( MXE_INTERRUPTED, fname,
			"Measurement was interrupted.");

			break;
		}

		mx_status = mx_scaler_is_busy( scaler_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(1);	/* Wait at least 1 millisecond. */
	}

	return MX_SUCCESSFUL_RESULT;
}

