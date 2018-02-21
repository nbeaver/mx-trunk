/*
 * Name:    d_keithley199_ainput.c
 *
 * Purpose: MX driver to read measurements from a Newport Electronics
 *          KEITHLEY199_AINPUT analog input.
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

#define KEITHLEY199_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_keithley199.h"
#include "d_keithley199_ainput.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley199_ainput_record_function_list = {
	NULL,
	mxd_keithley199_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_keithley199_ainput_open,
	mxd_keithley199_ainput_close
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_keithley199_ainput_analog_input_function_list =
{
	mxd_keithley199_ainput_read
};

/* KEITHLEY199_AINPUT analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley199_ainput_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_KEITHLEY199_AINPUT_STANDARD_FIELDS
};

long mxd_keithley199_ainput_num_record_fields
		= sizeof( mxd_keithley199_ainput_rf_defaults )
		  / sizeof( mxd_keithley199_ainput_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley199_ainput_rfield_def_ptr
			= &mxd_keithley199_ainput_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley199_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_KEITHLEY199_AINPUT **keithley199_ainput,
				MX_KEITHLEY199 **keithley199,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley199_ainput_get_pointers()";

	MX_RECORD *record = NULL;
	MX_RECORD *keithley199_record = NULL;
	MX_KEITHLEY199_AINPUT *keithley199_ainput_ptr = NULL;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = ainput->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_ANALOG_INPUT pointer %p "
		"passed was NULL.", ainput );
	}

	keithley199_ainput_ptr = (MX_KEITHLEY199_AINPUT *)
						record->record_type_struct;

	if ( keithley199_ainput_ptr == (MX_KEITHLEY199_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KEITHLEY199_AINPUT pointer for ainput '%s' is NULL.",
			record->name );
	}

	if ( keithley199_ainput != (MX_KEITHLEY199_AINPUT **) NULL ) {
		*keithley199_ainput = keithley199_ainput_ptr;
	}

	if ( keithley199 == (MX_KEITHLEY199 **) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	} 

	keithley199_record = keithley199_ainput_ptr->keithley199_record;

	if ( keithley199_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The keithley199_record pointer for analog input '%s' is NULL.",
			record->name );
	}

	*keithley199 = (MX_KEITHLEY199 *)
				keithley199_record->record_type_struct;

	if ( (*keithley199) == (MX_KEITHLEY199 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KEITHLEY199 pointer for analog input '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_keithley199_ainput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_keithley199_ainput_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY199_AINPUT *keithley199_ainput = NULL;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	keithley199_ainput = (MX_KEITHLEY199_AINPUT *)
				malloc( sizeof(MX_KEITHLEY199_AINPUT) );

	if ( keithley199_ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_KEITHLEY199_AINPUT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = keithley199_ainput;
	record->class_specific_function_list
			= &mxd_keithley199_ainput_analog_input_function_list;

	ainput->record = record;

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley199_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley199_ainput_open()";

	MX_ANALOG_INPUT *ainput = NULL;
	MX_KEITHLEY199_AINPUT *keithley199_ainput = NULL;
	MX_KEITHLEY199 *keithley199 = NULL;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley199_ainput_get_pointers( ainput,
				&keithley199_ainput, &keithley199, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( keithley199_ainput->keithley199_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the Keithley 199 record "
		"used by KEITHLEY199_AINPUT record '%s' is NULL.",
			record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley199_ainput_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley199_ainput_close()";

	MX_ANALOG_INPUT *ainput;
	MX_KEITHLEY199_AINPUT *keithley199_ainput = NULL;
	MX_KEITHLEY199 *keithley199 = NULL;
	mx_status_type mx_status;

	ainput = (MX_ANALOG_INPUT *) (record->record_class_struct);

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_INPUT pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley199_ainput_get_pointers( ainput,
				&keithley199_ainput, &keithley199, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley199_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_keithley199_ainput_read()";

	MX_KEITHLEY199_AINPUT *keithley199_ainput = NULL;
	MX_KEITHLEY199 *keithley199 = NULL;
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_keithley199_ainput_get_pointers( ainput,
				&keithley199_ainput, &keithley199, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we read from the Keithley 199 without having sent a command
	 * first, then we get a sensor reading.
	 */

	mx_status = mxi_keithley199_command( keithley199, NULL,
						response, sizeof(response),
						KEITHLEY199_AINPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Skip over the first 4 characters and then parse the sensor value. */

	num_items = sscanf( response + 4, "%lg",
			&(ainput->raw_value.double_value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to parse the analog input value from the "
		"message '%s' send by record '%s' failed.",
			response, ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

