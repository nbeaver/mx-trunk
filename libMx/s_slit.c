/*
 * Name:    s_slit.c
 *
 * Purpose: Scan description file for slit linear scan.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
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
#include "s_slit.h"
#include "d_slit_motor.h"

/* Initialize the slit scan driver jump table. */

MX_LINEAR_SCAN_FUNCTION_LIST mxs_slit_linear_scan_function_list = {
	mxs_slit_scan_create_record_structures,
	mxs_slit_scan_finish_record_initialization,
	mxs_slit_scan_delete_record,
	mxs_slit_scan_prepare_for_scan_start,
	mxs_slit_scan_compute_motor_positions,
	mxs_slit_scan_motor_record_array_move_special
};

/* We have our own private MX_SCAN_FUNCTION_LIST so that we may intercept
 * the prepare_for_scan_start function call before passing control on
 * to the regular mxs_linear_scan_prepare_for_scan_start function.
 */

MX_SCAN_FUNCTION_LIST mxs_slit_scan_function_list = {
	mxs_slit_scan_prepare_for_scan_start,
	mxs_linear_scan_execute_scan_body
};

MX_RECORD_FIELD_DEFAULTS mxs_slit_linear_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_LINEAR_SCAN_STANDARD_FIELDS
};

long mxs_slit_linear_scan_num_record_fields
			= sizeof( mxs_slit_linear_scan_defaults )
			/ sizeof( mxs_slit_linear_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_slit_linear_scan_def_ptr
			= &mxs_slit_linear_scan_defaults[0];

MX_EXPORT mx_status_type
mxs_slit_scan_create_record_structures( MX_RECORD *record,
		MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan )
{
	const char fname[] = "mxs_slit_scan_create_record_structures()";

	MX_SLIT_SCAN *slit_scan;

	/* At this point, the database may still be in the process of
	 * initializing itself, so it does not necessarily know yet
	 * which records are for slit motors and which are not.
	 * Thus, all it is safe for us to allocate at this time
	 * is the record type structure itself.
	 */

	slit_scan = (MX_SLIT_SCAN *) malloc( sizeof(MX_SLIT_SCAN) );

	if ( slit_scan == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an MX_SLIT_SCAN structure." );
	}

	record->record_type_struct = slit_scan;
	linear_scan->record_type_struct = slit_scan;

	slit_scan->is_slit_motor = NULL;
	slit_scan->real_motor_record_array = NULL;
	slit_scan->real_motor_start_position = NULL;
	slit_scan->real_motor_position = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_slit_scan_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxs_slit_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_LINEAR_SCAN *linear_scan;
	MX_SLIT_SCAN *slit_scan;
	MX_RECORD *motor_record;
	long i;

	record->class_specific_function_list
				= &mxs_slit_linear_scan_function_list;

	scan = (MX_SCAN *)(record->record_superclass_struct);

	record->class_specific_function_list
				= &mxs_slit_linear_scan_function_list;

	linear_scan = (MX_LINEAR_SCAN *)(record->record_class_struct);
	slit_scan = (MX_SLIT_SCAN *)(record->record_type_struct);

	slit_scan->is_slit_motor = (int *) malloc( sizeof(int)
					* (scan->num_motors) );

	if ( slit_scan->is_slit_motor == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory allocating the 'is_slit_motor' array for an MX_SLIT_SCAN.");
	}

	slit_scan->num_real_motors = 0L;

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;

		motor_record = (scan->motor_record_array)[i];

		if ( motor_record->mx_type == MXT_MTR_SLIT_MOTOR ) {
			slit_scan->is_slit_motor[i] = TRUE;
			slit_scan->num_real_motors += 2L;
		} else {
			slit_scan->is_slit_motor[i] = FALSE;
			slit_scan->num_real_motors += 1L;
		}
	}

	slit_scan->real_motor_start_position
	    = (double *) malloc( sizeof(double) * slit_scan->num_real_motors );

	if ( slit_scan->real_motor_start_position == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating real_motor_start_position array "
		"for MX_SLIT_SCAN." );
	}

	slit_scan->real_motor_position
	    = (double *) malloc( sizeof(double) * slit_scan->num_real_motors );

	if ( slit_scan->real_motor_position == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating real_motor_position array "
		"for MX_SLIT_SCAN." );
	}

	slit_scan->real_motor_record_array = (MX_RECORD **) malloc(
		slit_scan->num_real_motors * sizeof(MX_RECORD *) );

	if ( slit_scan->real_motor_record_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating real_motor_record_array "
		"for MX_SLIT_SCAN." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_slit_scan_delete_record( MX_RECORD *record )
{
	MX_SLIT_SCAN *slit_scan;
	long j;

	slit_scan = (MX_SLIT_SCAN *)(record->record_type_struct);

	if ( slit_scan != (MX_SLIT_SCAN *) NULL ) {
		if ( slit_scan->is_slit_motor != (int *) NULL ) {
			free( slit_scan->is_slit_motor );

			slit_scan->is_slit_motor = NULL;
		}
		if (slit_scan->real_motor_record_array != (MX_RECORD **) NULL){
			for ( j = 0; j < slit_scan->num_real_motors; j++ ) {
				slit_scan->real_motor_record_array[j] = NULL;
			}

			free( slit_scan->real_motor_record_array );

			slit_scan->real_motor_record_array = NULL;
		}
		if ( slit_scan->real_motor_start_position != (double *) NULL) {
			free( slit_scan->real_motor_start_position );

			slit_scan->real_motor_start_position = NULL;
		}
		if ( slit_scan->real_motor_position != (double *) NULL) {
			free( slit_scan->real_motor_position );

			slit_scan->real_motor_position = NULL;
		}

		free( slit_scan );

		record->record_type_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_slit_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	MX_SLIT_SCAN *slit_scan;

	slit_scan = (MX_SLIT_SCAN *) scan->record->record_type_struct;

	slit_scan->first_step = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_slit_scan_compute_motor_positions( MX_SCAN *scan,
					MX_LINEAR_SCAN *linear_scan )
{
	const char fname[] = "mxs_slit_scan_compute_motor_positions()";

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
mxs_slit_scan_motor_record_array_move_special(
				MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan,
				long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position,
				MX_MOTOR_MOVE_REPORT_FUNCTION fptr,
				int flags )
{
	const char fname[] = "mxs_slit_scan_motor_record_array_move_special()";

	MX_SLIT_SCAN *slit_scan;
	MX_SLIT_MOTOR *slit_motor;
	MX_RECORD *slit_motor_record;
	long i, j;
	double negative_original_position, positive_original_position;
	double negative_start_position, positive_start_position;
	double slit_pseudo_motor_start_position;
	double original_slit_center, original_slit_width;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.",fname));

	negative_start_position = positive_start_position = 0.0;

	slit_scan = (MX_SLIT_SCAN *)(scan->record->record_type_struct);

	/* In what is done below, the index i runs over the original list
	 * of motors, while the j index runs over a list that has had each
	 * slit motor replaced by the pair of real motors it is constructed
	 * from.
	 */

	/* If this is the first step in the scan, initialize some variables
	 * that could not be initialized earlier.
	 */

	if ( slit_scan->first_step ) {
		slit_scan->first_step = FALSE;

		for ( i = 0, j = 0; i < num_motor_records; i++, j++ ) {
			if ( slit_scan->is_slit_motor[i] == FALSE ) {
				slit_scan->real_motor_record_array[j]
					= scan->motor_record_array[i];

				slit_scan->real_motor_start_position[j]
					= linear_scan->start_position[i];
			} else {
				slit_motor_record = (MX_RECORD *)
						(scan->motor_record_array[i]);
				slit_motor = (MX_SLIT_MOTOR *)
				    (slit_motor_record->record_type_struct);

				slit_pseudo_motor_start_position
					= linear_scan->start_position[i];

				/* Get the pre-scan positions of the motors. */

				slit_scan->real_motor_record_array[j]
					= slit_motor->negative_motor_record;

				status = mx_motor_get_position(
					slit_motor->negative_motor_record,
					&negative_original_position );

				if ( status.code != MXE_SUCCESS )
					return status;

				slit_scan->real_motor_record_array[j+1]
					= slit_motor->positive_motor_record;

				status = mx_motor_get_position(
					slit_motor->positive_motor_record,
					&positive_original_position );

				if ( status.code != MXE_SUCCESS )
					return status;

				/* Compute the scan starting positions 
				 * depending on the slit motor type.
				 */

				switch( slit_motor->slit_type ) {
				case MXF_SLIT_CENTER_SAME:
					original_slit_width =
					    positive_original_position
						- negative_original_position;

					negative_start_position =
					    slit_pseudo_motor_start_position
						- 0.5 * original_slit_width;

					positive_start_position =
					    slit_pseudo_motor_start_position
						+ 0.5 * original_slit_width;

					break;
				case MXF_SLIT_WIDTH_SAME:
					original_slit_center =
					    0.5 * (positive_original_position
						+ negative_original_position);

					negative_start_position =
					    original_slit_center - 0.5 *
					      slit_pseudo_motor_start_position;

					positive_start_position =
					    original_slit_center + 0.5 *
					      slit_pseudo_motor_start_position;

					break;
				case MXF_SLIT_CENTER_OPPOSITE:
					original_slit_width =
					    positive_original_position
						+ negative_original_position;

					negative_start_position =
					    - slit_pseudo_motor_start_position
						+ 0.5 * original_slit_width;

					positive_start_position =
					    slit_pseudo_motor_start_position
						+ 0.5 * original_slit_width;

					break;
				case MXF_SLIT_WIDTH_OPPOSITE:
					original_slit_center =
					    0.5 * (positive_original_position
						- negative_original_position);

					negative_start_position =
					    - original_slit_center + 0.5 *
					      slit_pseudo_motor_start_position;

					positive_start_position =
					    original_slit_center + 0.5 *
					      slit_pseudo_motor_start_position;

					break;
				}

				MX_DEBUG( 2,
	("%s: negative_start_position = %g, positive_start_position = %g",
					fname, negative_start_position,
					positive_start_position));

				slit_scan->real_motor_start_position[j]
					= negative_start_position;

				j++;

				slit_scan->real_motor_start_position[j]
					= positive_start_position;
			}
		}
	}

	/* Non-slit motor positions are merely copied to real_motor_position,
	 * while the positions of the two real motors corresponding to
	 * each slit pseudo-motor are computed and placed in the
	 * real_motor_position array.
	 */

	for ( i = 0, j = 0; i < num_motor_records; i++, j++ ) {
		if ( slit_scan->is_slit_motor[i] == FALSE ) {
			slit_scan->real_motor_position[j] = position[i];
		} else {
			slit_motor_record = (MX_RECORD *)
						(scan->motor_record_array[i]);
			slit_motor = (MX_SLIT_MOTOR *)
				(slit_motor_record->record_type_struct);

			switch( slit_motor->slit_type ) {
			case MXF_SLIT_CENTER_SAME:
				slit_scan->real_motor_position[j] = 
				    slit_scan->real_motor_start_position[j]
				    + (linear_scan->step_size[i])
				      * (double)(linear_scan->step_number[i]);

				j++;

				slit_scan->real_motor_position[j] = 
				    slit_scan->real_motor_start_position[j]
				    + (linear_scan->step_size[i])
				      * (double)(linear_scan->step_number[i]);

				break;

			case MXF_SLIT_WIDTH_SAME:
				slit_scan->real_motor_position[j] = 
				    slit_scan->real_motor_start_position[j]
				    - 0.5 * (linear_scan->step_size[i])
				      * (double)(linear_scan->step_number[i]);

				j++;

				slit_scan->real_motor_position[j] = 
				    slit_scan->real_motor_start_position[j]
				    + 0.5 * (linear_scan->step_size[i])
				      * (double)(linear_scan->step_number[i]);

				break;

			case MXF_SLIT_CENTER_OPPOSITE:
				slit_scan->real_motor_position[j] = 
				    slit_scan->real_motor_start_position[j]
				    - (linear_scan->step_size[i])
				      * (double)(linear_scan->step_number[i]);

				j++;

				slit_scan->real_motor_position[j] = 
				    slit_scan->real_motor_start_position[j]
				    + (linear_scan->step_size[i])
				      * (double)(linear_scan->step_number[i]);

				break;

			case MXF_SLIT_WIDTH_OPPOSITE:
				slit_scan->real_motor_position[j] = 
				    slit_scan->real_motor_start_position[j]
				    + 0.5 * (linear_scan->step_size[i])
				      * (double)(linear_scan->step_number[i]);

				j++;

				slit_scan->real_motor_position[j] = 
				    slit_scan->real_motor_start_position[j]
				    + 0.5 * (linear_scan->step_size[i])
				      * (double)(linear_scan->step_number[i]);

				break;

			default:
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
					fname, "Unknown slit type %ld.",
					slit_motor->slit_type );
				break;
			}

			MX_DEBUG( 2,
	("%s: real_motor_position[%ld] = %g, real_motor_position[%ld] = %g",
			fname, j-1, slit_scan->real_motor_position[j-1],
			j, slit_scan->real_motor_position[j]));
		}
	}

	/* Now we may finally move the motors. */

	status = mx_motor_array_move_absolute(
			slit_scan->num_real_motors,
			slit_scan->real_motor_record_array,
			slit_scan->real_motor_position,
			0 );

	return status;
}

