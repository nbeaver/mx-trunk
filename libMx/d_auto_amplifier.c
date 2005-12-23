/*
 * Name:    d_auto_amplifier.c
 *
 * Purpose: Driver for autoscaling of amplifier settings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_amplifier.h"
#include "mx_autoscale.h"
#include "d_auto_amplifier.h"

MX_RECORD_FUNCTION_LIST mxd_auto_amplifier_record_function_list = {
	mxd_auto_amplifier_initialize_type,
	mxd_auto_amplifier_create_record_structures,
	mxd_auto_amplifier_finish_record_initialization,
	mxd_auto_amplifier_delete_record,
	NULL,
	mxd_auto_amplifier_dummy_function,
	mxd_auto_amplifier_dummy_function,
	mxd_auto_amplifier_open,
	mxd_auto_amplifier_dummy_function
};

MX_AUTOSCALE_FUNCTION_LIST mxd_auto_amplifier_autoscale_function_list = {
	mxd_auto_amplifier_read_monitor,
	mxd_auto_amplifier_get_change_request,
	mxd_auto_amplifier_change_control,
	mxd_auto_amplifier_get_offset_index,
	mxd_auto_amplifier_set_offset_index,
	mxd_auto_amplifier_get_parameter,
	mxd_auto_amplifier_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_auto_amplifier_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AUTOSCALE_STANDARD_FIELDS,
	MX_AUTO_AMPLIFIER_STANDARD_FIELDS
};

long mxd_auto_amplifier_num_record_fields
	= sizeof( mxd_auto_amplifier_record_field_defaults )
	/ sizeof( mxd_auto_amplifier_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_auto_amplifier_rfield_def_ptr
		= &mxd_auto_amplifier_record_field_defaults[0];

static mx_status_type
mxd_auto_amplifier_get_pointers( MX_AUTOSCALE *autoscale,
			MX_AUTO_AMPLIFIER **auto_amplifier,
			const char *calling_fname )
{
	const char fname[] = "mxd_auto_amplifier_get_pointers()";

	if ( autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AUTOSCALE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( auto_amplifier == (MX_AUTO_AMPLIFIER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AUTO_AMPLIFIER pointer passed was NULL." );
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

	*auto_amplifier = (MX_AUTO_AMPLIFIER *)
				autoscale->record->record_type_struct;

	if ( *auto_amplifier == (MX_AUTO_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AUTO_AMPLIFIER pointer for record '%s' is NULL.",
				autoscale->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*****/

MX_EXPORT mx_status_type
mxd_auto_amplifier_initialize_type( long record_type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_amplifier_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_auto_amplifier_create_record_structures()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_AMPLIFIER *auto_amplifier;

	/* Allocate memory for the necessary structures. */

	autoscale = (MX_AUTOSCALE *) malloc( sizeof(MX_AUTOSCALE) );

	if ( autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AUTOSCALE structure." );
	}

	auto_amplifier = (MX_AUTO_AMPLIFIER *)
					malloc( sizeof(MX_AUTO_AMPLIFIER) );

	if ( auto_amplifier == (MX_AUTO_AMPLIFIER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AUTO_AMPLIFIER structure." );
	}

	/* Now set up the necessary pointers. */

	autoscale->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = autoscale;
	record->record_type_struct = auto_amplifier;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list =
			&mxd_auto_amplifier_autoscale_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_amplifier_finish_record_initialization( MX_RECORD *record )
{
	const char fname[]
		= "mxd_auto_amplifier_finish_record_initialization()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_AMPLIFIER *auto_amplifier;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	autoscale = (MX_AUTOSCALE *) record->record_class_struct;

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check that the pointers point to the correct kind of 
	 * data structures.
	 */

	if ( autoscale->monitor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The monitor_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( autoscale->monitor_record->mx_class != MXC_SCALER ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Amplifier monitor record '%s' is not a scaler record.",
			autoscale->monitor_record->name );
	}

	if ( autoscale->control_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The control_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( autoscale->control_record->mx_class != MXC_AMPLIFIER ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Amplifier control record '%s' is not an amplifier record.",
			autoscale->control_record->name );
	}

	if ( autoscale->timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The timer_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( autoscale->timer_record->mx_class != MXC_TIMER ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Amplifier timer record '%s' is not an timer record.",
			autoscale->timer_record->name );
	}

	/* Initialize record values. */

	autoscale->monitor_value = 0L;

	autoscale->num_monitor_offsets = 0L;
	autoscale->monitor_offset_array = NULL;

	autoscale->last_limit_tripped = MXF_AUTO_NO_LIMIT_TRIPPED;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_amplifier_delete_record( MX_RECORD *record )
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
mxd_auto_amplifier_open( MX_RECORD *record )
{
	const char fname[] = "mxd_auto_amplifier_open()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_AMPLIFIER *auto_amplifier;
	long i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	autoscale = (MX_AUTOSCALE *) record->record_class_struct;

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We cannot depend on being able to call mx_amplifier_get_gain_range()
	 * until the open routine, since for some types of amplifier records
	 * the connection may be made via some network or serial connection
	 * that isn't available until the other record's open routine is
	 * invoked.
	 */

	mx_status = mx_amplifier_get_gain_range( autoscale->control_record,
						auto_amplifier->gain_range );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Pick a legal default value for present_amplifier_gain. */

	auto_amplifier->present_amplifier_gain = auto_amplifier->gain_range[0];

	/* How many gains are there in the reported gain range? */

	/**************************************************************
	 * WARNING * This logic assumes that amplifier gains occur in *
	 * WARNING * geometrical steps of 10.  This is true for any   *
	 * WARNING * amplifier I currently know about.                *
	 **************************************************************/

	autoscale->num_monitor_offsets = mx_round(
				log10( auto_amplifier->gain_range[1] )
				- log10( auto_amplifier->gain_range[0] )
				+ 1.0 );

	/* Create an array to contain the monitor offsets. */

	mx_status = mx_autoscale_create_monitor_offset_array( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the monitor offset array to all zeros. */

	for ( i = 0; i < autoscale->num_monitor_offsets; i++ ) {
		autoscale->monitor_offset_array[i] = 0.0;
	}

	mx_status = mx_autoscale_read_monitor( record, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_auto_amplifier_dummy_function( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_amplifier_read_monitor( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_amplifier_read_monitor()";

	MX_AUTO_AMPLIFIER *auto_amplifier;
	long scaler_value, offset, offset_index;
	double offset_per_second, last_measurement_time;
	mx_status_type mx_status;

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_read( autoscale->monitor_record,
						&scaler_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: scaler_value = %ld", fname, scaler_value));
	MX_DEBUG( 2,("%s: present_amplifier_gain = %g",
			fname, auto_amplifier->present_amplifier_gain));

	/* Compute the amplifier gain-specific offset for the monitor value. */

	/**************************************************************
	 * WARNING * This logic assumes that amplifier gains occur in *
	 * WARNING * geometrical steps of 10.  This is true for any   *
	 * WARNING * amplifier I currently know about.                *
	 **************************************************************/

	offset_index = mx_round( log10( auto_amplifier->present_amplifier_gain )
				- log10( auto_amplifier->gain_range[0] ) );

	MX_DEBUG( 2,("%s: offset_index = %ld", fname, offset_index));

	if ( ( offset_index < 0 )
	  || ( offset_index >= autoscale->num_monitor_offsets ) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The offset_index for monitor_value_array has an "
		"illegal_value of %ld.  The allowed values are (0 - %ld)",
			offset_index, autoscale->num_monitor_offsets );
	}

	offset_per_second = autoscale->monitor_offset_array[offset_index];

	MX_DEBUG( 2,("%s: offset_per_second = %g", fname, offset_per_second));

	mx_status = mx_timer_get_last_measurement_time( autoscale->timer_record,
							&last_measurement_time);

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
mxd_auto_amplifier_get_change_request( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_amplifier_get_change_request()";

	MX_AUTO_AMPLIFIER *auto_amplifier;
	double gain;
	double dynamic_high_limit, dynamic_low_limit, last_measurement_time;
	long last_scaler_value;
	mx_status_type mx_status;

#if 0
	MX_CLOCK_TICK clock_ticks_before, clock_ticks_after, clock_ticks_diff;
	double seconds_diff;
#endif

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* WARNING: mx_amplifier_get_gain() may be a time consuming operation
	 * for a Keithley amplifier.  We need to verify just how long it
	 * takes to read the gain from a Keithley.
	 */

#if 0
	clock_ticks_before = mx_current_clock_tick();
#endif

	mx_status = mx_amplifier_get_gain( autoscale->control_record, &gain );

#if 0
	clock_ticks_after = mx_current_clock_tick();

	clock_ticks_diff = mx_subtract_clock_ticks( clock_ticks_after,
							clock_ticks_before );

	seconds_diff = mx_convert_clock_ticks_to_seconds( clock_ticks_diff );

	MX_DEBUG( 2,("%s: clock_ticks_before = (%lu, %ld)", fname,
			(unsigned long) clock_ticks_before.high_order,
			(long) clock_ticks_before.low_order));

	MX_DEBUG( 2,("%s: clock_ticks_after = (%lu, %ld)", fname,
			(unsigned long) clock_ticks_after.high_order,
			(long) clock_ticks_after.low_order));

	MX_DEBUG( 2,("%s: get_gain took %ld clock ticks ( %g seconds )",
		fname, (long) clock_ticks_diff.low_order, seconds_diff));
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	auto_amplifier->present_amplifier_gain = gain;

	last_scaler_value = autoscale->monitor_value;

	MX_DEBUG( 2,("%s: last_scaler_value = %ld", fname, last_scaler_value));

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

	MX_DEBUG( 2,("%s: after message.", fname));

	autoscale->get_change_request = MXF_AUTO_NO_CHANGE;

	if ( last_scaler_value > dynamic_high_limit ) {
		MX_DEBUG( 2,("%s: Go to LOWER gain", fname));

		if ( gain <= auto_amplifier->gain_range[0] ) {
			MX_DEBUG( 2,("%s: Already at LOWEST possible gain.",
				fname));

			return MX_SUCCESSFUL_RESULT;
		}

		autoscale->get_change_request = MXF_AUTO_DECREASE_INTENSITY;

		autoscale->last_limit_tripped = MXF_AUTO_HIGH_LIMIT_TRIPPED;
	} else
	if ( last_scaler_value < dynamic_low_limit ) {
		MX_DEBUG( 2,("%s: Go to HIGHER gain", fname));

		if ( gain >= auto_amplifier->gain_range[1] ) {
			MX_DEBUG( 2,("%s: Already at HIGHEST possible gain.",
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
mxd_auto_amplifier_change_control( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_amplifier_change_control()";

	MX_AUTO_AMPLIFIER *auto_amplifier;
	double old_gain, new_gain;
	mx_status_type mx_status;

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	old_gain = auto_amplifier->present_amplifier_gain;

	MX_DEBUG( 2,("%s: change_control = %d",
			fname, autoscale->change_control));

	switch( autoscale->change_control ) {
	case MXF_AUTO_NO_CHANGE:
		MX_DEBUG( 2,("%s: No change requested.", fname));

		return MX_SUCCESSFUL_RESULT;
		break;

	case MXF_AUTO_INCREASE_INTENSITY:
		if ( old_gain >= auto_amplifier->gain_range[1] ) {

			MX_DEBUG( 2,("%s: Already at HIGHEST possible gain.",
				fname));

			return MX_SUCCESSFUL_RESULT;
		}

		new_gain = old_gain * 10.0;
		break;

	case MXF_AUTO_DECREASE_INTENSITY:
		if ( old_gain <= auto_amplifier->gain_range[0] ) {

			MX_DEBUG( 2,("%s: Already at LOWEST possible gain.",
				fname));

			return MX_SUCCESSFUL_RESULT;
		}

		new_gain = old_gain * 0.1;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal value %d for change control.",
			autoscale->change_control );
		break;
	}

	MX_DEBUG( 2,("%s: Changing gain from %g to %g",
			fname, old_gain, new_gain));

	mx_status = mx_amplifier_set_gain(autoscale->control_record, new_gain);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	auto_amplifier->present_amplifier_gain = new_gain;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_amplifier_get_offset_index( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_amplifier_get_offset_index()";

	MX_AUTO_AMPLIFIER *auto_amplifier;
	double gain;
	long offset_index;
	mx_status_type mx_status;

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_get_gain( autoscale->control_record, &gain );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	auto_amplifier->present_amplifier_gain = gain;

	offset_index = mx_round( log10( auto_amplifier->present_amplifier_gain )
				- log10( auto_amplifier->gain_range[0] ) );

	if ( ( offset_index < 0 )
	  || ( offset_index >= autoscale->num_monitor_offsets ) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The offset_index for monitor_value_array has an "
		"illegal_value of %ld.  The allowed values are (0 - %ld)",
			offset_index, autoscale->num_monitor_offsets );
	}

	autoscale->monitor_offset_index = offset_index;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_amplifier_set_offset_index( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_amplifier_set_offset_index()";

	MX_AUTO_AMPLIFIER *auto_amplifier;
	double gain;
	unsigned long offset_index;
	mx_status_type mx_status;

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	offset_index = autoscale->monitor_offset_index;

	if ( offset_index >= autoscale->num_monitor_offsets ) {

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested offset_index has an "
		"illegal_value of %ld.  The allowed values are (0 - %ld)",
			offset_index, autoscale->num_monitor_offsets );
	}

	gain = auto_amplifier->gain_range[0] * pow(10.0, (double) offset_index);

	mx_status = mx_amplifier_set_gain( autoscale->control_record, gain );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	auto_amplifier->present_amplifier_gain = gain;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_amplifier_get_parameter( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_amplifier_get_parameter()";

	MX_AUTO_AMPLIFIER *auto_amplifier;
	mx_status_type mx_status;

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

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
mxd_auto_amplifier_set_parameter( MX_AUTOSCALE *autoscale )
{
	const char fname[] = "mxd_auto_amplifier_set_parameter()";

	MX_AUTO_AMPLIFIER *auto_amplifier;
	mx_status_type mx_status;

	mx_status = mxd_auto_amplifier_get_pointers( autoscale,
						&auto_amplifier, fname );

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

