/*
 * Name:    d_mdrive_aio.c
 *
 * Purpose: MX analog input driver to read the analog input port attached
 *          to an Intelligent Motion Systems MDrive motor controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MDRIVE_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_analog_input.h"
#include "d_mdrive.h"
#include "d_mdrive_aio.h"

/* Initialize the MDRIVE analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_mdrive_ain_record_function_list = {
	NULL,
	mxd_mdrive_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_mdrive_ain_analog_input_function_list = {
	mxd_mdrive_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_mdrive_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_MDRIVE_AINPUT_STANDARD_FIELDS
};

long mxd_mdrive_ain_num_record_fields
		= sizeof( mxd_mdrive_ain_record_field_defaults )
			/ sizeof( mxd_mdrive_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mdrive_ain_rfield_def_ptr
			= &mxd_mdrive_ain_record_field_defaults[0];

static mx_status_type
mxd_mdrive_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_MDRIVE_AINPUT **mdrive_ainput,
			MX_MDRIVE **mdrive,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mdrive_ain_get_pointers()";

	MX_RECORD *mdrive_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mdrive_ainput == (MX_MDRIVE_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MDRIVE_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mdrive == (MX_MDRIVE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MDRIVE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mdrive_ainput = (MX_MDRIVE_AINPUT *)
				ainput->record->record_type_struct;

	if ( *mdrive_ainput == (MX_MDRIVE_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MDRIVE_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	mdrive_record = (*mdrive_ainput)->mdrive_record;

	if ( mdrive_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MDRIVE pointer for MDRIVE analog input "
		"record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( mdrive_record->mx_type != MXT_MTR_MDRIVE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"mdrive_record '%s' for MDrive analog input '%s' "
		"is not a MDrive record.  Instead, it is a '%s' record.",
			mdrive_record->name, ainput->record->name,
			mx_get_driver_name( mdrive_record ) );
	}

	*mdrive = (MX_MDRIVE *) mdrive_record->record_type_struct;

	if ( *mdrive == (MX_MDRIVE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MDRIVE pointer for MDrive record '%s' used by "
	"MDrive analog input record '%s' and passed by '%s' is NULL.",
			mdrive_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_mdrive_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_mdrive_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_MDRIVE_AINPUT *mdrive_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        mdrive_ainput = (MX_MDRIVE_AINPUT *)
				malloc( sizeof(MX_MDRIVE_AINPUT) );

        if ( mdrive_ainput == (MX_MDRIVE_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MDRIVE_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = mdrive_ainput;
        record->class_specific_function_list
			= &mxd_mdrive_ain_analog_input_function_list;

        analog_input->record = record;
	mdrive_ainput->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_mdrive_ain_read()";

	MX_MDRIVE_AINPUT *mdrive_ainput;
	MX_MDRIVE *mdrive;
	char response[80];
	int num_items;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mdrive = NULL;
	mdrive_ainput = NULL;

	mx_status = mxd_mdrive_ain_get_pointers( ainput,
					&mdrive_ainput, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mdrive_command( mdrive, "PR I5",
				response, sizeof( response ),
				MXD_MDRIVE_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%ld", &(ainput->raw_value.long_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Could not find a numerical value in the response to a 'PR I5' "
		"command to MDrive controller '%s' for "
		"analog input record '%s'.  Response = '%s'",
			mdrive->record->name,
			ainput->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

