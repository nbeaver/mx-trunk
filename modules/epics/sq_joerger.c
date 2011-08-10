/*
 * Name:    sq_joerger.c
 *
 * Purpose: Driver for quick scans that use Joerger scalers via EPICS.
 * 
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2009-2011 Illinois Institute of Technology
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
#include "mx_array.h"
#include "mx_variable.h"
#include "mx_epics.h"
#include "mx_scan.h"
#include "mx_scan_quick.h"
#include "sq_joerger.h"
#include "m_time.h"
#include "d_epics_motor.h"
#include "d_epics_scaler.h"
#include "d_epics_timer.h"

MX_RECORD_FUNCTION_LIST mxs_joerger_quick_scan_record_function_list = {
	mx_quick_scan_initialize_driver,
	mxs_joerger_quick_scan_create_record_structures,
	mxs_joerger_quick_scan_finish_record_initialization,
	mxs_joerger_quick_scan_delete_record,
	mx_quick_scan_print_scan_structure
};

MX_SCAN_FUNCTION_LIST mxs_joerger_quick_scan_scan_function_list = {
	mxs_joerger_quick_scan_prepare_for_scan_start,
	mxs_joerger_quick_scan_execute_scan_body,
	mxs_joerger_quick_scan_cleanup_after_scan_end
};

MX_RECORD_FIELD_DEFAULTS mxs_joerger_quick_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_QUICK_SCAN_STANDARD_FIELDS
};

long mxs_joerger_quick_scan_num_record_fields
			= sizeof( mxs_joerger_quick_scan_defaults )
			/ sizeof( mxs_joerger_quick_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_joerger_quick_scan_def_ptr
			= &mxs_joerger_quick_scan_defaults[0];

#define EPICS_NAME_LENGTH	41

#define FREE_ARRAYS mxs_joerger_quick_scan_free_arrays(scan,joerger_quick_scan)

static void
mxs_joerger_quick_scan_free_arrays( MX_SCAN *scan,
				MX_JOERGER_QUICK_SCAN *joerger_quick_scan )
{
	long i;

	if ( scan == NULL )
		return;

	if ( joerger_quick_scan == NULL )
		return;

	if ( joerger_quick_scan->motor_position_array != NULL ) {
	    for ( i = 0; i < scan->num_motors; i++ ) {
		if ( joerger_quick_scan->motor_position_array[i] != NULL ) {
		    mx_free( joerger_quick_scan->motor_position_array[i] );

		    joerger_quick_scan->motor_position_array[i] = NULL;
		}
	    }
	}
	if ( joerger_quick_scan->data_array != NULL ) {
	    for ( i = 0; i < scan->num_input_devices; i++ ) {
		if ( joerger_quick_scan->data_array[i] != NULL ) {
		    mx_free( joerger_quick_scan->data_array[i] );

		    joerger_quick_scan->data_array[i] = NULL;
		}
	    }
	}
}

static mx_status_type
mxs_joerger_quick_scan_get_pointers( MX_SCAN *scan,
				MX_QUICK_SCAN **quick_scan,
				MX_JOERGER_QUICK_SCAN **joerger_quick_scan,
				const char *calling_fname )
{
	const char fname[] = "mxs_joerger_quick_scan_get_pointers()";

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCAN pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( quick_scan == (MX_QUICK_SCAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_QUICK_SCAN pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( joerger_quick_scan == (MX_JOERGER_QUICK_SCAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_JOERGER_QUICK_SCAN pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*quick_scan = (MX_QUICK_SCAN *) (scan->record->record_class_struct);

	if ( *quick_scan == (MX_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_QUICK_SCAN pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	*joerger_quick_scan = (MX_JOERGER_QUICK_SCAN *)
					scan->record->record_type_struct;

	if ( *joerger_quick_scan == (MX_JOERGER_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_JOERGER_QUICK_SCAN pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_joerger_quick_scan_create_record_structures( MX_RECORD *record )
{
	const char fname[] =
		"mxs_joerger_quick_scan_create_record_structures()";

	MX_SCAN *scan;
	MX_QUICK_SCAN *quick_scan;
	MX_JOERGER_QUICK_SCAN *joerger_quick_scan;
	int i;

	/* Allocate memory for the necessary structures. */

	scan = (MX_SCAN *) malloc( sizeof(MX_SCAN) );

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	quick_scan = (MX_QUICK_SCAN *) malloc( sizeof(MX_QUICK_SCAN) );

	if ( quick_scan == (MX_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_QUICK_SCAN structure." );
	}

	joerger_quick_scan = (MX_JOERGER_QUICK_SCAN *)
				malloc( sizeof(MX_JOERGER_QUICK_SCAN) );

	if ( joerger_quick_scan == (MX_JOERGER_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_JOERGER_QUICK_SCAN structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_superclass_struct = scan;
	record->record_class_struct = quick_scan;
	record->record_type_struct = joerger_quick_scan;

	scan->record = record;

	scan->measurement.scan = scan;
	scan->datafile.scan = scan;
	scan->plot.scan = scan;

	scan->datafile.x_motor_array = NULL;
	scan->plot.x_motor_array = NULL;

	quick_scan->scan = scan;

	record->superclass_specific_function_list
				= &mxs_joerger_quick_scan_scan_function_list;

	record->class_specific_function_list = NULL;

	scan->num_missing_records = 0;
	scan->missing_record_array = NULL;

	for ( i = 0; i < MXS_JQ_MAX_MOTORS; i++ ) {
		joerger_quick_scan->motor_position_array[i] = NULL;
	}
	for ( i = 0; i < MXS_JQ_MAX_JOERGER_SCALERS; i++ ) {
		joerger_quick_scan->data_array[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_joerger_quick_scan_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] =
		"mxs_joerger_quick_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_QUICK_SCAN *quick_scan;
	MX_JOERGER_QUICK_SCAN *joerger_quick_scan;
	long i;
	mx_status_type mx_status;

	scan = (MX_SCAN *) record->record_superclass_struct;

	mx_status = mxs_joerger_quick_scan_get_pointers( scan,
			&quick_scan, &joerger_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( scan->num_motors < 0 )
	  || ( scan->num_motors > MXS_JQ_MAX_MOTORS ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal num_motors value %ld for scan '%s'.  "
		"The allowed range is (0-%d)",
			scan->num_motors, record->name,
			MXS_JQ_MAX_MOTORS );
	}

	if ( ( scan->num_input_devices < 0 )
	  || ( scan->num_input_devices > MXS_JQ_MAX_JOERGER_SCALERS ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal num_input_devices value %ld for scan '%s'.  "
		"The allowed range is (0-%d)",
			scan->num_input_devices, record->name,
			MXS_JQ_MAX_JOERGER_SCALERS );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;
	}

	/* For the Joerger quick scan, we do not have pre-move and post-move
	 * measurements, so the requested and actual number of measurements
	 * are the same.
	 */

	quick_scan->actual_num_measurements
			= quick_scan->requested_num_measurements;

	return mx_scan_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxs_joerger_quick_scan_delete_record( MX_RECORD *record )
{
	MX_SCAN *scan;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		mx_free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		mx_free( record->record_class_struct );

		record->record_class_struct = NULL;
	}

	scan = (MX_SCAN *) record->record_superclass_struct;

	if ( scan != NULL ) {
		if ( scan->missing_record_array != NULL ) {
			int i;

			for ( i = 0; i < scan->num_missing_records; i++ ) {
				mx_delete_record(
				    scan->missing_record_array[i] );
			}

			mx_free( scan->missing_record_array );
		}

		if ( scan->datafile.x_motor_array != NULL ) {
			mx_free( scan->datafile.x_motor_array );
		}

		if ( scan->plot.x_motor_array != NULL ) {
			mx_free( scan->plot.x_motor_array );
		}

		mx_free( scan );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxs_joerger_quick_scan_compute_scan_parameters(
				MX_SCAN *scan,
				MX_QUICK_SCAN *quick_scan,
				MX_JOERGER_QUICK_SCAN *joerger_quick_scan )
{
	const char fname[] = "mxs_joerger_quick_scan_compute_scan_parameters()";

	MX_RECORD *motor_record;
	double acceleration_time, longest_acceleration_time;
	double start_position, end_position;
	double real_start_position, real_end_position;
	long i;
	mx_status_type mx_status;

	/* Find out what the longest acceleration time is. */

	longest_acceleration_time = 0.0;

	for ( i = 0; i < scan->num_motors; i++ ) {

		motor_record = (scan->motor_record_array)[i];

		mx_status = mx_motor_get_acceleration_time( motor_record,
							&acceleration_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( acceleration_time > longest_acceleration_time ) {
			longest_acceleration_time = acceleration_time;
		}
	}

	joerger_quick_scan->acceleration_time = longest_acceleration_time;

	MX_DEBUG( 2,("%s: joerger_quick_scan->acceleration_time = %g",
		fname, joerger_quick_scan->acceleration_time));

	/* Extend the estimated scan duration time by twice the
	 * acceleration time.
	 */

	quick_scan->estimated_scan_duration
			+= ( 2.0 * joerger_quick_scan->acceleration_time );

	quick_scan->actual_num_measurements
		= quick_scan->requested_num_measurements;

	/* Compute the real start and end positions for the motors. */

	for ( i = 0; i < scan->num_motors; i++ ) {

		motor_record = (scan->motor_record_array)[i];

		start_position = (quick_scan->start_position)[i];
		end_position   = (quick_scan->end_position)[i];

		mx_status = mx_motor_compute_extended_scan_range(
				motor_record, start_position, end_position,
				&real_start_position, &real_end_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		joerger_quick_scan->real_start_position[i]
						= real_start_position;

		joerger_quick_scan->real_end_position[i]
						= real_end_position;

		MX_DEBUG( 2,("%s: motor '%s' real_start_position = %g",
			fname, motor_record->name,
			(joerger_quick_scan->real_start_position)[i] ));

		MX_DEBUG( 2,("%s: motor '%s' real_end_position = %g",
			fname, motor_record->name,
			(joerger_quick_scan->real_end_position)[i] ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_joerger_quick_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	const char fname[] = "mxs_joerger_quick_scan_prepare_for_scan_start()";

	MX_RECORD *motor_record;
	MX_RECORD *input_device_record;
	MX_RECORD *timer_record;
	MX_RECORD *synchronous_motion_mode_record;
	MX_RECORD *joerger_quick_scan_enable_record;
	MX_QUICK_SCAN *quick_scan;
	MX_JOERGER_QUICK_SCAN *joerger_quick_scan;
	MX_MOTOR *motor;
	MX_EPICS_MOTOR *epics_motor;
	MX_EPICS_SCALER *epics_scaler;
	MX_EPICS_TIMER *epics_timer;
	char pvname[MXU_EPICS_PVNAME_LENGTH+1];
	mx_bool_type joerger_quick_scan_enabled;
	long i, j, allowed_device;
	int32_t timer_preset;
	double scan_duration_seconds, scan_duration_ticks;
	double epics_update_rate, measurement_time_per_point;
	double update_intervals_per_measurement;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxs_joerger_quick_scan_get_pointers( scan,
			&quick_scan, &joerger_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer_record = NULL;

	if ( scan->num_input_devices <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"There must always be at least one input device for "
			"a Joerger quick scan, namely, the timer channel.  "
			"The scan '%s' has %ld input devices.",
				scan->record->name, scan->num_input_devices );
	}

	/* Are Joerger quick scans permitted for this beamline? */

	joerger_quick_scan_enable_record = mx_get_record( scan->record,
					MX_JOERGER_QUICK_SCAN_ENABLE_RECORD );

	if ( joerger_quick_scan_enable_record == (MX_RECORD *) NULL ) {
		joerger_quick_scan_enabled = FALSE;
	} else {
		mx_status = mx_get_bool_variable(
				joerger_quick_scan_enable_record,
				&joerger_quick_scan_enabled );
	}

	if ( joerger_quick_scan_enabled == FALSE ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"Joerger quick scans are NOT AVAILABLE anymore for this beamline.  "
	"Please do not try to use them." );
	}

	/* Verify that all of the input devices for this scan are
	 * Joerger scaler channels.  We do this by verifying that
	 * the MX scaler record are of type MXT_SCL_EPICS.
	 *
	 * We also record the input channel numbers as we go along.
	 */

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		input_device_record = ( scan->input_device_array )[i];

		allowed_device = FALSE;

		if ( input_device_record->mx_superclass != MXR_DEVICE ) {
			allowed_device = FALSE;
		} else {
			switch( input_device_record->mx_class ) {
			case MXC_SCALER:
			    if (input_device_record->mx_type == MXT_SCL_EPICS){
				allowed_device = TRUE;
			    } else {
				allowed_device = FALSE;
			    }
			    break;
			case MXC_TIMER:
			    if (input_device_record->mx_type == MXT_TIM_EPICS){
				allowed_device = TRUE;
			    } else {
				allowed_device = FALSE;
			    }
			    break;
			default:
			    allowed_device = FALSE;
			    break;
			}
		}

		if ( allowed_device == FALSE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
"Input device '%s' is not a Joerger scaler or timer channel.  "
"Only Joerger scaler or timer channels may be used by Joerger quick scans.",
				input_device_record->name );
		}

		/* The timer record should always be the first input
		 * device channel.
		 */

		if ( i == 0 ) {
			if ( input_device_record->mx_type != MXT_TIM_EPICS ) {
				return mx_error( MXE_TYPE_MISMATCH, fname,
		"The first input device channel for a Joerger quick scan "
		"must always be the timer channel for that Joerger.  "
		"The input device '%s' is not a Joerger timer.",
					input_device_record->name );
			}

			/* Save the MX record for the timer to be used. */

			timer_record = input_device_record;
		}

		switch( input_device_record->mx_type ) {
		case MXT_SCL_EPICS:
			epics_scaler = (MX_EPICS_SCALER *)
				input_device_record->record_type_struct;

			if ( epics_scaler == (MX_EPICS_SCALER *) NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_EPICS_SCALER pointer for scaler record '%s' is NULL.",
					input_device_record->name );
			}

			joerger_quick_scan->scaler_number[i] =
					epics_scaler->scaler_number;

			break;
		case MXT_TIM_EPICS:
			epics_timer = (MX_EPICS_TIMER *)
				input_device_record->record_type_struct;

			if ( epics_timer == (MX_EPICS_TIMER *) NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_EPICS_TIMER pointer for timer record '%s' is NULL.",
					input_device_record->name );
			}

			joerger_quick_scan->scaler_number[i] = 1;

			break;
		}
	}

	/* Is this scan a preset time scan? */

	if ( strcmp( scan->measurement_type, "preset_time" ) != 0 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The scan '%s' is of measurement type '%s'.  The only measurement "
	"type allowed for Joerger quick scans is 'preset_time'",
			scan->record->name, scan->measurement_type );
	}

	/* Get the EPICS record name for the Joerger counter/timer. */

	epics_timer = (MX_EPICS_TIMER *) timer_record->record_type_struct;

	if ( epics_timer == (MX_EPICS_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The record_type_struct pointer for record '%s' is NULL.",
			timer_record->name );
	}

	strlcpy( joerger_quick_scan->epics_record_name,
			epics_timer->epics_record_name,
			sizeof( joerger_quick_scan->epics_record_name ) );

	/* Get the rate at which the EPICS scaler record sends scaler
	 * data while counting is in progress.
	 */

	snprintf( pvname, sizeof(pvname),
		"%s.RATE", epics_timer->epics_record_name );

	mx_status = mx_caget_by_name( pvname,
				MX_CA_DOUBLE, 1,  &epics_update_rate );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The update rate of the EPICS scaler record defines a minimum
	 * plausible value for the measurement time per point of a Joerger
	 * quick scan.  This value is approximately (1.0/epics_update_rate)
	 * If the user is trying to run a scan with a shorter integration
	 * time than that, abort the scan.
	 */

	measurement_time_per_point
		= mx_quick_scan_get_measurement_time( quick_scan );

	update_intervals_per_measurement
		= epics_update_rate * measurement_time_per_point;

	MX_DEBUG( 2,("%s: measurement_time_per_point = %g",
				fname, measurement_time_per_point));
	MX_DEBUG( 2,("%s: epics_update_rate = %g", fname, epics_update_rate));
	MX_DEBUG( 2,("%s: update_intervals_per_measurement = %g",
				fname, update_intervals_per_measurement));

	if ( update_intervals_per_measurement < 0.99 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The requested measurement time per point (%g sec) for quick scan '%s' "
"is too short.  The minimum allowed measurement time per point is %g sec.",
			measurement_time_per_point,
			scan->record->name,
			mx_divide_safely( 1.0, epics_update_rate ) );
	}
			

	/* Allocate memory for the motor position array. */

	for ( i = 0; i < scan->num_motors; i++ ) {

		joerger_quick_scan->motor_position_array[i] = (double *)
		malloc(quick_scan->actual_num_measurements * sizeof(double));

		if ( joerger_quick_scan->motor_position_array[i] == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an %ld array "
			"of motor positions for motor %ld.",
				quick_scan->actual_num_measurements, i );
		}

		switch( scan->motor_record_array[i]->mx_type ) {
		case MXT_MTR_EPICS:
			epics_motor = (MX_EPICS_MOTOR *)
			    scan->motor_record_array[i]->record_type_struct;

			snprintf( joerger_quick_scan->motor_name_array[i],
				sizeof(joerger_quick_scan->motor_name_array[0]),
				"%s.RBV", epics_motor->epics_record_name );
			break;
		default:
			strlcpy( joerger_quick_scan->motor_name_array[i],
				scan->motor_record_array[i]->name,
			    sizeof(joerger_quick_scan->motor_name_array[0]) );
			break;
		}
	}

	/* Allocate memory for the scaler data array. */

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		joerger_quick_scan->data_array[i] = (uint32_t *)
	    malloc( quick_scan->actual_num_measurements * sizeof(uint32_t) );

		if ( joerger_quick_scan->data_array[i] == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an %ld array "
			"of data values for scaler %ld.",
				quick_scan->actual_num_measurements, i );
		}

		if ( scan->input_device_array[i] == timer_record ) {
		    snprintf( joerger_quick_scan->data_name_array[i],
				sizeof(joerger_quick_scan->data_name_array[0]),
				"%s.S1",
				joerger_quick_scan->epics_record_name );
		} else {
		    snprintf( joerger_quick_scan->data_name_array[i],
				sizeof(joerger_quick_scan->data_name_array[0]),
				"%s.S%d",
				joerger_quick_scan->epics_record_name,
				joerger_quick_scan->scaler_number[i] );
		}
#if 0
		MX_DEBUG(-2,("input_device[%ld] = '%s'",
			i, joerger_quick_scan->data_name_array[i]));
#endif
	}

	/* Is this scan supposed to be a synchronous motion mode scan? */

	synchronous_motion_mode_record = mx_get_record( scan->record,
					MX_SCAN_SYNCHRONOUS_MOTION_MODE );

	if ( synchronous_motion_mode_record == (MX_RECORD *) NULL ) {
		quick_scan->use_synchronous_motion_mode = FALSE;
	} else {
		mx_status = mx_get_bool_variable(synchronous_motion_mode_record,
				&(quick_scan->use_synchronous_motion_mode));
	}

	/* Initialize the datafile and plotting support. */

	mx_status = mx_standard_prepare_for_scan_start( scan );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_ARRAYS;
		return mx_status;
	}

	if ( mx_plotting_is_enabled( scan->record ) ) {
		mx_status = mx_plot_start_plot_section( &(scan->plot) );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_ARRAYS;
			return mx_status;
		}
	}

	/* Send the motors to the start position for the scan. */

	mx_scanlog_info("Moving to start position.");

	mx_status = mx_motor_array_move_absolute(
			scan->num_motors,
			scan->motor_record_array,
			quick_scan->start_position,
			0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_ARRAYS;
		return mx_status;
	}

	/* Make sure the timer is in preset time mode and all the
	 * scalers are in preset count mode.
	 */

	mx_status = mx_timer_set_mode( timer_record, MXCM_PRESET_MODE );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_ARRAYS;
		return mx_status;
	}

	mx_status = mx_timer_set_modes_of_associated_counters( timer_record );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_ARRAYS;
		return mx_status;
	}

	/* Get the clock frequency for the timer. */

	snprintf( pvname, sizeof(pvname),
		"%s.FREQ", epics_timer->epics_record_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_DOUBLE, 1,
					&(epics_timer->clock_frequency) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epics_timer->clock_frequency <= 0.0 ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The EPICS clock tick frequency '%s' has the illegal value "
	"of %g ticks per second.  "
	"EPICS clock tick frequencies must be positive numbers.",
			pvname, epics_timer->clock_frequency );
	}

	if ( epics_timer->clock_frequency < 1.0e-12 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The EPICS clock tick frequency '%s' has the implausible value "
	"of %g ticks per second.  A clock with this frequency that had "
	"been started 10000 years ago would not yet have reached the time "
	"for its second tick.",
			pvname, epics_timer->clock_frequency );
	}

	scan_duration_seconds = quick_scan->actual_num_measurements
			* mx_quick_scan_get_measurement_time( quick_scan );

	quick_scan->estimated_scan_duration = scan_duration_seconds;

	mx_info("The scan '%s' is predicted to last for at least %g seconds.",
		scan->record->name, scan_duration_seconds);

	scan_duration_ticks = scan_duration_seconds
					* epics_timer->clock_frequency;

	if ( scan_duration_ticks > (double) MXS_JQ_MAX_JOERGER_CLOCK_TICKS ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The predicted scan time of %g seconds is longer than the longest "
	"counting time supported by the Joerger scaler which is %g seconds.",
		scan_duration_seconds,
		mx_divide_safely( (double) MXS_JQ_MAX_JOERGER_CLOCK_TICKS,
					epics_timer->clock_frequency ));
	}

	/* Configure the Joerger timer via EPICS to count for the longest
	 * possible interval of 2**32 - 1 timer ticks which is 7 minutes
	 * and 9.5 seconds if using the standard frequency of 1.0e7.
	 */

	snprintf( pvname, sizeof(pvname),
		"%s.PR1", joerger_quick_scan->epics_record_name );

	timer_preset = MXS_JQ_MAX_JOERGER_CLOCK_TICKS;

	mx_status = mx_caput_by_name( pvname, MX_CA_LONG, 1, &timer_preset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the motors to get to their start positions. */

	mx_status = mx_wait_for_motor_array_stop( scan->num_motors,
				scan->motor_record_array, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_ARRAYS;
		return mx_status;
	}

	mx_info("All motors are at the start position.");

	/**** Set the motor speeds for the quick scan. ****/

	for ( i = 0; i < scan->num_motors; i++ ) {

		/* Save the old motor speed and synchronous motion mode. */

		motor_record = (scan->motor_record_array)[i];

		mx_status = mx_motor_save_speed( motor_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_ARRAYS;
			return mx_status;
		}

		/* Change the motor speeds. */

		mx_status = mx_motor_set_speed_between_positions(
					motor_record,
					(quick_scan->start_position)[i],
					(quick_scan->end_position)[i],
					scan_duration_seconds );

		if ( mx_status.code != MXE_SUCCESS ) {

			/* If the speed change failed, restore the old speed. */

			for ( j = 0; j < i; j++ ) {
				(void) mx_motor_restore_speed(
					(scan->motor_record_array)[i] );
			}

			FREE_ARRAYS;
			return mx_status;
		}
	}

	/* Compute the acceleration time and the real start and end positions
	 * correcting for the acceleration distance.
	 */

	mx_status = mxs_joerger_quick_scan_compute_scan_parameters(
			scan, quick_scan, joerger_quick_scan );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_scan_restore_speeds( scan );
		FREE_ARRAYS;
		return mx_status;
	}

	/**** Move to the 'real' start position. ****/

	mx_info("Correcting for acceleration distance.");

	mx_status = mx_motor_array_move_absolute(
			scan->num_motors,
			scan->motor_record_array,
			joerger_quick_scan->real_start_position,
			0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_scan_restore_speeds( scan );
		FREE_ARRAYS;
		return mx_status;
	}

	mx_status = mx_wait_for_motor_array_stop( scan->num_motors,
				scan->motor_record_array, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_scan_restore_speeds( scan );
		FREE_ARRAYS;
		return mx_status;
	}

	mx_info("Correction for acceleration distance complete.");

	/**** Switch to synchronous motion mode if requested. ****/

	if ( quick_scan->use_synchronous_motion_mode ) {
		for ( i = 0; i < scan->num_motors; i++ ) {
			motor_record = (scan->motor_record_array)[i];

			mx_status = mx_motor_set_synchronous_motion_mode(
						motor_record, TRUE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/**** Display the quick scan parameters. ****/

	for ( i = 0; i < scan->num_motors; i++ ) {
		motor_record = (scan->motor_record_array)[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		mx_info("Motor '%s' will scan from %g %s to %g %s.",
			motor_record->name,
			quick_scan->start_position[i],
			motor->units,
			quick_scan->end_position[i],
			motor->units );
	}

	/* Record the fact that we have not performed any measurements yet. */

	joerger_quick_scan->num_completed_measurements = 0;

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/* Do not use FREE_ARRAYS in the execute_scan_body() function since
 * the cleanup_after_scan_end() function will attempt to use the arrays
 * regardless of what error code this routine returns.
 */

MX_EXPORT mx_status_type
mxs_joerger_quick_scan_execute_scan_body( MX_SCAN *scan )
{
	const char fname[] = "mxs_joerger_quick_scan_execute_scan_body()";

	MX_QUICK_SCAN *quick_scan;
	MX_JOERGER_QUICK_SCAN *joerger_quick_scan;
	MX_EPICS_GROUP epics_group;
	MX_EPICS_TIMER *epics_timer;
	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	char pvname[MXU_EPICS_PVNAME_LENGTH+1];
	int interrupt;
#if 1
	int direction;
#endif
	mx_bool_type limit_hit;
	long i, j;
	unsigned long sleep_time;
	int32_t count;
	mx_bool_type fast_mode, start_fast_mode;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxs_joerger_quick_scan_get_pointers( scan,
			&quick_scan, &joerger_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	preset_time_struct = (MX_MEASUREMENT_PRESET_TIME *)
				scan->measurement.measurement_type_struct;

	if ( preset_time_struct == (MX_MEASUREMENT_PRESET_TIME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"measurement_type_struct for scan '%s' is NULL.",
			scan->record->name );
	}
	if ( preset_time_struct->timer_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"timer_record pointer in preset_time_struct for scan '%s' is NULL.",
			scan->record->name );
	}

	epics_timer = (MX_EPICS_TIMER *)
			preset_time_struct->timer_record->record_type_struct;

	if ( epics_timer == (MX_EPICS_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_EPICS_TIMER pointer for timer record '%s' used by scan '%s' is NULL.",
			preset_time_struct->timer_record->name,
			scan->record->name );
	}

	sleep_time = mx_round( 1000.0 *
			mx_quick_scan_get_measurement_time(quick_scan) );

	/* Is fast mode already on?  If not, then arrange for it to be
	 * turned on after the first measurement.
	 */

	mx_status = mx_get_fast_mode( scan->record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fast_mode ) {
		start_fast_mode = FALSE;
	} else {
		start_fast_mode = TRUE;
	}

	/* Start the motors. */

	mx_info("Starting the scan.");

#if 1
	for ( i = 0; i < scan->num_motors; i++ ) {

		if ( quick_scan->end_position[i]
			>= quick_scan->start_position[i] )
		{
			direction = 1;
		} else {
			direction = -1;
		}

		mx_status = mx_motor_constant_velocity_move(
				scan->motor_record_array[i], direction );

		if ( mx_status.code != MXE_SUCCESS ) {
			for ( j = 0; j < i; j++ ) {
				(void) mx_motor_soft_abort(
					scan->motor_record_array[i] );
			}
			return mx_status;
		}
	}
#else
	mx_status = mx_motor_array_move_absolute(
				scan->num_motors,
				scan->motor_record_array,
				joerger_quick_scan->real_end_position,
				MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	MX_DEBUG( 2,("%s: starting the timer.", fname));

	/* Start the timer. */

	snprintf( pvname, sizeof(pvname),
		"%s.CNT", joerger_quick_scan->epics_record_name );

	count = 1;

	if ( epics_timer->epics_record_version < 3.0 ) {
		mx_status = mx_caput_by_name( pvname, MX_CA_LONG, 1, &count );
	} else {
		mx_status = mx_caput_nowait_by_name( pvname,
						MX_CA_LONG, 1, &count );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info("Quick scan is in progress...");

	/* Read out the data from the scalers as it comes in. */

	for ( i = 0; i < quick_scan->actual_num_measurements; i++ ) {

		interrupt = mx_user_requested_interrupt();

		if ( interrupt == MXF_USER_INT_ABORT ) {

			for ( j = 0; j < scan->num_motors; j++ ) {
				(void) mx_motor_soft_abort(
						scan->motor_record_array[j] );
			}
			return mx_error( MXE_INTERRUPTED, fname,
				"Scan aborted." );
		}

		mx_info("%ld", i);

		for ( j = 0; j < scan->num_motors; j++ ) {
			switch( scan->motor_record_array[j]->mx_type ) {
			case MXT_MTR_EPICS:
			case MXT_MTR_PMAC_EPICS_TC:
			case MXT_MTR_PMAC_EPICS_BIO:
				mx_status = mx_caget_by_name(
					joerger_quick_scan->motor_name_array[j],
					MX_CA_DOUBLE, 1, 
			&(joerger_quick_scan->motor_position_array[j][i]) );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
				break;
			default:
				mx_status = mx_motor_get_position(
					scan->motor_record_array[j],
			&(joerger_quick_scan->motor_position_array[j][i]) );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			}
		}

		mx_epics_start_group( &epics_group );

		for ( j = 0; j < scan->num_input_devices; j++ ) {

			mx_status = mx_caget_by_name(
				joerger_quick_scan->data_name_array[j],
				MX_CA_LONG, 1, 
				&(joerger_quick_scan->data_array[j][i]) );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_epics_delete_group( &epics_group );

				return mx_status;
			}
		}
		mx_epics_end_group( &epics_group );

#if 0
		for ( j = 0; j < scan->num_motors; j++ ) {
			fprintf(stderr, "%g  ",
				joerger_quick_scan->motor_position_array[j][i]);
		}
		for ( j = 0; j < scan->num_input_devices; j++ ) {
			fprintf(stderr, "%lu  ",
				(unsigned long) joerger_quick_scan->data_array[j][i]);
		}
		fprintf(stderr, "\n" );
#endif
		/* If needed, start fast mode. */

		if ( start_fast_mode ) {
			mx_status = mx_set_fast_mode( scan->record, TRUE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			start_fast_mode = FALSE;
		}

		/* Record the fact that we have completed the next measurement
		 * and then wait for the requested dwell time.
		 */

		(joerger_quick_scan->num_completed_measurements)++;

		mx_msleep( sleep_time );
	}

	mx_info("Quick scan complete.");

	/* See if any of the motors hit a limit during the scan. */

	for ( i = 0; i < scan->num_motors; i++ ) {
		(void) mx_motor_positive_limit_hit(
				scan->motor_record_array[i], &limit_hit );
		(void) mx_motor_negative_limit_hit(
				scan->motor_record_array[i], &limit_hit );
	}

	MX_DEBUG( 2,("%s: Stopping the motors.",fname));

	/* Stop the motors. */

	for ( i = 0; i < scan->num_motors; i++ ) {
		(void) mx_motor_soft_abort( scan->motor_record_array[i] );
	}

	MX_DEBUG( 2,("%s: Stopping the timer.", fname));

	/* Stop the timer. */

	snprintf( pvname, sizeof(pvname),
		"%s.CNT", joerger_quick_scan->epics_record_name );

	count = 0;

	mx_status = mx_caput_by_name( pvname, MX_CA_LONG, 1, &count );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the motors have stopped so that it is safe to send
	 * them new commands.
	 */

	(void) mx_wait_for_motor_array_stop( scan->num_motors,
				scan->motor_record_array, 0 );

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/* Please note that most error conditions are ignored in the function
 * mxs_joerger_quick_scan_cleanup_after_scan_end() since it is important
 * that the data be saved to disk in spite of any extraneous errors that
 * may occur.
 */

MX_EXPORT mx_status_type
mxs_joerger_quick_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	const char fname[] = "mxs_joerger_quick_scan_cleanup_after_scan_end()";

	MX_QUICK_SCAN *quick_scan;
	MX_JOERGER_QUICK_SCAN *joerger_quick_scan;
	MX_EPICS_TIMER *epics_timer;
	double motor_positions[ MXS_JQ_MAX_MOTORS ];
	long data_values[ MXS_JQ_MAX_JOERGER_SCALERS ];

#if 0  /* WML */
	MX_SCALER *scaler;
	double dark_current[ MXS_JQ_MAX_JOERGER_SCALERS ];
#endif /* WML */

	double measurement_time;
	long i, j;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxs_joerger_quick_scan_get_pointers( scan,
			&quick_scan, &joerger_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS ) {

		/* Can't go any farther if this fails, so we abort. */

		(void) mx_scan_restore_speeds( scan );
		FREE_ARRAYS;
		return mx_status;
	}

	/* The first channel in Joerger quick scans is always the timer. */

	epics_timer = (MX_EPICS_TIMER *)
			(scan->input_device_array[0])->record_type_struct;

	/* Restore the old motor speeds and synchronous motion mode
	 * for this quick scan.
	 */

	(void) mx_scan_restore_speeds( scan );

	mx_info("Computing scaler values." );

	/* Record the dark currents in a local array. */

#if 0  /* WML: Dark current subtraction has been moved to the scaler driver. */

	for ( j = 0; j < scan->num_input_devices; j++ ) {

		scaler = (MX_SCALER *)
			(scan->input_device_array[j])->record_class_struct;

		if ( scaler->scaler_flags & MXF_SCL_SUBTRACT_DARK_CURRENT ) {
			dark_current[j] = scaler->dark_current;
		} else {
			dark_current[j] = 0.0;
		}
	}

#endif /* WML */

	/* Copy the data in the scan data arrays to the datafile
	 * and send another copy to the plotting program.
	 */

	for ( i = 1; i < joerger_quick_scan->num_completed_measurements; i++ ){

		for ( j = 0; j < scan->num_motors; j++ ) {
			motor_positions[j] = 
				joerger_quick_scan->motor_position_array[j][i];
		}
		for ( j = 0; j < scan->num_input_devices; j++ ) {
			data_values[j] =
			    (long) (joerger_quick_scan->data_array[j][i])
			      - (long) (joerger_quick_scan->data_array[j][i-1]);
		}

		/* Skip duplicate points by looking at the timer value
		 * in input device channel 0.  If the timer value has not
		 * changed, skip the point.
		 */

		if ( data_values[0] == 0 ) {

			MX_DEBUG( 2,("%s: Discarding measurement %ld",
					fname, i));

			continue;  /* Go back to the top of the for() loop. */
		}

		/* Channel 0 should always be the timer count, so we 
		 * multiply it by the clock frequency to get the measurement
		 * time in seconds.
		 */

		measurement_time = ((double) data_values[0])
					/ epics_timer->clock_frequency;

		/* Subtract dark currents. */

#if 0  /* WML: Dark current subtraction has been moved to the scaler driver. */

		for ( j = 1; j < scan->num_input_devices; j++ ) {
			data_values[j] -= mx_round( dark_current[j]
							* measurement_time );
		}

#endif /* WML */

		MX_DEBUG( 2,("%s: Copying measurement %ld to data file.",
				fname, i));

		mx_status = mx_add_array_to_datafile( &(scan->datafile),
			MXFT_DOUBLE, scan->num_motors, motor_positions,
			MXFT_LONG, scan->num_input_devices, data_values );

		if ( mx_status.code != MXE_SUCCESS ) {

			/* If we cannot write the data to the datafile,
			 * all is lost, so we abort.
			 */

			FREE_ARRAYS;
			return mx_status;
		}

		if ( mx_plotting_is_enabled( scan->record ) ) {
			MX_DEBUG( 2,(
			    "%s: Adding measurement %ld to plot buffer.",
				fname, i));

			/* Failing to update the plot correctly is not
			 * a reason to abort since that would interrupt
			 * the writing of the datafile.
			 */

			(void) mx_add_array_to_plot_buffer( &(scan->plot),
			    MXFT_DOUBLE, scan->num_motors, motor_positions,
			    MXFT_LONG, scan->num_input_devices, data_values );
		}
	}

	if ( mx_plotting_is_enabled( scan->record ) ) {
		/* Attempt to display the plot, but do not abort on an error.*/

		(void) mx_display_plot( &(scan->plot) );
	}

	/* Close the datafile and shut down the plot. */

	mx_status = mx_standard_cleanup_after_scan_end( scan );

	/* Free all of the data arrays that were allocated in the routine
	 * mxs_joerger_quick_scan_prepare_for_scan_start().
	 */

	FREE_ARRAYS;

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

