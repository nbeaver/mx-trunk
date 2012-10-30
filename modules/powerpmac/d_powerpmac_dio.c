/*
 * Name:    d_powerpmac_dio.c
 *
 * Purpose: MX input and output drivers to control Power PMAC variables as if
 *          they were digital input/output registers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_POWERPMAC_DIO_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_powerpmac.h"
#include "d_powerpmac_dio.h"

/* Initialize the POWERPMAC digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_powerpmac_din_record_function_list = {
	NULL,
	mxd_powerpmac_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_powerpmac_din_digital_input_function_list = {
	mxd_powerpmac_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_powerpmac_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_POWERPMAC_DINPUT_STANDARD_FIELDS
};

long mxd_powerpmac_din_num_record_fields
		= sizeof( mxd_powerpmac_din_record_field_defaults )
			/ sizeof( mxd_powerpmac_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_din_rfield_def_ptr
			= &mxd_powerpmac_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_powerpmac_dout_record_function_list = {
	NULL,
	mxd_powerpmac_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_powerpmac_dout_digital_output_function_list = {
	mxd_powerpmac_dout_read,
	mxd_powerpmac_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_powerpmac_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_POWERPMAC_DOUTPUT_STANDARD_FIELDS
};

long mxd_powerpmac_dout_num_record_fields
		= sizeof( mxd_powerpmac_dout_record_field_defaults )
			/ sizeof( mxd_powerpmac_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_dout_rfield_def_ptr
			= &mxd_powerpmac_dout_record_field_defaults[0];

static mx_status_type
mxd_powerpmac_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_POWERPMAC_DINPUT **powerpmac_dinput,
			MX_POWERPMAC **powerpmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_powerpmac_din_get_pointers()";

	MX_RECORD *powerpmac_record;
	MX_POWERPMAC_DINPUT *powerpmac_dinput_ptr;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed "
		"by '%s' was NULL.",
			calling_fname );
	}

	powerpmac_dinput_ptr = (MX_POWERPMAC_DINPUT *)
				dinput->record->record_type_struct;

	if ( powerpmac_dinput_ptr == (MX_POWERPMAC_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_POWERPMAC_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( powerpmac_dinput != (MX_POWERPMAC_DINPUT **) NULL ) {
		*powerpmac_dinput = powerpmac_dinput_ptr;
	}

	if ( powerpmac != (MX_POWERPMAC **) NULL ) {
		powerpmac_record = (*powerpmac_dinput)->powerpmac_record;

		if ( powerpmac_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_POWERPMAC pointer for Power PMAC digital input "
			"record '%s' passed by '%s' is NULL.",
				dinput->record->name, calling_fname );
		}

		*powerpmac = (MX_POWERPMAC *)
				powerpmac_record->record_type_struct;

		if ( *powerpmac == (MX_POWERPMAC *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_POWERPMAC pointer for Power PMAC record '%s' "
			"used by Power PMAC digital input record '%s' "
			"and passed by '%s' is NULL.",
				powerpmac_record->name,
				dinput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_powerpmac_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_POWERPMAC_DOUTPUT **powerpmac_doutput,
			MX_POWERPMAC **powerpmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_powerpmac_dout_get_pointers()";

	MX_RECORD *powerpmac_record;
	MX_POWERPMAC_DOUTPUT *powerpmac_doutput_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed "
		"by '%s' was NULL.",
			calling_fname );
	}

	powerpmac_doutput_ptr = (MX_POWERPMAC_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( powerpmac_doutput_ptr == (MX_POWERPMAC_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_POWERPMAC_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( powerpmac_doutput != (MX_POWERPMAC_DOUTPUT **) NULL ) {
		*powerpmac_doutput = powerpmac_doutput_ptr;
	}

	if ( powerpmac != (MX_POWERPMAC **) NULL ) {
		powerpmac_record = (*powerpmac_doutput)->powerpmac_record;

		if ( powerpmac_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_POWERPMAC pointer for Power PMAC digital output "
			"record '%s' passed by '%s' is NULL.",
				doutput->record->name, calling_fname );
		}

		*powerpmac = (MX_POWERPMAC *)
				powerpmac_record->record_type_struct;

		if ( *powerpmac == (MX_POWERPMAC *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_POWERPMAC pointer for Power PMAC record '%s' "
			"used by Power PMAC digital output record '%s' "
			"and passed by '%s' is NULL.",
				powerpmac_record->name,
				doutput->record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_powerpmac_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_powerpmac_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_POWERPMAC_DINPUT *powerpmac_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        powerpmac_dinput = (MX_POWERPMAC_DINPUT *)
				malloc( sizeof(MX_POWERPMAC_DINPUT) );

        if ( powerpmac_dinput == (MX_POWERPMAC_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_POWERPMAC_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = powerpmac_dinput;
        record->class_specific_function_list
			= &mxd_powerpmac_din_digital_input_function_list;

        digital_input->record = record;
	powerpmac_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_powerpmac_din_read()";

	MX_POWERPMAC_DINPUT *powerpmac_dinput = NULL;
	MX_POWERPMAC *powerpmac = NULL;
	char command[80];
	char response[80];
	int num_items;
	char *string_ptr;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_din_get_pointers( dinput,
					&powerpmac_dinput, &powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( command, powerpmac_dinput->powerpmac_variable_name,
			sizeof(command) );

	mx_status = mxi_powerpmac_command( powerpmac, command,
				response, sizeof( response ),
				MXD_POWERPMAC_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There should be an equals sign (=) in the response.
	 * The value we want follows the equals sign.
	 */

	string_ptr = strchr( response, '=' );

	if ( string_ptr == NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"No equals sign was found in the response '%s' "
		"by Power PMAC '%s' to the command '%s'.",
			response, powerpmac->record->name, command );
	}

	string_ptr++;	/* Skip over the equals sign. */

	/* Now parse the value returned. */

	num_items = sscanf( string_ptr, "%lu", &(dinput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Power PMAC variable value not found in response '%s' for "
		"Power PMAC digital input record '%s'.",
			response, dinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_powerpmac_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_powerpmac_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_POWERPMAC_DOUTPUT *powerpmac_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        powerpmac_doutput = (MX_POWERPMAC_DOUTPUT *)
				malloc( sizeof(MX_POWERPMAC_DOUTPUT) );

        if ( powerpmac_doutput == (MX_POWERPMAC_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_POWERPMAC_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = powerpmac_doutput;
        record->class_specific_function_list
			= &mxd_powerpmac_dout_digital_output_function_list;

        digital_output->record = record;
	powerpmac_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_powerpmac_dout_read()";

	MX_POWERPMAC_DOUTPUT *powerpmac_doutput;
	MX_POWERPMAC *powerpmac;
	char command[80];
	char response[80];
	int num_items;
	char *string_ptr;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_dout_get_pointers( doutput,
					&powerpmac_doutput, &powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( command, powerpmac_doutput->powerpmac_variable_name,
			sizeof(command) );

	mx_status = mxi_powerpmac_command( powerpmac, command,
				response, sizeof( response ),
				MXD_POWERPMAC_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There should be an equals sign (=) in the response.
	 * The value we want follows the equals sign.
	 */

	string_ptr = strchr( response, '=' );

	if ( string_ptr == NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"No equals sign was found in the response '%s' "
		"by Power PMAC '%s' to the command '%s'.",
			response, powerpmac->record->name, command );
	}

	string_ptr++;	/* Skip over the equals sign. */

	/* Now parse the value returned. */

	num_items = sscanf( string_ptr, "%lu", &(doutput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Power PMAC variable value not found in response '%s' for "
		"Power PMAC digital output record '%s'.",
			response, doutput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_powerpmac_dout_write()";

	MX_POWERPMAC_DOUTPUT *powerpmac_doutput;
	MX_POWERPMAC *powerpmac;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_dout_get_pointers( doutput,
					&powerpmac_doutput, &powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"%s=%lu",
		powerpmac_doutput->powerpmac_variable_name,
		doutput->value );

	mx_status = mxi_powerpmac_command( powerpmac, command,
					NULL, 0, MXD_POWERPMAC_DIO_DEBUG );

	return mx_status;
}

