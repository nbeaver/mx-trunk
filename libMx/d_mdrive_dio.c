/*
 * Name:    d_mdrive_dio.c
 *
 * Purpose: MX input and output drivers to control the digital I/O ports
 *          on an Intelligent Motion Systems MDrive motor controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MDRIVE_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_mdrive.h"
#include "d_mdrive_dio.h"

/* Initialize the MDRIVE digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_mdrive_din_record_function_list = {
	NULL,
	mxd_mdrive_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_mdrive_din_digital_input_function_list = {
	mxd_mdrive_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_mdrive_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_MDRIVE_DINPUT_STANDARD_FIELDS
};

long mxd_mdrive_din_num_record_fields
		= sizeof( mxd_mdrive_din_record_field_defaults )
			/ sizeof( mxd_mdrive_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mdrive_din_rfield_def_ptr
			= &mxd_mdrive_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_mdrive_dout_record_function_list = {
	NULL,
	mxd_mdrive_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_mdrive_dout_digital_output_function_list = {
	mxd_mdrive_dout_read,
	mxd_mdrive_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_mdrive_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_MDRIVE_DOUTPUT_STANDARD_FIELDS
};

long mxd_mdrive_dout_num_record_fields
		= sizeof( mxd_mdrive_dout_record_field_defaults )
			/ sizeof( mxd_mdrive_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mdrive_dout_rfield_def_ptr
			= &mxd_mdrive_dout_record_field_defaults[0];

static mx_status_type
mxd_mdrive_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_MDRIVE_DINPUT **mdrive_dinput,
			MX_MDRIVE **mdrive,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mdrive_din_get_pointers()";

	MX_RECORD *mdrive_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mdrive_dinput == (MX_MDRIVE_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MDRIVE_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mdrive == (MX_MDRIVE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MDRIVE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mdrive_dinput = (MX_MDRIVE_DINPUT *)
				dinput->record->record_type_struct;

	if ( *mdrive_dinput == (MX_MDRIVE_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MDRIVE_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	mdrive_record = (*mdrive_dinput)->mdrive_record;

	if ( mdrive_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MDRIVE pointer for MDRIVE digital input "
		"record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( mdrive_record->mx_type != MXT_MTR_MDRIVE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"mdrive_record '%s' for MDrive digital input '%s' "
		"is not a MDrive record.  Instead, it is a '%s' record.",
			mdrive_record->name, dinput->record->name,
			mx_get_driver_name( mdrive_record ) );
	}

	*mdrive = (MX_MDRIVE *) mdrive_record->record_type_struct;

	if ( *mdrive == (MX_MDRIVE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MDRIVE pointer for MDrive record '%s' used by "
	"MDrive digital input record '%s' and passed by '%s' is NULL.",
			mdrive_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mdrive_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_MDRIVE_DOUTPUT **mdrive_doutput,
			MX_MDRIVE **mdrive,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mdrive_dout_get_pointers()";

	MX_RECORD *mdrive_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mdrive_doutput == (MX_MDRIVE_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MDRIVE_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mdrive == (MX_MDRIVE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MDRIVE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mdrive_doutput = (MX_MDRIVE_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *mdrive_doutput == (MX_MDRIVE_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MDRIVE_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	mdrive_record = (*mdrive_doutput)->mdrive_record;

	if ( mdrive_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MDRIVE pointer for MDRIVE digital input "
		"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( mdrive_record->mx_type != MXT_MTR_MDRIVE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"mdrive_record '%s' for MDrive digital input '%s' "
		"is not a MDrive record.  Instead, it is a '%s' record.",
			mdrive_record->name, doutput->record->name,
			mx_get_driver_name( mdrive_record ) );
	}

	*mdrive = (MX_MDRIVE *) mdrive_record->record_type_struct;

	if ( *mdrive == (MX_MDRIVE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MDRIVE pointer for MDrive record '%s' used by "
	"MDrive digital input record '%s' and passed by '%s' is NULL.",
			mdrive_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_mdrive_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_mdrive_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_MDRIVE_DINPUT *mdrive_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        mdrive_dinput = (MX_MDRIVE_DINPUT *)
				malloc( sizeof(MX_MDRIVE_DINPUT) );

        if ( mdrive_dinput == (MX_MDRIVE_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MDRIVE_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = mdrive_dinput;
        record->class_specific_function_list
			= &mxd_mdrive_din_digital_input_function_list;

        digital_input->record = record;
	mdrive_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_mdrive_din_read()";

	MX_MDRIVE_DINPUT *mdrive_dinput;
	MX_MDRIVE *mdrive;
	char command[80];
	char response[80];
	int num_items, port_status;
	long port_number;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mdrive = NULL;
	mdrive_dinput = NULL;

	mx_status = mxd_mdrive_din_get_pointers( dinput,
					&mdrive_dinput, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_number = mdrive_dinput->port_number;

	if ((port_number < 1) || (port_number > 4)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %ld used by digital input record '%s' is outside "
	"the legal range of 1 to 4.", port_number,
			dinput->record->name );
	}

	sprintf( command, "PR I%ld", mdrive_dinput->port_number );

	mx_status = mxd_mdrive_command( mdrive, command,
				response, sizeof( response ),
				MXD_MDRIVE_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &port_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to parse response to a '%s' command sent to "
		"MDrive controller '%s'.  Response = '%s'",
			command, mdrive->record->name, response );
	}

	if ( port_status ) {
		dinput->value = 1;
	} else {
		dinput->value = 0;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_mdrive_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_mdrive_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_MDRIVE_DOUTPUT *mdrive_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        mdrive_doutput = (MX_MDRIVE_DOUTPUT *)
			malloc( sizeof(MX_MDRIVE_DOUTPUT) );

        if ( mdrive_doutput == (MX_MDRIVE_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MDRIVE_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = mdrive_doutput;
        record->class_specific_function_list
			= &mxd_mdrive_dout_digital_output_function_list;

        digital_output->record = record;
	mdrive_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mdrive_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_mdrive_dout_write()";

	MX_MDRIVE_DOUTPUT *mdrive_doutput;
	MX_MDRIVE *mdrive;
	char command[80];
	long port_number;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mdrive = NULL;
	mdrive_doutput = NULL;

	mx_status = mxd_mdrive_dout_get_pointers( doutput,
					&mdrive_doutput, &mdrive, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_number = mdrive_doutput->port_number;

	if ((port_number < 1) || (port_number > 4)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %ld used by digital output record '%s' is outside "
	"the legal range of 1 to 4.", port_number,
			doutput->record->name );
	}

	if ( doutput->value != 0 ) {
		doutput->value = 1;
	}

	sprintf( command, "I%ld=%lu",
			mdrive_doutput->port_number, doutput->value );

	mx_status = mxd_mdrive_command( mdrive, command,
					NULL, 0, MXD_MDRIVE_DIO_DEBUG );

	return mx_status;
}

