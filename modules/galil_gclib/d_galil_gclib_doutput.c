/*
 * Name:    d_galil_gclib_doutput.c
 *
 * Purpose: Galil controller uncommitted digital output device driver.
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
#include "mx_digital_output.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_galil_gclib_doutput_record_function_list = {
	NULL,
	mxd_galil_gclib_doutput_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_galil_gclib_doutput_digital_output_function_list = {
	mxd_galil_gclib_doutput_read,
	mxd_galil_gclib_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_galil_gclib_doutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_GALIL_GCLIB_DOUTPUT_STANDARD_FIELDS
};

long mxd_galil_gclib_doutput_num_record_fields
		= sizeof( mxd_galil_gclib_doutput_record_field_defaults )
		/ sizeof( mxd_galil_gclib_doutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_doutput_rfield_def_ptr
			= &mxd_galil_gclib_doutput_record_field_defaults[0];

/* ===== */

static mx_status_type
mxd_galil_gclib_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_GALIL_GCLIB_DOUTPUT **galil_gclib_doutput,
			MX_GALIL_GCLIB **galil_gclib,
			const char *calling_fname )
{
	static const char fname[] = "mxd_galil_gclib_doutput_get_pointers()";

	MX_GALIL_GCLIB_DOUTPUT *galil_gclib_doutput_ptr;
	MX_RECORD *galil_gclib_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	galil_gclib_doutput_ptr = (MX_GALIL_GCLIB_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( galil_gclib_doutput_ptr == (MX_GALIL_GCLIB_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_GALIL_GCLIB_DOUTPUT pointer for record '%s' is NULL.",
			doutput->record->name );
	}

	if ( galil_gclib_doutput != (MX_GALIL_GCLIB_DOUTPUT **) NULL ) {
		*galil_gclib_doutput = galil_gclib_doutput_ptr;
	}

	if ( galil_gclib != (MX_GALIL_GCLIB **) NULL ) {
		galil_gclib_record =
			galil_gclib_doutput_ptr->galil_gclib_record;

		if ( galil_gclib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The galil_gclib_record pointer for record '%s' "
			"is NULL.", doutput->record->name );
		}

		*galil_gclib = (MX_GALIL_GCLIB *)
				galil_gclib_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_galil_gclib_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output = NULL;
        MX_GALIL_GCLIB_DOUTPUT *galil_gclib_doutput = NULL;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
				malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        galil_gclib_doutput = (MX_GALIL_GCLIB_DOUTPUT *)
		malloc( sizeof(MX_GALIL_GCLIB_DOUTPUT) );

        if ( galil_gclib_doutput == (MX_GALIL_GCLIB_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_GALIL_GCLIB_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = galil_gclib_doutput;
        record->class_specific_function_list
			= &mxd_galil_gclib_doutput_digital_output_function_list;

        digital_output->record = record;
	galil_gclib_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_galil_gclib_doutput_read()";

	MX_GALIL_GCLIB_DOUTPUT *galil_gclib_doutput = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_doutput_get_pointers( doutput,
				&galil_gclib_doutput, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "MG @OUT[%lu]",
				galil_gclib_doutput->doutput_number );

	mx_status = mxi_galil_gclib_command( galil_gclib,
				command, response, sizeof(response) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_galil_gclib_doutput_write()";

	MX_GALIL_GCLIB_DOUTPUT *galil_gclib_doutput = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_doutput_get_pointers( doutput,
				&galil_gclib_doutput, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "OB %lu, %lu",
				galil_gclib_doutput->doutput_number,
				doutput->value );

	mx_status = mxi_galil_gclib_command( galil_gclib,
				command, response, sizeof(response) );

	return mx_status;
}

