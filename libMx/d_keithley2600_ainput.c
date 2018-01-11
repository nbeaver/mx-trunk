/*
 * Name:    d_keithley2600_ainput.c
 *
 * Purpose: MX driver to control Keithley 2600 series analog inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY2600_AINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_keithley.h"
#include "i_keithley2600.h"
#include "d_keithley2600_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley2600_ainput_record_function_list = {
	NULL,
	mxd_keithley2600_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_keithley2600_ainput_open
};

MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_keithley2600_ainput_analog_input_function_list =
{
	mxd_keithley2600_ainput_read
};

/* Keithley 2600 analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley2600_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_KEITHLEY2600_AINPUT_STANDARD_FIELDS
};

long mxd_keithley2600_ainput_num_record_fields
		= sizeof( mxd_keithley2600_ainput_rf_defaults )
		  / sizeof( mxd_keithley2600_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley2600_ainput_rfield_def_ptr
			= &mxd_keithley2600_ainput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley2600_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_KEITHLEY2600_AINPUT **keithley2600_ainput,
				MX_KEITHLEY2600 **keithley2600,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley2600_ainput_get_pointers()";

	MX_RECORD *record, *controller_record;
	MX_KEITHLEY2600_AINPUT *keithley2600_ainput_ptr;
	MX_KEITHLEY2600 *keithley2600_ptr;

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

	keithley2600_ainput_ptr = (MX_KEITHLEY2600_AINPUT *)
					record->record_type_struct;

	if ( keithley2600_ainput_ptr == (MX_KEITHLEY2600_AINPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2600_AINPUT pointer for ainput '%s' is NULL.",
			record->name );
	}

	if ( keithley2600_ainput != (MX_KEITHLEY2600_AINPUT **) NULL ) {
		*keithley2600_ainput = keithley2600_ainput_ptr;
	}

	controller_record = keithley2600_ainput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'controller_record' pointer for ainput '%s' is NULL.",
				record->name );
	}

	keithley2600_ptr = (MX_KEITHLEY2600 *)
					controller_record->record_type_struct;

	if ( keithley2600_ptr == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_KEITHLEY2600 pointer for controller record '%s' is NULL.",
				controller_record->name );
	}

	if ( keithley2600 != (MX_KEITHLEY2600 **) NULL ) {
		*keithley2600 = keithley2600_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_keithley2600_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_keithley2600_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY2600_AINPUT *keithley2600_ainput;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	keithley2600_ainput = (MX_KEITHLEY2600_AINPUT *)
				malloc( sizeof(MX_KEITHLEY2600_AINPUT) );

	if ( keithley2600_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY2600_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = keithley2600_ainput;
	record->class_specific_function_list
			= &mxd_keithley2600_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley2600_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley2600_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY2600_AINPUT *keithley2600_ainput;
	MX_KEITHLEY2600 *keithley2600;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley2600_ainput_get_pointers( ainput,
		&keithley2600_ainput, &keithley2600, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( islower( keithley2600_ainput->channel_name ) ) {
		keithley2600_ainput->channel_name
			= toupper( keithley2600_ainput->channel_name );
	}

	switch( keithley2600_ainput->channel_name ) {
	case 'A':
	    keithley2600_ainput->channel_number = 1;
	    break;
	case 'B':
	    keithley2600_ainput->channel_number = 2;
	    break;
	default:
	    return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Illegal channel name '%c' specified for analog input '%s'",
	    	keithley2600_ainput->channel_name, record->name );
	    break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley2600_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_keithley2600_ainput_read()";

	MX_KEITHLEY2600_AINPUT *keithley2600_ainput;
	MX_KEITHLEY2600 *keithley2600;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_keithley2600_ainput_get_pointers( ainput,
		&keithley2600_ainput, &keithley2600, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_keithley2600_command( keithley2600,
					"smua.measure.interval = 0.1",
				NULL, 0, keithley2600->keithley2600_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"print(smua.measure.%ld())",
		keithley2600_ainput->channel_number );

	mx_status = mxi_keithley2600_command( keithley2600, command,
				response, sizeof(response),
				keithley2600->keithley2600_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

