/*
 * Name:    i_wti_nps.c
 *
 * Purpose: MX interface driver for Western Telematic Inc. Network
 *          Power Switchs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_WTI_NPS_DEBUG		FALSE

#define MXI_WTI_NPS_DEBUG_RAW	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_wti_nps.h"

MX_RECORD_FUNCTION_LIST mxi_wti_nps_record_function_list = {
	NULL,
	mxi_wti_nps_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_wti_nps_open
};

MX_RECORD_FIELD_DEFAULTS mxi_wti_nps_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_WTI_NPS_STANDARD_FIELDS
};

long mxi_wti_nps_num_record_fields
	= sizeof( mxi_wti_nps_rf_defaults )
		    / sizeof( mxi_wti_nps_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxi_wti_nps_rfield_def_ptr
			= &mxi_wti_nps_rf_defaults[0];

MX_EXPORT mx_status_type
mxi_wti_nps_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_wti_nps_create_record_structures()";

	MX_WTI_NPS *wti_nps;

	/* Allocate memory for the necessary structures. */

	wti_nps = (MX_WTI_NPS *) malloc( sizeof(MX_WTI_NPS) );

	if ( wti_nps == (MX_WTI_NPS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_WTI_NPS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = wti_nps;

	record->record_function_list = &mxi_wti_nps_record_function_list;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	wti_nps->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_wti_nps_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_wti_nps_open()";

	MX_WTI_NPS *wti_nps;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL.");
	}

	wti_nps = (MX_WTI_NPS *) record->record_type_struct;

	if ( wti_nps == (MX_WTI_NPS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WTI_NPS pointer for record '%s' is NULL.",
			record->name);
	}

	/* Throw away any unread characters. */

	mx_status = mx_rs232_discard_unread_input( wti_nps->rs232_record,
						MXI_WTI_NPS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

