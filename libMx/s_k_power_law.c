/*
 * Name:    s_k_power_law.c
 *
 * Purpose: Implements xafs wavenumber scans for which the counting
 *          time follows a power law.
 *
 *                                      [ (    k    )                        ]
 *          counting_time = base_time * [ ( ------- ) ^ (power_law_exponent) ]
 *                                      [ ( k_start )                        ]
 *
 *          In C code, this can be written as
 *
 *            counting_time = base_time * pow((k/k_start), power_law_exponent);
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2015 Illinois Institute of Technology
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
#include "m_time.h"
#include "s_k_power_law.h"

MX_LINEAR_SCAN_FUNCTION_LIST mxs_k_power_law_linear_scan_function_list = {
	mxs_k_power_law_scan_create_record_structures,
	mxs_k_power_law_scan_finish_record_initialization,
};

MX_RECORD_FIELD_DEFAULTS mxs_k_power_law_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_LINEAR_SCAN_STANDARD_FIELDS
};

long mxs_k_power_law_scan_num_record_fields
			= sizeof( mxs_k_power_law_scan_defaults )
			/ sizeof( mxs_k_power_law_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_k_power_law_scan_def_ptr
			= &mxs_k_power_law_scan_defaults[0];

static mx_status_type
mxs_k_power_law_scan_acquire_data( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mxs_k_power_law_scan_acquire_data()";

	MX_SCAN *scan;
	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	mx_status_type mx_status;

	scan = (MX_SCAN *) measurement->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCAN pointer for measurement pointer %p is NULL.",
			measurement );
	}

	MX_DEBUG(-2,("%s invoked for scan '%s'", fname, scan->record->name ));

	preset_time_struct = (MX_MEASUREMENT_PRESET_TIME *)
				measurement->measurement_type_struct;

	if ( preset_time_struct == (MX_MEASUREMENT_PRESET_TIME *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MEASUREMENT_PRESET_TIME pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	MX_DEBUG(-2,("%s: integration time = %f",
		fname, preset_time_struct->integration_time ));

	mx_status = mx_acquire_data( measurement );

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_k_power_law_scan_create_record_structures( MX_RECORD *record,
						MX_SCAN *scan,
						MX_LINEAR_SCAN *linear_scan )
{
	MX_K_POWER_LAW_SCAN *k_power_law_scan = NULL;

	k_power_law_scan = (MX_K_POWER_LAW_SCAN *)
				malloc( sizeof(MX_K_POWER_LAW_SCAN) );

	record->record_type_struct = k_power_law_scan;
	linear_scan->record_type_struct = k_power_law_scan;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_k_power_law_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_k_power_law_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_SCAN_FUNCTION_LIST *flist;
	long i;

	/* FIXME: We need to verify that the scan record is set up 
	 * correctly to use preset time measurements.
	 */

	record->class_specific_function_list
				= &mxs_k_power_law_linear_scan_function_list;

	scan = (MX_SCAN *) record->record_superclass_struct;

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;
	}

	/* Set up a special acquire_data function for this scan type. */

	flist = (MX_SCAN_FUNCTION_LIST *)
			record->superclass_specific_function_list;

	if ( flist == (MX_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Scan function list pointer for scan '%s' is NULL.",
			record->name );
	}

	flist->acquire_data = mxs_k_power_law_scan_acquire_data;

	return MX_SUCCESSFUL_RESULT;
}

