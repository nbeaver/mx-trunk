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
	mxs_mcs_quick_scan_finish_record_initialization,
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

MX_EXPORT mx_status_type
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

	return MX_SUCCESSFUL_RESULT;
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

