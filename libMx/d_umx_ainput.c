/*
 * Name:    d_umx_ainput.c
 *
 * Purpose: MX analog input driver for UMX-based microcontrollers.
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

#define UMX_AINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_umx.h"
#include "d_umx_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_umx_ainput_record_function_list = {
	NULL,
	mxd_umx_ainput_create_record_structures
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_umx_ainput_analog_input_function_list =
{
	mxd_umx_ainput_read
};

/* Keithley 2600 analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_umx_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_UMX_AINPUT_STANDARD_FIELDS
};

long mxd_umx_ainput_num_record_fields
		= sizeof( mxd_umx_ainput_rf_defaults )
		  / sizeof( mxd_umx_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_umx_ainput_rfield_def_ptr
			= &mxd_umx_ainput_rf_defaults[0];

/*----*/

/* A private function for the use of the driver. */

static mx_status_type
mxd_umx_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_UMX_AINPUT **umx_ainput,
				MX_RECORD **umx_record,
				const char *calling_fname )
{
	static const char fname[] = "mxd_umx_ainput_get_pointers()";

	MX_RECORD *record = NULL;
	MX_UMX_AINPUT *umx_ainput_ptr = NULL;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = ainput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	umx_ainput_ptr = (MX_UMX_AINPUT *) record->record_type_struct;

	if ( umx_ainput_ptr == (MX_UMX_AINPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_UMX_AINPUT pointer for ainput '%s' is NULL.",
			record->name );
	}

	if ( umx_ainput != (MX_UMX_AINPUT **) NULL ) {
		*umx_ainput = umx_ainput_ptr;
	}

	if ( umx_record != (MX_RECORD **) NULL ) {
		*umx_record = umx_ainput_ptr->umx_record;

		if ( (*umx_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The 'umx_record' pointer for ainput '%s' is NULL.",
					record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_umx_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_umx_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_UMX_AINPUT *umx_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	umx_ainput = (MX_UMX_AINPUT *)
				malloc( sizeof(MX_UMX_AINPUT) );

	if ( umx_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_UMX_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = umx_ainput;
	record->class_specific_function_list
			= &mxd_umx_ainput_analog_input_function_list;

	ainput->record = record;
	umx_ainput->record = record;

	ainput->subclass = MXT_AIN_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_umx_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_umx_ainput_read()";

	MX_UMX_AINPUT *umx_ainput = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[80];
	int num_items;
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	mx_status = mxd_umx_ainput_get_pointers( ainput,
		&umx_ainput, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	debug_flag = TRUE;

	snprintf( command, sizeof(command),
		"GET %s.VAL", umx_ainput->ainput_name );

	mx_status = mx_umx_command( umx_record, command,
				response, sizeof(response),
				debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "$%ld", &(ainput->raw_value.long_value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not see the status of analog input '%s' in the "
		"response '%s' to command '%s'.",
			ainput->record->name, response, command );
	}

	return MX_SUCCESSFUL_RESULT;
}

