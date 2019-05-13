/*
 * Name:    d_umx_doutput.c
 *
 * Purpose: MX driver for UMX digital outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define UMX_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "mx_umx.h"
#include "d_umx_doutput.h"

/* Initialize the doutput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_umx_doutput_record_function_list = {
	NULL,
	mxd_umx_doutput_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_umx_doutput_digital_output_function_list =
{
	NULL,
	mxd_umx_doutput_write
};

/* Numato GPIO digital output data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_umx_doutput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_UMX_DOUTPUT_STANDARD_FIELDS
};

long mxd_umx_doutput_num_record_fields
		= sizeof( mxd_umx_doutput_rf_defaults )
		  / sizeof( mxd_umx_doutput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_umx_doutput_rfield_def_ptr
			= &mxd_umx_doutput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_umx_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_UMX_DOUTPUT **umx_doutput,
				MX_RECORD **umx_record,
				const char *calling_fname )
{
	static const char fname[] = "mxd_umx_doutput_get_pointers()";

	MX_RECORD *record = NULL;
	MX_UMX_DOUTPUT *umx_doutput_ptr;

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

	umx_doutput_ptr = (MX_UMX_DOUTPUT *) record->record_type_struct;

	if ( umx_doutput_ptr == (MX_UMX_DOUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_UMX_DOUTPUT pointer for digital output '%s' is NULL.",
			record->name );
	}

	if ( umx_doutput != (MX_UMX_DOUTPUT **) NULL ) {
		*umx_doutput = umx_doutput_ptr;
	}

	if ( umx_record != (MX_RECORD **) NULL ) {
		*umx_record = umx_doutput_ptr->umx_record;

		if ( (*umx_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The 'umx_record' pointer for digital output '%s' is NULL.",
								record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_umx_doutput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_umx_doutput_create_record_structures()";

	MX_DIGITAL_OUTPUT *doutput;
	MX_UMX_DOUTPUT *umx_doutput;

	/* Allocate memory for the necessary structures. */

	doutput = (MX_DIGITAL_OUTPUT *) malloc( sizeof(MX_DIGITAL_OUTPUT) );

	if ( doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
	}

	umx_doutput = (MX_UMX_DOUTPUT *)
				malloc( sizeof(MX_UMX_DOUTPUT) );

	if ( umx_doutput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_UMX_DOUTPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = doutput;
	record->record_type_struct = umx_doutput;
	record->class_specific_function_list
		= &mxd_umx_doutput_digital_output_function_list;

	doutput->record = record;
	umx_doutput->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_umx_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_umx_doutput_write()";

	MX_UMX_DOUTPUT *umx_doutput = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[80];
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	mx_status = mxd_umx_doutput_get_pointers( doutput,
				&umx_doutput, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	debug_flag = TRUE;

	snprintf( command, sizeof(command),
		"PUT %s.VAL %ld",
		umx_doutput->doutput_name,
		doutput->value );

	mx_status = mx_umx_command( umx_record, command,
				response, sizeof(response),
				debug_flag );

	return mx_status;
}

