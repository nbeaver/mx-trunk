/*
 * Name:    d_picomotor_aio.c
 *
 * Purpose: MX input drivers to control Picomotor analog input ports.
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

#define MXD_PICOMOTOR_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "i_picomotor.h"
#include "d_picomotor_aio.h"

/* Initialize the PICOMOTOR analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_picomotor_ain_record_function_list = {
	NULL,
	mxd_picomotor_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_picomotor_ain_analog_input_function_list = {
	mxd_picomotor_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_picomotor_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_PICOMOTOR_AINPUT_STANDARD_FIELDS
};

long mxd_picomotor_ain_num_record_fields
		= sizeof( mxd_picomotor_ain_record_field_defaults )
			/ sizeof( mxd_picomotor_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_ain_rfield_def_ptr
			= &mxd_picomotor_ain_record_field_defaults[0];

/* === */

static mx_status_type
mxd_picomotor_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_PICOMOTOR_AINPUT **picomotor_ainput,
			MX_PICOMOTOR_CONTROLLER **picomotor_controller,
			const char *calling_fname )
{
	static const char fname[] = "mxd_picomotor_ain_get_pointers()";

	MX_RECORD *picomotor_controller_record;
	MX_PICOMOTOR_AINPUT *picomotor_ainput_pointer;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	picomotor_ainput_pointer = (MX_PICOMOTOR_AINPUT *)
				ainput->record->record_type_struct;

	if ( picomotor_ainput_pointer == (MX_PICOMOTOR_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PICOMOTOR_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( picomotor_ainput != (MX_PICOMOTOR_AINPUT **) NULL ) {
		*picomotor_ainput = picomotor_ainput_pointer;
	}

	if ( picomotor_controller != (MX_PICOMOTOR_CONTROLLER **) NULL ) {

		picomotor_controller_record =
			picomotor_ainput_pointer->picomotor_controller_record;

		if ( picomotor_controller_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PICOMOTOR_CONTROLLER pointer for PICOMOTOR analog input "
		"record '%s' passed by '%s' is NULL.",
				ainput->record->name, calling_fname );
		}

		*picomotor_controller = (MX_PICOMOTOR_CONTROLLER *)
			picomotor_controller_record->record_type_struct;

		if ( *picomotor_controller == (MX_PICOMOTOR_CONTROLLER *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PICOMOTOR_CONTROLLER pointer for Picomotor record '%s' used by "
	"Picomotor analog input record '%s' and passed by '%s' is NULL.",
				picomotor_controller_record->name,
				ainput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_picomotor_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_PICOMOTOR_AINPUT *picomotor_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        picomotor_ainput = (MX_PICOMOTOR_AINPUT *)
				malloc( sizeof(MX_PICOMOTOR_AINPUT) );

        if ( picomotor_ainput == (MX_PICOMOTOR_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PICOMOTOR_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = picomotor_ainput;
        record->class_specific_function_list
			= &mxd_picomotor_ain_analog_input_function_list;

        analog_input->record = record;
	picomotor_ainput->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_picomotor_ain_read()";

	MX_PICOMOTOR_AINPUT *picomotor_ainput;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	picomotor_controller = NULL;
	picomotor_ainput = NULL;

	mx_status = mxd_picomotor_ain_get_pointers( ainput,
			&picomotor_ainput, &picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "AIN %s %d",
		picomotor_ainput->driver_name,
		picomotor_ainput->channel_number );

	mx_status = mxi_picomotor_command( picomotor_controller, command,
					response, sizeof( response ),
					MXD_PICOMOTOR_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%ld", &(ainput->raw_value.long_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Could not find a numerical value in the response to an 'AI' "
		"command to Picomotor controller '%s' for "
		"analog input record '%s'.  Response = '%s'",
			picomotor_controller->record->name,
			ainput->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

