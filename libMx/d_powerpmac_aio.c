/*
 * Name:    d_powerpmac_aio.c
 *
 * Purpose: MX input and output drivers to control Power PMAC variables as if
 *          they were analog inputs and outputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_POWERPMAC_AIO_DEBUG		TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_POWERPMAC_LIBRARY

#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "i_powerpmac.h"
#include "d_powerpmac_aio.h"

/* Initialize the POWERPMAC analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_powerpmac_ain_record_function_list = {
	NULL,
	mxd_powerpmac_ain_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_powerpmac_ain_analog_input_function_list = {
	mxd_powerpmac_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_powerpmac_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_POWERPMAC_AINPUT_STANDARD_FIELDS
};

long mxd_powerpmac_ain_num_record_fields
		= sizeof( mxd_powerpmac_ain_record_field_defaults )
			/ sizeof( mxd_powerpmac_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_ain_rfield_def_ptr
			= &mxd_powerpmac_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_powerpmac_aout_record_function_list = {
	NULL,
	mxd_powerpmac_aout_create_record_structures
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_powerpmac_aout_analog_output_function_list = {
	mxd_powerpmac_aout_read,
	mxd_powerpmac_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_powerpmac_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_POWERPMAC_AOUTPUT_STANDARD_FIELDS
};

long mxd_powerpmac_aout_num_record_fields
		= sizeof( mxd_powerpmac_aout_record_field_defaults )
			/ sizeof( mxd_powerpmac_aout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_aout_rfield_def_ptr
			= &mxd_powerpmac_aout_record_field_defaults[0];

static mx_status_type
mxd_powerpmac_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_POWERPMAC_AINPUT **powerpmac_ainput,
			MX_POWERPMAC **powerpmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_powerpmac_ain_get_pointers()";

	MX_RECORD *powerpmac_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (powerpmac_ainput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_POWERPMAC_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (powerpmac == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_POWERPMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*powerpmac_ainput = (MX_POWERPMAC_AINPUT *) ainput->record->record_type_struct;

	if ( *powerpmac_ainput == (MX_POWERPMAC_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_POWERPMAC_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	powerpmac_record = (*powerpmac_ainput)->powerpmac_record;

	if ( powerpmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_POWERPMAC pointer for POWERPMAC analog input record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( powerpmac_record->mx_type != MXI_CTRL_POWERPMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"powerpmac_record '%s' for POWERPMAC analog input '%s' is not a POWERPMAC record.  "
"Instead, it is a '%s' record.",
			powerpmac_record->name, ainput->record->name,
			mx_get_driver_name( powerpmac_record ) );
	}

	*powerpmac = (MX_POWERPMAC *) powerpmac_record->record_type_struct;

	if ( *powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_POWERPMAC pointer for POWERPMAC record '%s' used by POWERPMAC analog input "
	"record '%s' and passed by '%s' is NULL.",
			powerpmac_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_powerpmac_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_POWERPMAC_AOUTPUT **powerpmac_aoutput,
			MX_POWERPMAC **powerpmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_powerpmac_aout_get_pointers()";

	MX_RECORD *powerpmac_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (powerpmac_aoutput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_POWERPMAC_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (powerpmac == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_POWERPMAC pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*powerpmac_aoutput = (MX_POWERPMAC_AOUTPUT *) aoutput->record->record_type_struct;

	if ( *powerpmac_aoutput == (MX_POWERPMAC_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_POWERPMAC_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	powerpmac_record = (*powerpmac_aoutput)->powerpmac_record;

	if ( powerpmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_POWERPMAC pointer for POWERPMAC analog output record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( powerpmac_record->mx_type != MXI_CTRL_POWERPMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"powerpmac_record '%s' for POWERPMAC analog output '%s' is not a POWERPMAC record.  "
"Instead, it is a '%s' record.",
			powerpmac_record->name, aoutput->record->name,
			mx_get_driver_name( powerpmac_record ) );
	}

	*powerpmac = (MX_POWERPMAC *) powerpmac_record->record_type_struct;

	if ( *powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_POWERPMAC pointer for POWERPMAC record '%s' used by POWERPMAC analog output "
	"record '%s' and passed by '%s' is NULL.",
			powerpmac_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_powerpmac_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_powerpmac_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_POWERPMAC_AINPUT *powerpmac_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        powerpmac_ainput = (MX_POWERPMAC_AINPUT *) malloc( sizeof(MX_POWERPMAC_AINPUT) );

        if ( powerpmac_ainput == (MX_POWERPMAC_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_POWERPMAC_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = powerpmac_ainput;
        record->class_specific_function_list
                                = &mxd_powerpmac_ain_analog_input_function_list;

        analog_input->record = record;
	powerpmac_ainput->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_powerpmac_ain_read()";

	MX_POWERPMAC_AINPUT *powerpmac_ainput;
	MX_POWERPMAC *powerpmac;
	char command[80];
	char response[80];
	int num_items;
	char *string_ptr;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	powerpmac = NULL;
	powerpmac_ainput = NULL;

	mx_status = mxd_powerpmac_ain_get_pointers( ainput,
					&powerpmac_ainput, &powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( command, powerpmac_ainput->powerpmac_variable_name,
			sizeof(command) );

	mx_status = mxi_powerpmac_command( powerpmac, command,
				response, sizeof( response ),
				MXD_POWERPMAC_AIO_DEBUG );

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

	num_items = sscanf( string_ptr, "%lg",
				&(ainput->raw_value.double_value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"POWERPMAC variable value not found in response '%s' for "
		"POWERPMAC analog input record '%s'.",
			response, ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_powerpmac_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_powerpmac_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_POWERPMAC_AOUTPUT *powerpmac_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *)
					malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        powerpmac_aoutput = (MX_POWERPMAC_AOUTPUT *) malloc( sizeof(MX_POWERPMAC_AOUTPUT) );

        if ( powerpmac_aoutput == (MX_POWERPMAC_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_POWERPMAC_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = powerpmac_aoutput;
        record->class_specific_function_list
                                = &mxd_powerpmac_aout_analog_output_function_list;

        analog_output->record = record;
	powerpmac_aoutput->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_powerpmac_aout_read()";

	MX_POWERPMAC_AOUTPUT *powerpmac_aoutput;
	MX_POWERPMAC *powerpmac;
	char command[80];
	char response[80];
	int num_items;
	char *string_ptr;
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_aout_get_pointers( aoutput,
					&powerpmac_aoutput, &powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( command, powerpmac_aoutput->powerpmac_variable_name,
			sizeof(command) );

	mx_status = mxi_powerpmac_command( powerpmac, command,
				response, sizeof( response ),
				MXD_POWERPMAC_AIO_DEBUG );

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

	num_items = sscanf( string_ptr, "%lg",
				&(aoutput->raw_value.double_value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"POWERPMAC variable value not found in response '%s' for "
		"POWERPMAC analog output record '%s'.",
			response, aoutput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_powerpmac_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_powerpmac_aout_write()";

	MX_POWERPMAC_AOUTPUT *powerpmac_aoutput;
	MX_POWERPMAC *powerpmac;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_powerpmac_aout_get_pointers( aoutput,
					&powerpmac_aoutput, &powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"%s=%g",
		powerpmac_aoutput->powerpmac_variable_name,
		aoutput->raw_value.double_value );

	mx_status = mxi_powerpmac_command( powerpmac, command,
					NULL, 0, MXD_POWERPMAC_AIO_DEBUG );

	return mx_status;
}

#endif /* HAVE_POWERPMAC_LIBRARY */
