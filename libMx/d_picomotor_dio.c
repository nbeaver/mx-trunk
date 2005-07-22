/*
 * Name:    d_picomotor_dio.c
 *
 * Purpose: MX input and output drivers to control New Focus Picomotor
 *          digital I/O ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PICOMOTOR_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_types.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_picomotor.h"
#include "d_picomotor_dio.h"

/* Initialize the PICOMOTOR digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_picomotor_din_record_function_list = {
	NULL,
	mxd_picomotor_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_picomotor_din_digital_input_function_list = {
	mxd_picomotor_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_picomotor_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_PICOMOTOR_DINPUT_STANDARD_FIELDS
};

long mxd_picomotor_din_num_record_fields
		= sizeof( mxd_picomotor_din_record_field_defaults )
			/ sizeof( mxd_picomotor_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_din_rfield_def_ptr
			= &mxd_picomotor_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_picomotor_dout_record_function_list = {
	NULL,
	mxd_picomotor_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_picomotor_dout_digital_output_function_list = {
	mxd_picomotor_dout_read,
	mxd_picomotor_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_picomotor_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_PICOMOTOR_DOUTPUT_STANDARD_FIELDS
};

long mxd_picomotor_dout_num_record_fields
		= sizeof( mxd_picomotor_dout_record_field_defaults )
			/ sizeof( mxd_picomotor_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_dout_rfield_def_ptr
			= &mxd_picomotor_dout_record_field_defaults[0];

static mx_status_type
mxd_picomotor_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_PICOMOTOR_DINPUT **picomotor_dinput,
			MX_PICOMOTOR_CONTROLLER **picomotor_controller,
			const char *calling_fname )
{
	static const char fname[] = "mxd_picomotor_din_get_pointers()";

	MX_RECORD *picomotor_controller_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( picomotor_dinput == (MX_PICOMOTOR_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PICOMOTOR_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( picomotor_controller == (MX_PICOMOTOR_CONTROLLER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PICOMOTOR_CONTROLLER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*picomotor_dinput = (MX_PICOMOTOR_DINPUT *)
				dinput->record->record_type_struct;

	if ( *picomotor_dinput == (MX_PICOMOTOR_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PICOMOTOR_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	picomotor_controller_record = (*picomotor_dinput)->picomotor_controller_record;

	if ( picomotor_controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PICOMOTOR_CONTROLLER pointer for PICOMOTOR digital input "
		"record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( picomotor_controller_record->mx_type != MXT_MTR_PICOMOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"picomotor_controller_record '%s' for Picomotor digital input '%s' "
		"is not a Picomotor record.  Instead, it is a '%s' record.",
			picomotor_controller_record->name, dinput->record->name,
			mx_get_driver_name( picomotor_controller_record ) );
	}

	*picomotor_controller = (MX_PICOMOTOR_CONTROLLER *)
				picomotor_controller_record->record_type_struct;

	if ( *picomotor_controller == (MX_PICOMOTOR_CONTROLLER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PICOMOTOR_CONTROLLER pointer for Picomotor record '%s' used by "
	"Picomotor digital input record '%s' and passed by '%s' is NULL.",
			picomotor_controller_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_picomotor_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_PICOMOTOR_DOUTPUT **picomotor_doutput,
			MX_PICOMOTOR_CONTROLLER **picomotor_controller,
			const char *calling_fname )
{
	static const char fname[] = "mxd_picomotor_dout_get_pointers()";

	MX_RECORD *picomotor_controller_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( picomotor_doutput == (MX_PICOMOTOR_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PICOMOTOR_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( picomotor_controller == (MX_PICOMOTOR_CONTROLLER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PICOMOTOR_CONTROLLER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*picomotor_doutput = (MX_PICOMOTOR_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *picomotor_doutput == (MX_PICOMOTOR_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PICOMOTOR_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	picomotor_controller_record = (*picomotor_doutput)->picomotor_controller_record;

	if ( picomotor_controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PICOMOTOR_CONTROLLER pointer for PICOMOTOR digital input "
		"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( picomotor_controller_record->mx_type != MXT_MTR_PICOMOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"picomotor_controller_record '%s' for Picomotor digital input '%s' "
		"is not a Picomotor record.  Instead, it is a '%s' record.",
			picomotor_controller_record->name, doutput->record->name,
			mx_get_driver_name( picomotor_controller_record ) );
	}

	*picomotor_controller = (MX_PICOMOTOR_CONTROLLER *)
				picomotor_controller_record->record_type_struct;

	if ( *picomotor_controller == (MX_PICOMOTOR_CONTROLLER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_PICOMOTOR_CONTROLLER pointer for Picomotor record '%s' used by "
	"Picomotor digital input record '%s' and passed by '%s' is NULL.",
			picomotor_controller_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_picomotor_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_picomotor_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_PICOMOTOR_DINPUT *picomotor_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        picomotor_dinput = (MX_PICOMOTOR_DINPUT *)
				malloc( sizeof(MX_PICOMOTOR_DINPUT) );

        if ( picomotor_dinput == (MX_PICOMOTOR_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PICOMOTOR_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = picomotor_dinput;
        record->class_specific_function_list
			= &mxd_picomotor_din_digital_input_function_list;

        digital_input->record = record;
	picomotor_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_picomotor_din_read()";

	MX_PICOMOTOR_DINPUT *picomotor_dinput;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[80];
	char response[80];
	char *ptr;
	int num_items, channel_number;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_din_get_pointers( dinput,
			&picomotor_dinput, &picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_number = picomotor_dinput->channel_number;

	if ( (channel_number < 0) || (channel_number > 9 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %d used by digital input record '%s' is outside "
	"the legal range of 0 to 9.", channel_number,
			dinput->record->name );
	}

	sprintf( command, "IN %s %d",
			picomotor_dinput->driver_name,
			picomotor_dinput->channel_number );

	mx_status = mxi_picomotor_command( picomotor_controller, command, 
					response, sizeof( response ),
					MXD_PICOMOTOR_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command '%s' by "
		"Picomotor digital input '%s'.  Response = '%s'",
			command, dinput->record->name, response );
	}

	ptr++;

	num_items = sscanf( ptr, "%ld", &(dinput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to find the digital input value for record '%s' "
		"in the response to command '%s' sent to Picomotor "
		"controller '%s'.  Response = '%s'",
			dinput->record->name, command,
			picomotor_controller->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_picomotor_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_picomotor_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_PICOMOTOR_DOUTPUT *picomotor_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        picomotor_doutput = (MX_PICOMOTOR_DOUTPUT *)
			malloc( sizeof(MX_PICOMOTOR_DOUTPUT) );

        if ( picomotor_doutput == (MX_PICOMOTOR_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_PICOMOTOR_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = picomotor_doutput;
        record->class_specific_function_list
			= &mxd_picomotor_dout_digital_output_function_list;

        digital_output->record = record;
	picomotor_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_picomotor_dout_read()";

	MX_PICOMOTOR_DOUTPUT *picomotor_doutput;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[80];
	char response[80];
	char *ptr;
	int num_items, channel_number;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_dout_get_pointers( doutput,
			&picomotor_doutput, &picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_number = picomotor_doutput->channel_number;

	if ( (channel_number < 0) || (channel_number > 9 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %d used by digital output record '%s' is outside "
	"the legal range of 0 to 9.", channel_number,
			doutput->record->name );
	}

	sprintf( command, "OUT %s %d",
			picomotor_doutput->driver_name,
			picomotor_doutput->channel_number );

	mx_status = mxi_picomotor_command( picomotor_controller, command, 
					response, sizeof( response ),
					MXD_PICOMOTOR_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command '%s' by "
		"Picomotor digital input '%s'.  Response = '%s'",
			command, doutput->record->name, response );
	}

	ptr++;

	num_items = sscanf( ptr, "%ld", &(doutput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to find the digital output value for record '%s' "
		"in the response to command '%s' sent to Picomotor "
		"controller '%s'.  Response = '%s'",
			doutput->record->name, command,
			picomotor_controller->record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_picomotor_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_picomotor_dout_write()";

	MX_PICOMOTOR_DOUTPUT *picomotor_doutput;
	MX_PICOMOTOR_CONTROLLER *picomotor_controller;
	char command[80];
	int channel_number;
	mx_status_type mx_status;

	mx_status = mxd_picomotor_dout_get_pointers( doutput,
			&picomotor_doutput, &picomotor_controller, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	channel_number = picomotor_doutput->channel_number;

	if (channel_number < 0) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Channel number %d used by digital output record '%s' is less than "
	"the minimum value of 0.", channel_number, doutput->record->name );
	}

	if ( doutput->value != 0 ) {
		doutput->value = 1;
	}

	sprintf( command, "OUT %s %d=%ld",
			picomotor_doutput->driver_name,
			channel_number, doutput->value );

	mx_status = mxi_picomotor_command( picomotor_controller, command,
					NULL, 0, MXD_PICOMOTOR_DIO_DEBUG );

	return mx_status;
}

