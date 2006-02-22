/*
 * Name:    d_pfcu_filter_summary.c
 *
 * Purpose: MX driver to control all of the filters in a single XIA PFCU
 *          module as a unit.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PFCU_FILTER_SUMMARY_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_pfcu.h"
#include "d_pfcu_filter_summary.h"

/* Initialize the doutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_pfcu_filter_summary_record_function_list = {
	NULL,
	mxd_pfcu_filter_summary_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_pfcu_filter_summary_digital_output_function_list =
{
	mxd_pfcu_filter_summary_read,
	mxd_pfcu_filter_summary_write
};

/* PFCU filter summary data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pfcu_filter_summary_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_PFCU_FILTER_SUMMARY_STANDARD_FIELDS
};

long mxd_pfcu_filter_summary_num_record_fields
		= sizeof( mxd_pfcu_filter_summary_rf_defaults )
		  / sizeof( mxd_pfcu_filter_summary_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pfcu_filter_summary_rfield_def_ptr
			= &mxd_pfcu_filter_summary_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pfcu_filter_summary_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_PFCU_FILTER_SUMMARY **pfcu_filter_summary,
				MX_PFCU **pfcu,
				const char *calling_fname )
{
	static const char fname[] = "mxd_pfcu_filter_summary_get_pointers()";

	MX_RECORD *record, *pfcu_record;
	MX_PFCU_FILTER_SUMMARY *pfcu_filter_summary_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = doutput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	pfcu_filter_summary_ptr = (MX_PFCU_FILTER_SUMMARY *)
					record->record_type_struct;

	if ( pfcu_filter_summary_ptr == (MX_PFCU_FILTER_SUMMARY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PFCU_FILTER_SUMMARY pointer for doutput '%s' is NULL.",
			record->name );
	}

	if ( pfcu_filter_summary != (MX_PFCU_FILTER_SUMMARY **) NULL ) {
		*pfcu_filter_summary = pfcu_filter_summary_ptr;
	}

	if ( pfcu != (MX_PFCU **) NULL ) {
		pfcu_record = pfcu_filter_summary_ptr->pfcu_record;

		if ( pfcu_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'pfcu_record' pointer for doutput '%s' is NULL.",
				record->name );
		}

		*pfcu = (MX_PFCU *) pfcu_record->record_type_struct;

		if ( *pfcu == (MX_PFCU *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PFCU pointer for PFCU record '%s' is NULL.",
				pfcu_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_pfcu_filter_summary_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_pfcu_filter_summary_create_record_structures()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_PFCU_FILTER_SUMMARY *pfcu_filter_summary;

	/* Allocate memory for the necessary structures. */

	doutput = (MX_DIGITAL_OUTPUT *) malloc( sizeof(MX_DIGITAL_OUTPUT) );

	if ( doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
	}

	pfcu_filter_summary = (MX_PFCU_FILTER_SUMMARY *)
				malloc( sizeof(MX_PFCU_FILTER_SUMMARY) );

	if ( pfcu_filter_summary == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PFCU_FILTER_SUMMARY structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = doutput;
	record->record_type_struct = pfcu_filter_summary;
	record->class_specific_function_list
		= &mxd_pfcu_filter_summary_digital_output_function_list;

	doutput->record = record;
	pfcu_filter_summary->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_filter_summary_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_pfcu_filter_summary_read()";

	MX_PFCU_FILTER_SUMMARY *pfcu_filter_summary;
	MX_PFCU *pfcu;
	char response[80];
	int i;
	unsigned long value;
	mx_status_type mx_status;

	mx_status = mxd_pfcu_filter_summary_get_pointers( doutput,
				&pfcu_filter_summary, &pfcu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pfcu_command( pfcu, pfcu_filter_summary->module_number,
					"F", response, sizeof(response),
					MXD_PFCU_FILTER_SUMMARY_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value = 0;

	for ( i = 0; i < 4; i++ ) {
		switch( response[i] ) {
		case '0':
			break;
		case '1':
			value |= ( 1 << i );
			break;
		case '2':
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"PFCU '%s' has detected an open circuit for position %d",
				pfcu->record->name, i+1 );
			break;
		case '3':
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"PFCU '%s' has detected a short circuit for position %d",
				pfcu->record->name, i+1 );
			break;
		default:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"PFCU '%s' has reported an illegal status '%c' "
			"for position %d in response '%s'.",
				pfcu->record->name, response[i], i+1, response);
			break;
		}
	}

	doutput->value = value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_filter_summary_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_pfcu_filter_summary_write()";

	MX_PFCU_FILTER_SUMMARY *pfcu_filter_summary;
	MX_PFCU *pfcu;
	char insert_command[80], remove_command[80];
	unsigned long value;
	mx_status_type mx_status;

	mx_status = mxd_pfcu_filter_summary_get_pointers( doutput,
				&pfcu_filter_summary, &pfcu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strcpy( insert_command, "I" );

	strcpy( remove_command, "R" );

	value = doutput->value;

	if ( value & 0x1 ) {
		strcat( insert_command, "1" );
	} else {
		strcat( remove_command, "1" );
	}

	if ( value & 0x2 ) {
		strcat( insert_command, "2" );
	} else {
		strcat( remove_command, "2" );
	}

	if ( value & 0x4 ) {
		strcat( insert_command, "3" );
	} else {
		strcat( remove_command, "3" );
	}

	if ( value & 0x8 ) {
		strcat( insert_command, "4" );
	} else {
		strcat( remove_command, "4" );
	}

	/* Send the insert command first so that we avoid the possibility of
	 * putting too much beam on the sample.  This command is only sent
	 * if at least one filter is to be inserted.  This is because the
	 * PFCU controller complains if an 'I' command is sent without any 
	 * numerical values after it.
	 */

	if ( strcmp( insert_command, "I" ) != 0 ) {
		mx_status = mxi_pfcu_command( pfcu,
					pfcu_filter_summary->module_number,
					insert_command, NULL, 0,
					MXD_PFCU_FILTER_SUMMARY_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* If the insert succeeded, send the remove command.  As above with
	 * the insert command, we do not send the command if no filters are
	 * to be removed.
	 */

	if ( strcmp( remove_command, "R" ) != 0 ) {
		mx_status = mxi_pfcu_command( pfcu,
					pfcu_filter_summary->module_number,
					remove_command, NULL, 0,
					MXD_PFCU_FILTER_SUMMARY_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

