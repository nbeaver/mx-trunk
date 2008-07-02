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

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_scan_area_detector.h"
#include "sa_wedge.h"

MX_AREA_DETECTOR_SCAN_FUNCTION_LIST
			mxs_wedge_area_detector_scan_function_list = {
	mxs_wedge_scan_create_record_structures,
	mxs_wedge_scan_finish_record_initialization
};

MX_RECORD_FIELD_DEFAULTS mxs_wedge_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_AREA_DETECTOR_SCAN_STANDARD_FIELDS
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

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_wedge_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_wedge_scan_finish_record_initialization()";

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

