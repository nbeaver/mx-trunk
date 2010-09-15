/*
 * Name:    sq_energy_mcs.c
 *
 * Purpose: Driver for quick scans of the energy pseudomotor that use
 *          an MX MCS record to collect the data.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_TIMING		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_constants.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_variable.h"
#include "mx_hrt_debug.h"
#include "mx_mcs.h"
#include "mx_timer.h"
#include "mx_pulse_generator.h"
#include "mx_scan.h"
#include "mx_scan_quick.h"
#include "sq_mcs.h"
#include "sq_energy_mcs.h"
#include "m_time.h"
#include "m_pulse_period.h"
#include "d_mcs_mce.h"
#include "d_mcs_scaler.h"
#include "d_mcs_timer.h"

MX_RECORD_FUNCTION_LIST mxs_energy_mcs_quick_scan_record_function_list = {
	mx_quick_scan_initialize_driver,
	mxs_energy_mcs_quick_scan_create_record_structures,
	mxs_energy_mcs_quick_scan_finish_record_initialization,
	mxs_mcs_quick_scan_delete_record,
	mx_quick_scan_print_scan_structure
};

MX_SCAN_FUNCTION_LIST mxs_energy_mcs_quick_scan_scan_function_list = {
	mxs_energy_mcs_quick_scan_prepare_for_scan_start,
	mxs_energy_mcs_quick_scan_execute_scan_body,
	mxs_energy_mcs_quick_scan_cleanup_after_scan_end
};

MX_RECORD_FIELD_DEFAULTS mxs_energy_mcs_quick_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_QUICK_SCAN_STANDARD_FIELDS
};

long mxs_energy_mcs_quick_scan_num_record_fields
			= sizeof( mxs_energy_mcs_quick_scan_defaults )
			/ sizeof( mxs_energy_mcs_quick_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_energy_mcs_quick_scan_def_ptr
			= &mxs_energy_mcs_quick_scan_defaults[0];

/*------*/

static mx_status_type
mxs_energy_mcs_quick_scan_move_to_start( MX_SCAN *scan,
				MX_QUICK_SCAN *quick_scan,
				MX_MCS_QUICK_SCAN *mcs_quick_scan,
				double measurement_time,
				mx_bool_type correct_for_quick_scan_backlash )
{
	static const char fname[] = "mxs_energy_mcs_quick_scan_move_to_start()";

	MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *energy_mcs_quick_scan_extension;
	MX_RECORD *d_spacing_record, *theta_record;
	MX_MOTOR *theta_motor;
	double d_spacing;
	double energy_start, energy_end;
	double theta_start, theta_end;
	double sin_theta_start, sin_theta_end;
	long number_of_points;
	double time_per_point, requested_scan_time;

	double requested_theta_distance, requested_theta_speed;
	double acceleration_distance, acceleration_time;

	double original_raw_speed, requested_raw_speed;
	double original_acceleration_time;
	double raw_acceleration, raw_acceleration_distance;
	double raw_base_speed;

	double real_theta_start, real_theta_end;
	double real_sin_theta_start, real_sin_theta_end;
	double real_energy_start, real_energy_end;
	double real_scan_time;
	long actual_num_measurements;
	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name));
#endif

	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	/*** Update the current value of the d-spacing. ***/

	d_spacing_record = energy_mcs_quick_scan_extension->d_spacing_record;

	mx_status = mx_get_double_variable( d_spacing_record, &d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy_mcs_quick_scan_extension->d_spacing = d_spacing;


	/*** Convert the energy start and end into the theta start and end. ***/

	energy_start = quick_scan->start_position[0];
	energy_end   = quick_scan->end_position[0];


	sin_theta_start = mx_divide_safely( MX_HC,
				( 2.0 * d_spacing * energy_start ) );

	theta_start = ( 180.0 / MX_PI ) * asin( sin_theta_start );

	sin_theta_end = mx_divide_safely( MX_HC,
				( 2.0 * d_spacing * energy_end ) );

	theta_end = ( 180.0 / MX_PI )  * asin( sin_theta_end );

	number_of_points = quick_scan->requested_num_measurements;

	time_per_point = atof( scan->measurement_arguments );

	requested_theta_distance = fabs( theta_end - theta_start );

	requested_scan_time = (number_of_points - 1) * time_per_point;

	requested_theta_speed = mx_divide_safely( requested_theta_distance,
						requested_scan_time );

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s: d_spacing = %f", fname, d_spacing));
	MX_DEBUG(-2,("%s: energy start = %f, energy end = %f",
		fname, energy_start, energy_end));
	MX_DEBUG(-2,("%s: theta start = %f, theta end = %f",
		fname, theta_start, theta_end));
	MX_DEBUG(-2,("%s: number_of_points = %ld, time_per_point = %f",
		fname, number_of_points, time_per_point));
	MX_DEBUG(-2,("%s: requested_theta_distance = %f, scan_time = %f",
		fname, requested_theta_distance, requested_scan_time));
	MX_DEBUG(-2,("%s: requested_theta_speed = %f",
		fname, requested_theta_speed));
#endif

	/*** Get the acceleration parameters for this motor. ***/

	theta_record = energy_mcs_quick_scan_extension->theta_record;

	if ( theta_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The theta_record pointer for energy quick scan '%s' is NULL.",
			scan->record->name );
	}

	theta_motor = (MX_MOTOR *) theta_record->record_class_struct;

	if ( theta_motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for theta record '%s' used by "
		"energy quick scan '%s' is NULL.",
			theta_record->name, scan->record->name );
	}

	mx_status =
	    mx_motor_update_speed_and_acceleration_parameters( theta_motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	original_raw_speed = theta_motor->raw_speed;

	original_acceleration_time = -1;

	requested_raw_speed = mx_divide_safely( requested_theta_speed,
						fabs(theta_motor->scale) );

	raw_base_speed = theta_motor->raw_base_speed;

	switch( theta_motor->acceleration_type ) {
	case MXF_MTR_ACCEL_RATE:
		raw_acceleration = theta_motor->raw_acceleration_parameters[0];
		break;
	case MXF_MTR_ACCEL_TIME:
		original_acceleration_time =
				theta_motor->raw_acceleration_parameters[0];

		raw_acceleration =
			mx_divide_safely( original_raw_speed - raw_base_speed,
						original_acceleration_time );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Acceleration type %ld for motor '%s' is not supported "
		"for energy quick scan '%s'.",
			theta_motor->acceleration_type,
			theta_record->name,
			scan->record->name );
	}

	acceleration_time =
		mx_divide_safely( requested_raw_speed - raw_base_speed,
					raw_acceleration );

	raw_acceleration_distance = 
	    + 0.5 * raw_acceleration * acceleration_time * acceleration_time;

	acceleration_distance =
		fabs( theta_motor->scale * raw_acceleration_distance );

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s: original_raw_speed = %f, requested_raw_speed = %f",
		fname, original_raw_speed, requested_raw_speed));

	MX_DEBUG(-2,("%s: raw_base_speed = %f", fname, raw_base_speed));

	MX_DEBUG(-2,
	("%s: raw_acceleration = %f, original_acceleration_time = %f",
		fname, raw_acceleration, original_acceleration_time));

	MX_DEBUG(-2,("%s: acceleration_distance = %f, acceleration_time = %f",
		fname, acceleration_distance, acceleration_time));
#endif

	/*** Compute the real theta start and end for the move. ***/

	if ( theta_end < theta_start ) {
		real_theta_start = theta_start + acceleration_distance;
		real_theta_end   = theta_end - acceleration_distance;
	} else {
		real_theta_start = theta_start - acceleration_distance;
		real_theta_end   = theta_end + acceleration_distance;
	}

	/*** Compute the real energy start and end for the move. ***/

	real_sin_theta_start = sin( real_theta_start * MX_PI / 180.0 );

	real_energy_start = mx_divide_safely( MX_HC,
				2.0 * d_spacing * real_sin_theta_start );

	real_sin_theta_end = sin( real_theta_end * MX_PI / 180.0 );

	real_energy_end = mx_divide_safely( MX_HC,
				2.0 * d_spacing * real_sin_theta_end );

	real_scan_time = requested_scan_time + 2.0 * acceleration_time;

	actual_num_measurements = 1 + mx_divide_safely( real_scan_time,
							time_per_point );

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s: real_theta_start = %f, real_theta_end = %f",
		fname, real_theta_start, real_theta_end));
	MX_DEBUG(-2,("%s: real_sin_theta_start = %f, real_sin_theta_end = %f",
		fname, real_sin_theta_start, real_sin_theta_end ));
	MX_DEBUG(-2,("%s: real_energy_start = %f, real_energy_end = %f",
		fname, real_energy_start, real_energy_end ));
	MX_DEBUG(-2,("%s: real_scan_time = %f", fname, real_scan_time));
	MX_DEBUG(-2,("%s: actual_num_measurements = %ld",
		fname, actual_num_measurements));
#endif

	/*** Copy the necessary parameters into the MX scan structures. ***/

	quick_scan->actual_num_measurements = actual_num_measurements;

	mcs_quick_scan->premove_measurement_time = 0.0;
	mcs_quick_scan->acceleration_time = acceleration_time;
	mcs_quick_scan->scan_body_time = requested_scan_time;
	mcs_quick_scan->deceleration_time = acceleration_time;
	mcs_quick_scan->postmove_measurement_time = 0.0;

	mcs_quick_scan->real_start_position[0] = real_energy_start;
	mcs_quick_scan->real_end_position[0] = real_energy_end;

	/*** Move to the start position. ***/

	mx_info( "Moving to the 'energy' start position at %f",
		mcs_quick_scan->real_start_position[0] );

	mx_status = mxs_mcs_quick_scan_move_absolute_and_wait( scan,
					mcs_quick_scan->real_start_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Save the theta speed and set the new speed. ***/

	mx_status = mx_motor_save_speed( theta_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_set_speed( theta_record, requested_theta_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info( "Ready at the start position." );

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s complete for scan '%s'.", fname, scan->record->name));
#endif

	return mx_status;
}

/*------*/

static mx_status_type
mxs_energy_mcs_quick_scan_compute_motor_positions( MX_SCAN *scan,
					MX_QUICK_SCAN *quick_scan,
					MX_MCS_QUICK_SCAN *mcs_quick_scan,
					MX_MCS *mcs )
{
	static const char fname[] =
		"mxs_energy_mcs_quick_scan_compute_motor_positions()";

	MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *energy_mcs_quick_scan_extension;
	MX_RECORD *theta_record, *mce_record;
	MX_MOTOR *theta_motor;
	unsigned long num_encoder_values;
	double *encoder_value_array;
	double *energy_position_array;
	double energy_start, theta_start, sin_theta_start;
	double start_of_bin_value, scaled_encoder_value;
	double theta, sin_theta, energy, d_spacing;
	long i;
	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name));
#endif
	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	theta_record = energy_mcs_quick_scan_extension->theta_record;

	theta_motor = (MX_MOTOR *) theta_record->record_class_struct;

	mce_record = mxs_mcs_quick_scan_find_encoder_readout( theta_record );

	if ( mce_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"No multichannel encoder (MCE) record was found in "
		"the MX database for theta motor '%s'.",
			theta_record->name );
	}

	if ( mcs_quick_scan->motor_position_array == (double **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The motor_position_array pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	energy_position_array = mcs_quick_scan->motor_position_array[0];

	if ( energy_position_array == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The motor_position_array[0] pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	energy_start = mcs_quick_scan->real_start_position[0];

	d_spacing = energy_mcs_quick_scan_extension->d_spacing;

	sin_theta_start =  mx_divide_safely( MX_HC,
				2.0 * d_spacing * energy_start );

	theta_start = asin( sin_theta_start ) * 180.0 / MX_PI;

	/* Read in the multichannel encoder values. */

	mx_status = mx_mce_read( mce_record,
			&num_encoder_values, &encoder_value_array );

	if ( mx_status.code != MXE_SUCCESS ) {

		/* An error occurred while reading the encoder
		 * value array, so fill the position array with -1.
		 */

		for ( i = 0; i < quick_scan->actual_num_measurements; i++ ) {
			energy_position_array[i] = -1.0;
		}
	} else {
		if ( num_encoder_values > quick_scan->actual_num_measurements )
		{
		    num_encoder_values = quick_scan->actual_num_measurements;
		}

		start_of_bin_value = theta_start;

		for ( i = 0; i < num_encoder_values; i++ ) {
			scaled_encoder_value = theta_motor->scale
						* encoder_value_array[i];

			/* The scaled motor value reflects the distance
			 * that the motor has moved during this bin,
			 * so we should put the reported position
			 * in the middle of the bin.
			 */

			theta = start_of_bin_value + 0.5 * scaled_encoder_value;

			start_of_bin_value += scaled_encoder_value;

			sin_theta = sin( theta * MX_PI / 180.0 );

			energy = mx_divide_safely( MX_HC,
					2.0 * d_spacing * sin_theta );

			energy_position_array[i] = energy;
		}

		/* Fill in the rest of the array (if any) with 0. */

		for ( ; i < quick_scan->actual_num_measurements; i++ ) {
			energy_position_array[i] = 0.0;
		}
	}
			

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s complete for scan '%s'.", fname, scan->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxs_energy_mcs_quick_scan_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_energy_mcs_quick_scan_create_record_structures()";

	MX_SCAN *scan;
	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	mx_status_type mx_status;

	mx_status = mxs_mcs_quick_scan_create_record_structures( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scan = (MX_SCAN *) record->record_superclass_struct;

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Modify the superclass specific function list to point to
	 * custom versions of the scanning functions.
	 */

	record->superclass_specific_function_list
				= &mxs_energy_mcs_quick_scan_scan_function_list;

	/* Allocate memory for the energy quick scan specific extension. */

	mcs_quick_scan->extension_ptr = (MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *)
		malloc( sizeof(MX_ENERGY_MCS_QUICK_SCAN_EXTENSION) );

	if ( mcs_quick_scan->extension_ptr ==
		(MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_ENERGY_MCS_QUICK_SCAN_EXTENSION structure." );
	}

	/* Replace the move_to_start function. */

	mcs_quick_scan->move_to_start_fn =
			mxs_energy_mcs_quick_scan_move_to_start;

	mcs_quick_scan->compute_motor_positions_fn =
			mxs_energy_mcs_quick_scan_compute_motor_positions;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_energy_mcs_quick_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_energy_mcs_quick_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *energy_mcs_quick_scan_extension;
	MX_RECORD *d_spacing_record;
	MX_RECORD *theta_record;
	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, record->name));
#endif

	scan = (MX_SCAN *) record->record_superclass_struct;

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	if ( energy_mcs_quick_scan_extension == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ENERGY_MCS_QUICK_SCAN_EXTENSION pointer for "
		"scan '%s' is NULL.", record->name );
	}

	mx_status = mxs_mcs_quick_scan_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*************************************************************
	 *** Verify that the 'theta_real' record exists and works. ***
	 *** If not found, use the 'theta' record instead.         ***
	 *************************************************************/

	theta_record = mx_get_record( record, "theta_real" );

	if ( theta_record == (MX_RECORD *) NULL ) {
		theta_record = mx_get_record( record, "theta" );

		if ( theta_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"No record named 'theta_real' or 'theta' "
			"is found in the MX database.");
		}
	}

	energy_mcs_quick_scan_extension->theta_record = theta_record;

	/*** Verify that the 'd_spacing' record exists and works. ***/

	d_spacing_record = mx_get_record( record, "d_spacing" );

	if ( d_spacing_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"No record named 'd_spacing' is found in the MX database." );
	}

	energy_mcs_quick_scan_extension->d_spacing_record = d_spacing_record;

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s complete for scan '%s'.", fname, record->name));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_energy_mcs_quick_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_energy_mcs_quick_scan_prepare_for_scan_start()";

	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *energy_mcs_quick_scan_extension;
	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	mx_status = mxs_mcs_quick_scan_prepare_for_scan_start( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_energy_mcs_quick_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_energy_mcs_quick_scan_execute_scan_body()";

	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *energy_mcs_quick_scan_extension;
	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	mx_status = mxs_mcs_quick_scan_execute_scan_body( scan );

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* Please note that most error conditions are ignored in the
 * cleanup_after_scan_end() function since it is important that
 * the data be saved to disk in spite of any extraneous errors
 * that may occur.
 */

MX_EXPORT mx_status_type
mxs_energy_mcs_quick_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_energy_mcs_quick_scan_cleanup_after_scan_end()";

	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *energy_mcs_quick_scan_extension;
	mx_status_type mx_status;

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	mx_status = mxs_mcs_quick_scan_cleanup_after_scan_end( scan );

#if DEBUG_TIMING
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

