/*
 * Name:    s_pseudomotor.c
 *
 * Purpose: Scan driver for both pseudomotor and relative linear scans.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006, 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "mx_driver.h"
#include "s_pseudomotor.h"
#include "d_network_motor.h"
#include "d_slit_motor.h"
#include "d_trans_motor.h"

/* Initialize the pseudomotor scan driver jump table. */

MX_LINEAR_SCAN_FUNCTION_LIST mxs_pseudomotor_linear_scan_function_list = {
	mxs_pseudomotor_scan_create_record_structures,
	mxs_pseudomotor_scan_finish_record_initialization,
	NULL,
	mxs_pseudomotor_scan_prepare_for_scan_start,
	mxs_pseudomotor_scan_compute_motor_positions,
	mxs_pseudomotor_scan_motor_record_array_move_special,
	mxs_pseudomotor_scan_cleanup_after_scan_end
};

/* We have our own private MX_SCAN_FUNCTION_LIST so that we may intercept
 * the prepare_for_scan_start function call before passing control on
 * to the regular mxs_linear_scan_prepare_for_scan_start function.
 */

MX_SCAN_FUNCTION_LIST mxs_pseudomotor_scan_function_list = {
	mxs_pseudomotor_scan_prepare_for_scan_start,
	mxs_linear_scan_execute_scan_body
};

MX_RECORD_FIELD_DEFAULTS mxs_pseudomotor_linear_scan_dflts[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_LINEAR_SCAN_STANDARD_FIELDS
};

long mxs_pseudomotor_linear_scan_num_record_fields
			= sizeof( mxs_pseudomotor_linear_scan_dflts )
			/ sizeof( mxs_pseudomotor_linear_scan_dflts[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_pseudomotor_linear_scan_def_ptr
			= &mxs_pseudomotor_linear_scan_dflts[0];

/***************/

/* Initialize the relative scan driver jump table. */

MX_LINEAR_SCAN_FUNCTION_LIST mxs_relative_linear_scan_function_list = {
	mxs_pseudomotor_scan_create_record_structures,
	mxs_pseudomotor_scan_finish_record_initialization,
	NULL,
	mxs_pseudomotor_scan_prepare_for_scan_start,
	mxs_pseudomotor_scan_compute_motor_positions,
	mxs_pseudomotor_scan_motor_record_array_move_special,
	mxs_pseudomotor_scan_cleanup_after_scan_end
};

/* We have our own private MX_SCAN_FUNCTION_LIST so that we may intercept
 * the prepare_for_scan_start function call before passing control on
 * to the regular mxs_linear_scan_prepare_for_scan_start function.
 */

MX_SCAN_FUNCTION_LIST mxs_relative_scan_function_list = {
	mxs_pseudomotor_scan_prepare_for_scan_start,
	mxs_linear_scan_execute_scan_body
};

MX_RECORD_FIELD_DEFAULTS mxs_relative_linear_scan_dflts[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_LINEAR_SCAN_STANDARD_FIELDS
};

long mxs_relative_linear_scan_num_record_fields
			= sizeof( mxs_relative_linear_scan_dflts )
			/ sizeof( mxs_relative_linear_scan_dflts[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_relative_linear_scan_def_ptr
			= &mxs_relative_linear_scan_dflts[0];

/***************/

MX_EXPORT mx_status_type
mxs_pseudomotor_scan_create_record_structures( MX_RECORD *record,
		MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan )
{
	static const char fname[]
		= "mxs_pseudomotor_scan_create_record_structures()";

	MX_PSEUDOMOTOR_SCAN *pseudomotor_scan;

	/* At this point, the database may still be in the process of
	 * initializing itself, so it does not necessarily know yet
	 * which records are for pseudomotors and which are not.
	 * Thus, all it is safe for us to allocate at this time
	 * is the record type structure itself.
	 */

	pseudomotor_scan = (MX_PSEUDOMOTOR_SCAN *)
				malloc( sizeof(MX_PSEUDOMOTOR_SCAN) );

	if ( pseudomotor_scan == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating an MX_PSEUDOMOTOR_SCAN structure." );
	}

	record->record_type_struct = pseudomotor_scan;
	linear_scan->record_type_struct = pseudomotor_scan;

	record->class_specific_function_list
				= &mxs_pseudomotor_linear_scan_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_pseudomotor_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxs_pseudomotor_scan_finish_record_initialization()";

	MX_SCAN *scan;
	long i;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	scan = (MX_SCAN *) record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SCAN pointer for record '%s' is NULL.",
			record->name );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_pseudomotor_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	MX_PSEUDOMOTOR_SCAN *pseudomotor_scan;

	pseudomotor_scan = (MX_PSEUDOMOTOR_SCAN *)
				scan->record->record_type_struct;

	pseudomotor_scan->first_step = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_pseudomotor_scan_compute_motor_positions( MX_SCAN *scan,
					MX_LINEAR_SCAN *linear_scan )
{
	static const char fname[]
		= "mxs_pseudomotor_scan_compute_motor_positions()";

	double *motor_position;
	long i;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCAN scan structure pointer is NULL.");
	}

	if ( linear_scan == (MX_LINEAR_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LINEAR_SCAN structure pointer is NULL.");
	}

	motor_position = scan->motor_position;

	if ( motor_position == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"motor_position array pointer is NULL." );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {
		motor_position[i] = linear_scan->start_position[i]
	 		+ (linear_scan->step_size[i])
				* (double)(linear_scan->step_number[i]);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_pseudomotor_scan_motor_record_array_move_special(
				MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan,
				long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position,
				MX_MOTOR_MOVE_REPORT_FUNCTION fptr,
				unsigned long flags )
{
	static const char fname[]
		= "mxs_pseudomotor_scan_motor_record_array_move_special()";

	MX_PSEUDOMOTOR_SCAN *pseudomotor_scan;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	long i;
	double current_position;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.",fname));

	pseudomotor_scan = (MX_PSEUDOMOTOR_SCAN *)
				scan->record->record_type_struct;

	/* If this is the first step in the scan, tell each motor record
	 * to compute saved start positions.  At present (Sept. 2002),
	 * this request is ignored for all motor types except network
	 * motors, slit motors and translation motors.
	 */

	if ( pseudomotor_scan->first_step ) {
		pseudomotor_scan->first_step = FALSE;

		if ( (scan->record->mx_type) == MXS_LIN_RELATIVE ) {
			for ( i = 0; i < num_motor_records; i++ ) {
				motor_record = (scan->motor_record_array)[i];

				mx_status = mx_motor_get_position(
					motor_record, &current_position );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				mx_status = mx_motor_save_start_positions(
					motor_record, current_position );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		} else {
			for ( i = 0; i < num_motor_records; i++ ) {
				motor_record = (scan->motor_record_array)[i];

				mx_status = mx_motor_save_start_positions(
						motor_record,
						linear_scan->start_position[i]);

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		}
	}

	/* Tell all the motor records to use saved start positions when
	 * computing their next move destination.  As mentioned above,
	 * this will be ignored for most motor types.
	 */

	for ( i = 0; i < num_motor_records; i++ ) {

		mx_status = mx_motor_use_start_positions(
					(scan->motor_record_array)[i] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If this is a relative scan, then the computed motors are offsets
	 * relative to the start position.  However, we must send absolute
	 * positions to the actual motors, so we add the saved start position
	 * for each motor to the computed motor positions.
	 */

	if ( (scan->record->mx_type) == MXS_LIN_RELATIVE ) {
		MX_DEBUG( 2,
		("%s: computing absolute positions from relative positions.",
		 	fname ));

		for ( i = 0; i < num_motor_records; i++ ) {
			motor = 
			   (scan->motor_record_array)[i]->record_class_struct;

			(scan->motor_position)[i] +=
			   ((motor->raw_saved_start_position) * (motor->scale));
		}
	}

	/* Now we may finally move the motors. */

	mx_status = mx_motor_array_move_absolute(
				num_motor_records, motor_record_array,
				position, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_pseudomotor_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_pseudomotor_scan_cleanup_after_scan_end()";

	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	int i;
#if 0
	double saved_start_position;
#else
	double *destination_array;
#endif
	mx_status_type mx_status;

	if ( (scan->record->mx_type != MXS_LIN_RELATIVE ) ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If this is a 'relative_scan' then we need to move the motors
	 * back to their start positions.
	 */

	mx_info( "Moving motors back to their original positions." );

#if 0
	/* This version moves motors one at a time. */

	for ( i = 0; i < scan->num_motors; i++ ) {
		motor_record = (scan->motor_record_array)[i];

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for motor %d used by scan '%s' is NULL.",
				i, scan->record->name );
		}

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_MOTOR pointer for motor '%s' used by scan '%s' is NULL.",
				motor_record->name, scan->record->name );
		}

		saved_start_position = motor->save_start_positions;

		mx_status = mx_motor_move_absolute( motor_record,
						saved_start_position, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Wait until the motor gets back to its start position. */

		mx_status = mx_wait_for_motor_stop( motor_record, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
#else
	/* This version moves motors simultaneously. */

	destination_array = (double *)
				malloc( scan->num_motors * sizeof(double) );

	if ( destination_array == (double *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory allocating a %ld array of motor destinations.",
			scan->num_motors );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {
		motor_record = (scan->motor_record_array)[i];

		if ( motor_record == (MX_RECORD *) NULL ) {
			mx_free( destination_array );
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for motor %d used by scan '%s' is NULL.",
				i, scan->record->name );
		}

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor == (MX_MOTOR *) NULL ) {
			mx_free( destination_array );
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_MOTOR pointer for motor '%s' used by scan '%s' is NULL.",
				motor_record->name, scan->record->name );
		}

		destination_array[i] = motor->save_start_positions;
	}

	mx_status = mx_motor_array_move_absolute( scan->num_motors,
						scan->motor_record_array,
						destination_array, 0 );

	mx_free( destination_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_wait_for_motor_array_stop( scan->num_motors,
						scan->motor_record_array, 0 );
#endif

	return mx_status;
}

