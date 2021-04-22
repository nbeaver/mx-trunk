/*
 * Name:    d_umx_dinput.c
 *
 * Purpose: MX digital input driver for UMX-based microcontrollers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019-2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define UMX_DINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_umx.h"
#include "d_umx_dinput.h"

/* Initialize the dinput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_umx_dinput_record_function_list = {
	NULL,
	mxd_umx_dinput_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST
		mxd_umx_dinput_digital_input_function_list =
{
	mxd_umx_dinput_read
};

/* UMX digital input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_umx_dinput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_UMX_DINPUT_STANDARD_FIELDS
};

long mxd_umx_dinput_num_record_fields
		= sizeof( mxd_umx_dinput_rf_defaults )
		  / sizeof( mxd_umx_dinput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_umx_dinput_rfield_def_ptr
			= &mxd_umx_dinput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_umx_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
				MX_UMX_DINPUT **umx_dinput,
				MX_RECORD **umx_record,
				const char *calling_fname )
{
	static const char fname[] = "mxd_umx_dinput_get_pointers()";

	MX_RECORD *record = NULL;
	MX_UMX_DINPUT *umx_dinput_ptr;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = dinput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_DIGITAL_INPUT pointer passed was NULL." );
	}

	umx_dinput_ptr = (MX_UMX_DINPUT *)
					record->record_type_struct;

	if ( umx_dinput_ptr == (MX_UMX_DINPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_UMX_DINPUT pointer for digital input '%s' is NULL.",
			record->name );
	}

	if ( umx_dinput != (MX_UMX_DINPUT **) NULL ) {
		*umx_dinput = umx_dinput_ptr;
	}

	if ( umx_record != (MX_RECORD **) NULL ) {
		*umx_record = umx_dinput_ptr->umx_record;

		if ( (*umx_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The 'umx_record' pointer for digital input '%s' is NULL.",
								record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_umx_dinput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_umx_dinput_create_record_structures()";

	MX_DIGITAL_INPUT *dinput;
	MX_UMX_DINPUT *umx_dinput;

	/* Allocate memory for the necessary structures. */

	dinput = (MX_DIGITAL_INPUT *) malloc( sizeof(MX_DIGITAL_INPUT) );

	if ( dinput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DIGITAL_INPUT structure." );
	}

	umx_dinput = (MX_UMX_DINPUT *)
				malloc( sizeof(MX_UMX_DINPUT) );

	if ( umx_dinput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_UMX_DINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = dinput;
	record->record_type_struct = umx_dinput;
	record->class_specific_function_list
		= &mxd_umx_dinput_digital_input_function_list;

	dinput->record = record;
	umx_dinput->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_umx_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_umx_dinput_read()";

	MX_UMX_DINPUT *umx_dinput = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[80];
	int num_items;
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	mx_status = mxd_umx_dinput_get_pointers( dinput,
				&umx_dinput, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	debug_flag = FALSE;

	snprintf( command, sizeof(command),
		"GET %s.value", umx_dinput->dinput_name );

	mx_status = mx_umx_command( umx_record, command,
				response, sizeof(response),
				debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "$%lu", &(dinput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not see the status of digital input '%s' in the "
		"response '%s' to command '%s'.",
			dinput->record->name, response, command );
	}

	return MX_SUCCESSFUL_RESULT;
}

