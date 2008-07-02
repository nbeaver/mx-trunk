/*
 * Name:    sa_wedge.c
 *
 * Purpose: Scan description file for area detector wedge scans.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_scan_area_detector.h"
#include "sa_wedge.h"

MX_AREA_DETECTOR_SCAN_FUNCTION_LIST
			mxs_wedge_area_detector_scan_function_list = {
	mxs_wedge_scan_create_record_structures,
	mxs_wedge_scan_finish_record_initialization,
	NULL,
	mxs_wedge_scan_execute_scan_body
};

MX_RECORD_FIELD_DEFAULTS mxs_wedge_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_AREA_DETECTOR_SCAN_STANDARD_FIELDS,
	MXS_WEDGE_SCAN_STANDARD_FIELDS
};

long mxs_wedge_scan_num_record_fields = sizeof( mxs_wedge_scan_defaults )
					/ sizeof( mxs_wedge_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_wedge_scan_def_ptr
			= &mxs_wedge_scan_defaults[0];


MX_EXPORT mx_status_type
mxs_wedge_scan_create_record_structures( MX_RECORD *record,
					MX_SCAN *scan,
					MX_AREA_DETECTOR_SCAN *ad_scan )
{
	static const char fname[] = "mxs_wedge_scan_create_record_structures()";

	MX_WEDGE_SCAN *wedge_scan;

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, record->name));

	wedge_scan = (MX_WEDGE_SCAN *) malloc( sizeof(MX_WEDGE_SCAN) );

	if ( wedge_scan == (MX_WEDGE_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate an MX_WEDGE_SCAN structure." );
	}

	record->record_type_struct = wedge_scan;
	ad_scan->record_type_struct = wedge_scan;

	record->class_specific_function_list =
			&mxs_wedge_area_detector_scan_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_wedge_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_wedge_scan_finish_record_initialization()";

	MX_SCAN *scan;
	long i;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, record->name));

	scan = (MX_SCAN *) record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCAN pointer for record '%s' is NULL.", record->name );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_wedge_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] = "mxs_wedge_scan_execute_scan_body()";

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name));

	return MX_SUCCESSFUL_RESULT;
}

