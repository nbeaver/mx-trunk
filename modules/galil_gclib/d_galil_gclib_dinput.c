/*
 * Name:    d_galil_gclib_dinput.c
 *
 * Purpose: Galil controller uncommitted digital input device driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
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
#include "mx_digital_input.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_galil_gclib_dinput_record_function_list = {
	NULL,
	mxd_galil_gclib_dinput_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST
		mxd_galil_gclib_dinput_digital_input_function_list = {
	mxd_galil_gclib_dinput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_galil_gclib_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_GALIL_GCLIB_DINPUT_STANDARD_FIELDS
};

long mxd_galil_gclib_dinput_num_record_fields
		= sizeof( mxd_galil_gclib_dinput_record_field_defaults )
		/ sizeof( mxd_galil_gclib_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_dinput_rfield_def_ptr
			= &mxd_galil_gclib_dinput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_galil_gclib_dinput_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_GALIL_GCLIB_DINPUT **galil_gclib_dinput,
			MX_GALIL_GCLIB **galil_gclib,
			const char *calling_fname )
{
	static const char fname[] = "mxd_galil_gclib_dinput_get_pointers()";

	MX_GALIL_GCLIB_DINPUT *galil_gclib_dinput_ptr;
	MX_RECORD *galil_gclib_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	galil_gclib_dinput_ptr = (MX_GALIL_GCLIB_DINPUT *)
				dinput->record->record_type_struct;

	if ( galil_gclib_dinput_ptr == (MX_GALIL_GCLIB_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_GALIL_GCLIB_DINPUT pointer for record '%s' is NULL.",
			dinput->record->name );
	}

	if ( galil_gclib_dinput != (MX_GALIL_GCLIB_DINPUT **) NULL ) {
		*galil_gclib_dinput = galil_gclib_dinput_ptr;
	}

	if ( galil_gclib != (MX_GALIL_GCLIB **) NULL ) {
		galil_gclib_record =
			galil_gclib_dinput_ptr->galil_gclib_record;

		if ( galil_gclib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The galil_gclib_record pointer for record '%s' "
			"is NULL.", dinput->record->name );
		}

		*galil_gclib = (MX_GALIL_GCLIB *)
				galil_gclib_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_dinput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_galil_gclib_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input = NULL;
        MX_GALIL_GCLIB_DINPUT *galil_gclib_dinput = NULL;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        galil_gclib_dinput = (MX_GALIL_GCLIB_DINPUT *)
		malloc( sizeof(MX_GALIL_GCLIB_DINPUT) );

        if ( galil_gclib_dinput == (MX_GALIL_GCLIB_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_GALIL_GCLIB_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = galil_gclib_dinput;
        record->class_specific_function_list
			= &mxd_galil_gclib_dinput_digital_input_function_list;

        digital_input->record = record;
	galil_gclib_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_galil_gclib_dinput_read()";

	MX_GALIL_GCLIB_DINPUT *galil_gclib_dinput = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_dinput_get_pointers( dinput,
				&galil_gclib_dinput, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "MG @IN[%lu]",
				galil_gclib_dinput->dinput_number );

	mx_status = mxi_galil_gclib_command( galil_gclib,
				command, response, sizeof(response) );

	return mx_status;
}

