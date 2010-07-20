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

#define DEBUG_TIMING		TRUE

#define DEBUG_PAUSE_REQUEST	FALSE

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
	mxs_mcs_quick_scan_initialize_type,
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
#if DEBUG_TIMING
	static const char fname[] = "mxs_energy_mcs_quick_scan_move_to_start()";
#endif
	MX_ENERGY_MCS_QUICK_SCAN_EXTENSION *energy_mcs_quick_scan_extension;
	MX_RECORD *d_spacing_record, *theta_record;
	double d_spacing;
	double energy_start, energy_end;
	double theta_start, theta_end;
	double sin_theta_start, sin_theta_end;
	long number_of_points;
	double time_per_point, requested_scan_time;
	double requested_theta_distance, requested_theta_speed;
	double acceleration_distance, acceleration_time;

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

	MX_DEBUG(-2,("%s: d_spacing = %f", fname, d_spacing));

	/*** Convert the energy start and end into the theta start and end. ***/

	energy_start = quick_scan->start_position[0];
	energy_end   = quick_scan->end_position[0];

	MX_DEBUG(-2,("%s: energy start = %f, energy end = %f",
		fname, energy_start, energy_end));

	sin_theta_start = mx_divide_safely( MX_HC,
				( 2.0 * d_spacing * energy_start ) );

	theta_start = ( 180.0 / MX_PI ) * asin( sin_theta_start );

	sin_theta_end = mx_divide_safely( MX_HC,
				( 2.0 * d_spacing * energy_end ) );

	theta_end = ( 180.0 / MX_PI )  * asin( sin_theta_end );

	MX_DEBUG(-2,("%s: theta start = %f, theta end = %f",
		fname, theta_start, theta_end));

	number_of_points = quick_scan->requested_num_measurements;

	time_per_point = atof( scan->measurement_arguments );

	MX_DEBUG(-2,("%s: number_of_points = %ld, time_per_point = %f",
		fname, number_of_points, time_per_point));

	requested_theta_distance = fabs( theta_end - theta_start );

	requested_scan_time = (number_of_points - 1) * time_per_point;

	MX_DEBUG(-2,("%s: requested_theta_distance = %f, scan_time = %f",
		fname, requested_theta_distance, requested_scan_time));

	requested_theta_speed = mx_divide_safely( requested_theta_distance,
						requested_scan_time );

	MX_DEBUG(-2,("%s: requested_theta_speed = %f",
		fname, requested_theta_speed));

	/*** Get the acceleration parameters for this motor. ***/

	theta_record = energy_mcs_quick_scan_extension->theta_record;

	mx_status = mx_motor_get_acceleration_distance( theta_record,
						&acceleration_distance );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_acceleration_time( theta_record,
						&acceleration_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: acceleration_distance = %f, acceleration_time = %f",
		fname, acceleration_distance, acceleration_time));

	/*** Compute the real theta start and end for the move. ***/

	if ( theta_end < theta_start ) {
		real_theta_start = theta_start + acceleration_distance;
		real_theta_end   = theta_end - acceleration_distance;
	} else {
		real_theta_start = theta_start - acceleration_distance;
		real_theta_end   = theta_end + acceleration_distance;
	}

	MX_DEBUG(-2,("%s: real_theta_start = %f, real_theta_end = %f",
		fname, real_theta_start, real_theta_end));

	/*** Compute the real energy start and end for the move. ***/

	real_sin_theta_start = sin( real_theta_start * MX_PI / 180.0 );

	real_energy_start = mx_divide_safely( MX_HC,
				2.0 * d_spacing * real_sin_theta_start );

	real_sin_theta_end = sin( real_theta_end * MX_PI / 180.0 );

	real_energy_end = mx_divide_safely( MX_HC,
				2.0 * d_spacing * real_sin_theta_end );

	MX_DEBUG(-2,("%s: real_sin_theta_start = %f, real_sin_theta_end = %f",
		fname, real_sin_theta_start, real_sin_theta_end ));

	MX_DEBUG(-2,("%s: real_energy_start = %f, real_energy_end = %f",
		fname, real_energy_start, real_energy_end ));

	real_scan_time = requested_scan_time + 2.0 * acceleration_time;

	MX_DEBUG(-2,("%s: real_scan_time = %f", fname, real_scan_time));

	actual_num_measurements = 1 + mx_divide_safely( real_scan_time,
							time_per_point );

	MX_DEBUG(-2,("%s: actual_num_measurements = %ld",
		fname, actual_num_measurements));

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

/*------*/ MX_EXPORT mx_status_type
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
	double position, speed;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, record->name));

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

	/*** Verify that the 'theta' record exists and works. ***/

	theta_record = mx_get_record( record, "theta" );

	if ( theta_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"No record named 'theta' is found in the MX database." );
	}

	energy_mcs_quick_scan_extension->theta_record = theta_record;

	mx_status = mx_motor_get_position( theta_record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_speed( theta_record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: theta position = %f, speed = %f",
		fname, position, speed));

	/*** Verify that the 'd_spacing' record exists and works. ***/

	d_spacing_record = mx_get_record( record, "d_spacing" );

	if ( d_spacing_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"No record named 'd_spacing' is found in the MX database." );
	}

	energy_mcs_quick_scan_extension->d_spacing_record = d_spacing_record;

	mx_status = mx_get_double_variable( d_spacing_record,
				&(energy_mcs_quick_scan_extension->d_spacing) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: d_spacing = %f", fname,
				energy_mcs_quick_scan_extension->d_spacing));

	MX_DEBUG(-2,("%s complete for scan '%s'.", fname, record->name));

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

#if 0 && DEBUG_TIMING
	MX_HRT_TIMING timing_measurement;
#endif

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	mx_status = mxs_mcs_quick_scan_prepare_for_scan_start( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s complete.", fname));

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

#if 0 && DEBUG_TIMING
	MX_HRT_TIMING timing_measurement;
#endif

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	mx_status = mxs_mcs_quick_scan_execute_scan_body( scan );

	MX_DEBUG(-2,("%s complete.", fname));

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

#if 0 && DEBUG_TIMING
	MX_HRT_TIMING timing_measurement;
#endif

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy_mcs_quick_scan_extension = mcs_quick_scan->extension_ptr;

	mx_status = mxs_mcs_quick_scan_cleanup_after_scan_end( scan );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

