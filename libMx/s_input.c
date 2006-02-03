/*
 * Name:    s_input.c
 *
 * Purpose: Scan description file for input device linear scan.
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

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "s_input.h"

/* Initialize the input device scan driver jump table. */

MX_LINEAR_SCAN_FUNCTION_LIST mxs_input_linear_scan_function_list = {
	mxs_input_scan_create_record_structures,
	mxs_input_scan_finish_record_initialization,
	NULL,
	NULL,
	mxs_input_scan_compute_motor_positions,
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxs_input_linear_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_LINEAR_SCAN_STANDARD_FIELDS
};

mx_length_type mxs_input_linear_scan_num_record_fields
			= sizeof( mxs_input_linear_scan_defaults )
			/ sizeof( mxs_input_linear_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_input_linear_scan_def_ptr
			= &mxs_input_linear_scan_defaults[0];

MX_EXPORT mx_status_type
mxs_input_scan_create_record_structures( MX_RECORD *record,
		MX_SCAN *scan, MX_LINEAR_SCAN *linear_scan )
{
	record->record_type_struct = NULL;
	linear_scan->record_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_input_scan_finish_record_initialization( MX_RECORD *record )
{
	record->class_specific_function_list
				= &mxs_input_linear_scan_function_list;

	record->class_specific_function_list
				= &mxs_input_linear_scan_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_input_scan_compute_motor_positions( MX_SCAN *scan,
					MX_LINEAR_SCAN *linear_scan )
{
	static const char fname[] = "mxs_input_scan_compute_motor_positions()";

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCAN structure pointer is NULL.");
	}

	if ( scan->num_motors != 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Number of motors for input device scan is not zero.  Instead = %ld",
			(long) scan->num_motors );
	}

	/* Nothing to compute for a input device scan. */

	return MX_SUCCESSFUL_RESULT;
}

