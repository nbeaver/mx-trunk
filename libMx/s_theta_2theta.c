/*
 * Name:    s_theta_2theta.c
 *
 * Purpose: Scan description file for theta-2 theta linear scan.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "s_theta_2theta.h"

MX_LINEAR_SCAN_FUNCTION_LIST mxs_2theta_linear_scan_function_list = {
	mxs_2theta_scan_create_record_structures,
	mxs_2theta_scan_finish_record_initialization,
	NULL,
	NULL,
	mxs_2theta_scan_compute_motor_positions,
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxs_2theta_linear_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_LINEAR_SCAN_STANDARD_FIELDS
};

long mxs_2theta_linear_scan_num_record_fields
			= sizeof( mxs_2theta_linear_scan_defaults )
			/ sizeof( mxs_2theta_linear_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_2theta_linear_scan_def_ptr
			= &mxs_2theta_linear_scan_defaults[0];

MX_EXPORT mx_status_type
mxs_2theta_scan_create_record_structures( MX_RECORD *record,
		MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan )
{
	record->record_type_struct = NULL;
	linear_scan->record_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_2theta_scan_finish_record_initialization( MX_RECORD *record )
{
	MX_SCAN *scan;

	record->class_specific_function_list
				= &mxs_2theta_linear_scan_function_list;

	record->class_specific_function_list
				= &mxs_2theta_linear_scan_function_list;

	scan = (MX_SCAN *)(record->record_superclass_struct);

	scan->motor_is_independent_variable[0] = TRUE;
	scan->motor_is_independent_variable[1] = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_2theta_scan_compute_motor_positions( MX_SCAN *scan,
					MX_LINEAR_SCAN *linear_scan )
{
	const char fname[] = "mxs_2theta_scan_compute_motor_positions()";

	double *motor_position;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCAN scan structure pointer is NULL.");
	}

	if ( linear_scan == (MX_LINEAR_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LINEAR_SCAN structure pointer is NULL.");
	}

	if ( scan->num_motors != 2 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Number of motors for 2 theta scan is not 2.  Instead = %ld",
			scan->num_motors );
	}

	motor_position = scan->motor_position;

	if ( motor_position == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"motor_position array pointer is NULL." );
	}

	motor_position[0] = linear_scan->start_position[0]
	 + (linear_scan->step_size[0]) * (double)(linear_scan->step_number[0]);

	motor_position[1] = 2.0 * motor_position[0];

	return MX_SUCCESSFUL_RESULT;
}

