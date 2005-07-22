/*
 * Name:    d_auto_filter_amplifier.c
 *
 * Purpose: Driver for autoscaling of filter settings with associated
 *          autoscaling of an amplifier when the filter settings change.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_record.h"
#include "mx_driver.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_digital_output.h"
#include "mx_amplifier.h"
#include "mx_autoscale.h"
#include "d_auto_amplifier.h"
#include "d_auto_filter_amplifier.h"

MX_RECORD_FUNCTION_LIST mxd_auto_filter_amp_record_function_list = {
	mxd_auto_filter_amp_initialize_type,
	mxd_auto_filter_amp_create_record_structures,
	mxd_auto_filter_amp_finish_record_initialization,
	mxd_auto_filter_amp_delete_record,
	NULL,
	mxd_auto_filter_amp_dummy_function,
	mxd_auto_filter_amp_dummy_function,
	mxd_auto_filter_amp_open,
	mxd_auto_filter_amp_dummy_function
};

MX_AUTOSCALE_FUNCTION_LIST mxd_auto_filter_amp_autoscale_function_list = {
	mxd_auto_filter_amp_read_monitor,
	mxd_auto_filter_amp_get_change_request,
	mxd_auto_filter_amp_change_control,
	mxd_auto_filter_amp_get_offset_index,
	mxd_auto_filter_amp_set_offset_index,
	mxd_auto_filter_amp_get_parameter,
	mxd_auto_filter_amp_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_auto_filter_amp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AUTOSCALE_STANDARD_FIELDS,
	MX_AUTO_FILTER_AMP_STANDARD_FIELDS
};

long mxd_auto_filter_amp_num_record_fields
	= sizeof( mxd_auto_filter_amp_record_field_defaults )
	/ sizeof( mxd_auto_filter_amp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_auto_filter_amp_rfield_def_ptr
		= &mxd_auto_filter_amp_record_field_defaults[0];

static mx_status_type
mxd_auto_filter_amp_get_pointers( MX_AUTOSCALE *autoscale,
			MX_AUTO_FILTER_AMP **auto_filter_amp,
			const char *calling_fname )
{
	const char fname[] = "mxd_auto_filter_amp_get_pointers()";

	if ( autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AUTOSCALE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( auto_filter_amp == (MX_AUTO_FILTER_AMP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AUTO_FILTER_AMP pointer passed was NULL." );
	}
	if ( autoscale->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "The MX_RECORD pointer for the MX_AUTOSCALE structure passed is NULL." );
	}
	if ( autoscale->record->mx_class != MXC_AUTOSCALE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' passed is not an autoscale record.",
			autoscale->record->name );
	}

	*auto_filter_amp = (MX_AUTO_FILTER_AMP *)
				autoscale->record->record_type_struct;

	if ( *auto_filter_amp == (MX_AUTO_FILTER_AMP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AUTO_FILTER_AMP pointer for record '%s' is NULL.",
				autoscale->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*****/

MX_EXPORT mx_status_type
mxd_auto_filter_amp_initialize_type( long record_type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_auto_filter_amp_create_record_structures()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_FILTER_AMP *auto_filter_amp;

	/* Allocate memory for the necessary structures. */

	autoscale = (MX_AUTOSCALE *) malloc( sizeof(MX_AUTOSCALE) );

	if ( autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AUTOSCALE structure." );
	}

	auto_filter_amp = (MX_AUTO_FILTER_AMP *)
				malloc( sizeof(MX_AUTO_FILTER_AMP) );

	if ( auto_filter_amp == (MX_AUTO_FILTER_AMP *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AUTO_FILTER_AMP structure." );
	}

	/* Now set up the necessary pointers. */

	autoscale->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = autoscale;
	record->record_type_struct = auto_filter_amp;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list =
			&mxd_auto_filter_amp_autoscale_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_finish_record_initialization( MX_RECORD *record )
{
	const char fname[]
		= "mxd_auto_filter_amp_finish_record_initialization()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_FILTER_AMP *auto_filter_amp;
	MX_RECORD *monitor_record, *filter_record, *timer_record;
	MX_RECORD *amplifier_autoscale_record;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	autoscale = (MX_AUTOSCALE *) record->record_class_struct;

	mx_status = mxd_auto_filter_amp_get_pointers( autoscale,
						&auto_filter_amp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	monitor_record = autoscale->monitor_record;

	if ( monitor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The monitor_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( monitor_record->mx_class != MXC_SCALER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Filter monitor record '%s' is not a scaler record.",
			monitor_record->name );
	}

	filter_record = autoscale->control_record;

	if ( filter_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The control_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( filter_record->mx_class != MXC_DIGITAL_OUTPUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Filter control record '%s' is not an filter record.",
			filter_record->name );
	}

	timer_record = autoscale->timer_record;

	if ( timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The timer_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( timer_record->mx_class != MXC_TIMER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Filter timer record '%s' is not an timer record.",
			timer_record->name );
	}

	amplifier_autoscale_record =
		auto_filter_amp->amplifier_autoscale_record;

	if ( amplifier_autoscale_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The amplifier_autoscale_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( amplifier_autoscale_record->mx_type != MXT_AUT_AMPLIFIER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The amplifier_autoscale_record '%s' used by record '%s' "
		"is not of type 'autoscale_amplifier'.",
			amplifier_autoscale_record->name, record->name );
	}

	/* Initialize record values. */

	autoscale->monitor_value = 0L;

	/* The monitor offset value should not be affected by the filter
	 * setting, so there should be only one monitor offset.  Thus,
	 * we can go ahead and allocate memory for it here.
	 */

	autoscale->num_monitor_offsets = 1L;

	mx_status = mx_autoscale_create_monitor_offset_array( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale->last_limit_tripped = MXF_AUTO_NO_LIMIT_TRIPPED;

	/* Initialize the monitor offset to zero. */

	autoscale->monitor_offset_array[0] = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_delete_record( MX_RECORD *record )
{
	MX_AUTOSCALE *autoscale;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}

	autoscale = (MX_AUTOSCALE *) record->record_class_struct;

	if ( autoscale != NULL ) {

		if ( autoscale->monitor_offset_array != NULL ) {
			free( autoscale->monitor_offset_array );

			autoscale->monitor_offset_array = NULL;
		}

		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_open( MX_RECORD *record )
{
	return mx_autoscale_read_monitor( record, NULL );
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_dummy_function( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_read_monitor( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_filter_amp_read_monitor()";

	long scaler_value, offset;
	double offset_per_second, last_measurement_time;
	mx_status_type mx_status;

	if ( autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AUTOSCALE pointer passed was NULL." );
	}

	mx_status = mx_scaler_read( autoscale->monitor_record,
						&scaler_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: scaler_value = %ld", fname, scaler_value));

	/* For this autoscale driver, there is only one monitor offset. */

	offset_per_second = autoscale->monitor_offset_array[0];

	MX_DEBUG( 2,("%s: offset_per_second = %g", fname, offset_per_second));

	mx_status = mx_timer_get_last_measurement_time( autoscale->timer_record,
							&last_measurement_time);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: last_measurement_time = %g",
			fname, last_measurement_time));

	if ( last_measurement_time >= 0.0 ) {
		offset = mx_round( offset_per_second * last_measurement_time );
	} else {
		offset = 0L;
	}

	MX_DEBUG( 2,("%s: offset = %ld", fname, offset));

	scaler_value -= offset;

	MX_DEBUG( 2,("%s: new scaler_value = %ld", fname, scaler_value));

	autoscale->monitor_value = scaler_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_get_change_request( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_filter_amp_get_change_request()";

	MX_AUTO_FILTER_AMP *auto_filter_amp;
	unsigned long filter_setting;
	long last_scaler_value;
	double dynamic_low_limit, dynamic_high_limit, last_measurement_time;
	mx_status_type mx_status;

	mx_status = mxd_auto_filter_amp_get_pointers( autoscale,
						&auto_filter_amp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	last_scaler_value = autoscale->monitor_value;

	MX_DEBUG( 2,("%s: last_scaler_value = %ld", fname, last_scaler_value));

	mx_status = mx_digital_output_read( autoscale->control_record,
						&filter_setting );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	auto_filter_amp->present_filter_setting = filter_setting;

	mx_status = mx_autoscale_compute_dynamic_limits( autoscale->record,
				&dynamic_low_limit, &dynamic_high_limit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_timer_get_last_measurement_time( autoscale->timer_record,
							&last_measurement_time);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dynamic_low_limit *= last_measurement_time;
	dynamic_high_limit *= last_measurement_time;

	autoscale->get_change_request = MXF_AUTO_NO_CHANGE;

	if ( last_scaler_value > dynamic_high_limit ) {
		MX_DEBUG( 2,("%s: Go to THICKER filter", fname));

		if ( filter_setting >= auto_filter_amp->maximum_filter_setting )
		{
			MX_DEBUG( 2,("%s: Already at THICKEST filter.",
				fname));

			return MX_SUCCESSFUL_RESULT;
		}

		autoscale->get_change_request = MXF_AUTO_DECREASE_INTENSITY;

		autoscale->last_limit_tripped = MXF_AUTO_HIGH_LIMIT_TRIPPED;
	} else
	if ( last_scaler_value < dynamic_low_limit ) {
		MX_DEBUG( 2,("%s: Go to THINNER filter", fname));

		if ( filter_setting <= auto_filter_amp->minimum_filter_setting )
		{
			MX_DEBUG( 2,("%s: Already at THINNEST filter.",
				fname));

			return MX_SUCCESSFUL_RESULT;
		}

		autoscale->get_change_request = MXF_AUTO_INCREASE_INTENSITY;

		autoscale->last_limit_tripped = MXF_AUTO_LOW_LIMIT_TRIPPED;
	} else {
		MX_DEBUG( 2,("%s: NO CHANGE", fname));

		return MX_SUCCESSFUL_RESULT;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_change_control( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_filter_amp_change_control()";

	MX_AUTO_FILTER_AMP *auto_filter_amp;
	MX_RECORD *amplifier_autoscale_record;
	MX_AUTOSCALE *amplifier_autoscale;
	int i, busy;
	unsigned long old_filter_setting, new_filter_setting;
	long last_amplifier_scaler_value;
	double dynamic_low_limit, dynamic_high_limit;
	double old_gain, new_gain;
	mx_status_type mx_status;

	mx_status = mxd_auto_filter_amp_get_pointers( autoscale,
						&auto_filter_amp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	amplifier_autoscale_record =
			auto_filter_amp->amplifier_autoscale_record;

	if ( amplifier_autoscale_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The amplifier_autoscale_record pointer for record '%s' is NULL.",
			autoscale->record->name );
	}

	amplifier_autoscale = (MX_AUTOSCALE *)
			amplifier_autoscale_record->record_class_struct;

	if ( amplifier_autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AUTOSCALE pointer for record '%s' is NULL.",
			amplifier_autoscale_record->name );
	}

	old_filter_setting = auto_filter_amp->present_filter_setting;

	MX_DEBUG( 2,("%s: change_control = %d",
			fname, autoscale->change_control));

	switch( autoscale->change_control ) {
	case MXF_AUTO_NO_CHANGE:
		MX_DEBUG( 2,("%s: No change requested.", fname));

		return MX_SUCCESSFUL_RESULT;
		break;

	case MXF_AUTO_INCREASE_INTENSITY:
		if ( old_filter_setting <= 
				auto_filter_amp->minimum_filter_setting )
		{
			MX_DEBUG( 2,("%s: Already at MINIMUM filter setting.",
				fname));

			return MX_SUCCESSFUL_RESULT;
		}

		new_filter_setting = old_filter_setting - 1;
		break;

	case MXF_AUTO_DECREASE_INTENSITY:
		if ( old_filter_setting >= 
				auto_filter_amp->maximum_filter_setting )
		{
			MX_DEBUG( 2,("%s: Already at MAXIMUM filter setting.",
				fname));

			return MX_SUCCESSFUL_RESULT;
		}

		new_filter_setting = old_filter_setting + 1;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal value %d for change_control.",
			autoscale->change_control );
		break;
	}

	MX_DEBUG( 2,("%s: Changing filter '%s' from %#lx to %#lx.",
		fname, autoscale->control_record->name,
		old_filter_setting, new_filter_setting));

	mx_status = mx_digital_output_write( autoscale->control_record,
						new_filter_setting );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	auto_filter_amp->present_filter_setting = new_filter_setting;

	/*************************************************************
	 * Now that we have changed the filter settings, we need to  *
	 * change the amplifier gain to make sure that the amplifier *
	 * is also within its autoscale limits.                      *
	 *************************************************************/

	/* Read the present amplifier gain. */

	mx_status = mx_amplifier_get_gain( amplifier_autoscale->control_record,
						&old_gain );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: amplifier starting gain = %g", fname, old_gain));

	/* Repeatedly autoscale the amplifier setting until we either
	 * get the same value twice in a row or reach a limit on
	 * the amplifier gain.
	 */

	for ( i = 0; ; i++ ) {

		/* Perform a 1 second measurement. */

		MX_DEBUG( 2,("%s: Performing a 1 second measurement.", fname));

		mx_status = mx_timer_start(
				amplifier_autoscale->timer_record, 1.0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		busy = TRUE;

		while ( busy ) {

			if ( mx_user_requested_interrupt() ) {
				return mx_error( MXE_INTERRUPTED, fname,
			"The amplifier scaler measurement was interrupted." );
			}

			mx_status = mx_timer_is_busy(
				amplifier_autoscale->timer_record, &busy );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_msleep(1);	/* Wait at least 1 millisecond. */
		}

		/* Read the amplifier monitor channel. */

		mx_status = mx_autoscale_read_monitor(
					amplifier_autoscale_record,
					&last_amplifier_scaler_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: last_amplifier_scaler_value = %ld",
			fname, last_amplifier_scaler_value));

		/* Get the dynamic limits for the scaler value. */

		mx_status = mx_autoscale_compute_dynamic_limits(
					amplifier_autoscale_record,
					&dynamic_low_limit,
					&dynamic_high_limit );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: dynamic_low_limit = %g",
				fname, dynamic_low_limit));
		MX_DEBUG( 2,("%s: dynamic_high_limit = %g",
				fname, dynamic_high_limit));

		if ( autoscale->change_control == MXF_AUTO_NO_CHANGE ) {

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"autoscale->change_control has the value 0.  "
			"This should never happen and is a programming bug." );
		} else
		if ( autoscale->change_control == MXF_AUTO_INCREASE_INTENSITY )
		{
			/* We decreased the filter thickness earlier.  If
			 * last_amplifier_scaler_value is now below the
			 * dynamic high limit, then we are done with changes
			 * to the amplifier gain.
			 */

			if ( last_amplifier_scaler_value <= dynamic_high_limit )
			{
				MX_DEBUG( 2,
("%s: last scaler value <= dyn. high limit, so autoscaling is complete.",
					fname));

				break;		/* Exit the for() loop. */
			}
		} else
		if ( autoscale->change_control == MXF_AUTO_DECREASE_INTENSITY )
		{
			/* We increased the filter thickness earlier.  If
			 * last_amplifier_scaler_value is now above the
			 * dynamic low limit, then we are done with changes
			 * to the amplifier gain.
			 */

			if ( last_amplifier_scaler_value >= dynamic_low_limit )
			{

				MX_DEBUG( 2,
("%s: last scaler value >= dyn. low limit, so autoscaling is complete.",
					fname));

				break;		/* Exit the for() loop. */
			}
		} else {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal value %d for change_control.",
				autoscale->change_control );
		}

		/* Change the amplifier gain.
		 *
		 * If the filter was changed to increase the intensity,
		 * we change the amplifier to reduce the intensity.
		 *
		 * If the filter was changed to reduce the intensity,
		 * we change the amplifier to increase the intensity.
		 */

		mx_status = mx_autoscale_change_control(
					amplifier_autoscale_record,
					-(autoscale->change_control) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Read the new amplifier gain setting. */

		mx_status = mx_amplifier_get_gain(
			amplifier_autoscale->control_record, &new_gain );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: New gain setting = %g", fname, new_gain ));

		if ( mx_difference( new_gain, old_gain ) < 0.0001 ) {

			MX_DEBUG( 2,
		("%s: new gain == old gain, so autoscaling is complete.",
				fname));

			break;		/* Exit the for() loop. */
		}
		old_gain = new_gain;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_get_offset_index( MX_AUTOSCALE *autoscale )
{
	/* The offset index for the autoscale filter driver is always zero. */

	autoscale->monitor_offset_index = 0L;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_set_offset_index( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_filter_amp_set_offset_index()";

	unsigned long saved_index;

	/* The offset index for the autoscale filter driver is only
	 * allowed to be zero.
	 */

	saved_index = autoscale->monitor_offset_index;

	if ( saved_index != 0L ) {

		autoscale->monitor_offset_index = 0L;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested monitor offset index of %ld is not allowed.  "
		"Zero is the only allowed value.",
			saved_index );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_get_parameter( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_filter_amp_get_parameter()";

	MX_AUTO_FILTER_AMP *auto_filter_amp;
	mx_status_type mx_status;

	mx_status = mxd_auto_filter_amp_get_pointers( autoscale,
						&auto_filter_amp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,(
	"%s invoked for autoscale '%s' for parameter type '%s' (%d).",
		fname, autoscale->record->name,
		mx_get_field_label_string( autoscale->record,
			autoscale->parameter_type ),
		autoscale->parameter_type));

	switch( autoscale->parameter_type ) {
	default:
		mx_status = mx_autoscale_default_get_parameter_handler(
								autoscale );
		break;
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_auto_filter_amp_set_parameter( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_filter_amp_set_parameter()";

	MX_AUTO_FILTER_AMP *auto_filter_amp;
	mx_status_type mx_status;

	mx_status = mxd_auto_filter_amp_get_pointers( autoscale,
						&auto_filter_amp, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,(
	"%s invoked for autoscale '%s' for parameter type '%s' (%d).",
		fname, autoscale->record->name,
		mx_get_field_label_string( autoscale->record,
			autoscale->parameter_type ),
		autoscale->parameter_type));

	switch( autoscale->parameter_type ) {
	default:
		mx_status = mx_autoscale_default_set_parameter_handler(
								autoscale );
		break;
	}

	return mx_status;
}

