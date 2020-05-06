/*
 * Name:    d_galil_gclib_aoutput.c
 *
 * Purpose: MX driver for Galil gclib controller analog output.
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
#include "mx_analog_output.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib_aoutput.h"

MX_RECORD_FUNCTION_LIST mxd_galil_gclib_aoutput_record_function_list = {
	NULL,
	mxd_galil_gclib_aoutput_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
  mxd_galil_gclib_aoutput_analog_output_function_list = {
	mxd_galil_gclib_aoutput_read,
	mxd_galil_gclib_aoutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_galil_gclib_aoutput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_GALIL_GCLIB_AOUTPUT_STANDARD_FIELDS
};

long mxd_galil_gclib_aoutput_num_record_fields
		= sizeof( mxd_galil_gclib_aoutput_record_field_defaults )
		/ sizeof( mxd_galil_gclib_aoutput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_aoutput_rfield_def_ptr
			= &mxd_galil_gclib_aoutput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_galil_gclib_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_GALIL_GCLIB_AOUTPUT **galil_gclib_aoutput,
			MX_GALIL_GCLIB **galil_gclib,
			const char *calling_fname )
{
	static const char fname[] = "mxd_galil_gclib_aoutput_get_pointers()";

	MX_GALIL_GCLIB_AOUTPUT *galil_gclib_aoutput_ptr;
	MX_RECORD *galil_gclib_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	galil_gclib_aoutput_ptr = (MX_GALIL_GCLIB_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( galil_gclib_aoutput_ptr == (MX_GALIL_GCLIB_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_GALIL_GCLIB_AOUTPUT pointer for record '%s' is NULL.",
			aoutput->record->name );
	}

	if ( galil_gclib_aoutput != (MX_GALIL_GCLIB_AOUTPUT **) NULL ) {
		*galil_gclib_aoutput = galil_gclib_aoutput_ptr;
	}

	if ( galil_gclib != (MX_GALIL_GCLIB **) NULL ) {
		galil_gclib_record =
			galil_gclib_aoutput_ptr->galil_gclib_record;

		if ( galil_gclib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The galil_gclib_record pointer for record '%s' "
			"is NULL.", aoutput->record->name );
		}

		*galil_gclib = (MX_GALIL_GCLIB *)
				galil_gclib_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_aoutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_galil_gclib_aoutput_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_GALIL_GCLIB_AOUTPUT *galil_gclib_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        galil_gclib_aoutput = (MX_GALIL_GCLIB_AOUTPUT *)
				malloc( sizeof(MX_GALIL_GCLIB_AOUTPUT) );

        if ( galil_gclib_aoutput == (MX_GALIL_GCLIB_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_GALIL_GCLIB_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = galil_gclib_aoutput;
        record->class_specific_function_list
			= &mxd_galil_gclib_aoutput_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_aoutput_read( MX_ANALOG_OUTPUT *aoutput )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_galil_gclib_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_galil_gclib_aoutput_write()";

	MX_GALIL_GCLIB_AOUTPUT *galil_gclib_aoutput = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_aoutput_get_pointers( aoutput,
				&galil_gclib_aoutput, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "AO %s,%g",
				galil_gclib_aoutput->aoutput_name,
				aoutput->value );

	mx_status = mxi_galil_gclib_command( galil_gclib,
				command, response, sizeof(response) );

	return mx_status;
}

