/*
 * Name:    d_galil_gclib_ainput.c
 *
 * Purpose: MX driver for Galil gclib controller analog input.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib_ainput.h"

MX_RECORD_FUNCTION_LIST mxd_galil_gclib_ainput_record_function_list = {
	NULL,
	mxd_galil_gclib_ainput_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_galil_gclib_ainput_open
};

MX_ANALOG_INPUT_FUNCTION_LIST
  mxd_galil_gclib_ainput_analog_input_function_list = {
	mxd_galil_gclib_ainput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_galil_gclib_ainput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_GALIL_GCLIB_AINPUT_STANDARD_FIELDS
};

long mxd_galil_gclib_ainput_num_record_fields
		= sizeof( mxd_galil_gclib_ainput_record_field_defaults )
		/ sizeof( mxd_galil_gclib_ainput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_ainput_rfield_def_ptr
			= &mxd_galil_gclib_ainput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_galil_gclib_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_GALIL_GCLIB_AINPUT **galil_gclib_ainput,
			MX_GALIL_GCLIB **galil_gclib,
			const char *calling_fname )
{
	static const char fname[] = "mxd_galil_gclib_ainput_get_pointers()";

	MX_GALIL_GCLIB_AINPUT *galil_gclib_ainput_ptr;
	MX_RECORD *galil_gclib_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	galil_gclib_ainput_ptr = (MX_GALIL_GCLIB_AINPUT *)
				ainput->record->record_type_struct;

	if ( galil_gclib_ainput_ptr == (MX_GALIL_GCLIB_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_GALIL_GCLIB_AINPUT pointer for record '%s' is NULL.",
			ainput->record->name );
	}

	if ( galil_gclib_ainput != (MX_GALIL_GCLIB_AINPUT **) NULL ) {
		*galil_gclib_ainput = galil_gclib_ainput_ptr;
	}

	if ( galil_gclib != (MX_GALIL_GCLIB **) NULL ) {
		galil_gclib_record =
			galil_gclib_ainput_ptr->galil_gclib_record;

		if ( galil_gclib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The galil_gclib_record pointer for record '%s' "
			"is NULL.", ainput->record->name );
		}

		*galil_gclib = (MX_GALIL_GCLIB *)
				galil_gclib_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_ainput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_galil_gclib_ainput_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_GALIL_GCLIB_AINPUT *galil_gclib_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_ANALOG_INPUT structure." );
        }

        galil_gclib_ainput = (MX_GALIL_GCLIB_AINPUT *)
				malloc( sizeof(MX_GALIL_GCLIB_AINPUT) );

        if ( galil_gclib_ainput == (MX_GALIL_GCLIB_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_GALIL_GCLIB_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = galil_gclib_ainput;
        record->class_specific_function_list
			= &mxd_galil_gclib_ainput_analog_input_function_list;

        analog_input->record = record;
	galil_gclib_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_galil_gclib_ainput_read()";

	MX_ANALOG_INPUT *ainput = NULL;
	MX_GALIL_GCLIB_AINPUT *galil_gclib_ainput = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_galil_gclib_ainput_get_pointers( ainput,
				&galil_gclib_ainput, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure the type of the analog input using the AQ command. */

	snprintf( command, sizeof(command), "AQ %lu,%lu",
				galil_gclib_ainput->ainput_number,
				galil_gclib_ainput->ainput_type );

	mx_status = mxi_galil_gclib_command( galil_gclib,
				command, response, sizeof(response) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_galil_gclib_ainput_read()";

	MX_GALIL_GCLIB_AINPUT *galil_gclib_ainput = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_ainput_get_pointers( ainput,
				&galil_gclib_ainput, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "MG @AN[%lu]",
				galil_gclib_ainput->ainput_number );

	mx_status = mxi_galil_gclib_command( galil_gclib,
				command, response, sizeof(response) );

	return mx_status;
}

