/*
 * Name:    s_motor.c
 *
 * Purpose: Scan description file for motor linear scan.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "s_motor.h"

/* Initialize the motor scan driver jump table. */

MX_LINEAR_SCAN_FUNCTION_LIST mxs_motor_linear_scan_function_list = {
	mxs_motor_scan_create_record_structures,
	mxs_motor_scan_finish_record_initialization,
	NULL,
	NULL,
	mxs_motor_scan_compute_motor_positions,
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxs_motor_linear_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_LINEAR_SCAN_STANDARD_FIELDS
};

long mxs_motor_linear_scan_num_record_fields
			= sizeof( mxs_motor_linear_scan_defaults )
			/ sizeof( mxs_motor_linear_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_motor_linear_scan_def_ptr
			= &mxs_motor_linear_scan_defaults[0];

MX_EXPORT mx_status_type
mxs_motor_scan_create_record_structures( MX_RECORD *record,
		MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan )
{
	record->record_type_struct = NULL;
	linear_scan->record_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_motor_scan_finish_record_initialization( MX_RECORD *record )
{
	MX_SCAN *scan;
	long i;

	record->class_specific_function_list
				= &mxs_motor_linear_scan_function_list;

	record->class_specific_function_list
				= &mxs_motor_linear_scan_function_list;

	scan = (MX_SCAN *)(record->record_superclass_struct);

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_motor_scan_compute_motor_positions( MX_SCAN *scan,
					MX_LINEAR_SCAN *linear_scan )
{
	static const char fname[] = "mxs_motor_scan_compute_motor_positions()";

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

