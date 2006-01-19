/*
 * Name:    d_mclennan_dio.c
 *
 * Purpose: MX input and output drivers to control Mclennan digital I/O ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MCLENNAN_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_mclennan.h"
#include "d_mclennan_dio.h"

/* Initialize the MCLENNAN digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_mclennan_din_record_function_list = {
	NULL,
	mxd_mclennan_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_mclennan_din_digital_input_function_list = {
	mxd_mclennan_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_mclennan_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_MCLENNAN_DINPUT_STANDARD_FIELDS
};

long mxd_mclennan_din_num_record_fields
		= sizeof( mxd_mclennan_din_record_field_defaults )
			/ sizeof( mxd_mclennan_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mclennan_din_rfield_def_ptr
			= &mxd_mclennan_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_mclennan_dout_record_function_list = {
	NULL,
	mxd_mclennan_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_mclennan_dout_digital_output_function_list = {
	mxd_mclennan_dout_read,
	mxd_mclennan_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_mclennan_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_MCLENNAN_DOUTPUT_STANDARD_FIELDS
};

long mxd_mclennan_dout_num_record_fields
		= sizeof( mxd_mclennan_dout_record_field_defaults )
			/ sizeof( mxd_mclennan_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mclennan_dout_rfield_def_ptr
			= &mxd_mclennan_dout_record_field_defaults[0];

static mx_status_type
mxd_mclennan_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_MCLENNAN_DINPUT **mclennan_dinput,
			MX_MCLENNAN **mclennan,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mclennan_din_get_pointers()";

	MX_RECORD *mclennan_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan_dinput == (MX_MCLENNAN_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCLENNAN_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan == (MX_MCLENNAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCLENNAN pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mclennan_dinput = (MX_MCLENNAN_DINPUT *)
				dinput->record->record_type_struct;

	if ( *mclennan_dinput == (MX_MCLENNAN_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCLENNAN_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	mclennan_record = (*mclennan_dinput)->mclennan_record;

	if ( mclennan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MCLENNAN pointer for MCLENNAN digital input "
		"record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( mclennan_record->mx_type != MXT_MTR_MCLENNAN ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"mclennan_record '%s' for Mclennan digital input '%s' "
		"is not a Mclennan record.  Instead, it is a '%s' record.",
			mclennan_record->name, dinput->record->name,
			mx_get_driver_name( mclennan_record ) );
	}

	*mclennan = (MX_MCLENNAN *) mclennan_record->record_type_struct;

	if ( *mclennan == (MX_MCLENNAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MCLENNAN pointer for Mclennan record '%s' used by "
	"Mclennan digital input record '%s' and passed by '%s' is NULL.",
			mclennan_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mclennan_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_MCLENNAN_DOUTPUT **mclennan_doutput,
			MX_MCLENNAN **mclennan,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mclennan_dout_get_pointers()";

	MX_RECORD *mclennan_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan_doutput == (MX_MCLENNAN_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCLENNAN_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mclennan == (MX_MCLENNAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCLENNAN pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mclennan_doutput = (MX_MCLENNAN_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *mclennan_doutput == (MX_MCLENNAN_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCLENNAN_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	mclennan_record = (*mclennan_doutput)->mclennan_record;

	if ( mclennan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MCLENNAN pointer for MCLENNAN digital input "
		"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( mclennan_record->mx_type != MXT_MTR_MCLENNAN ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"mclennan_record '%s' for Mclennan digital input '%s' "
		"is not a Mclennan record.  Instead, it is a '%s' record.",
			mclennan_record->name, doutput->record->name,
			mx_get_driver_name( mclennan_record ) );
	}

	*mclennan = (MX_MCLENNAN *) mclennan_record->record_type_struct;

	if ( *mclennan == (MX_MCLENNAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_MCLENNAN pointer for Mclennan record '%s' used by "
	"Mclennan digital input record '%s' and passed by '%s' is NULL.",
			mclennan_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_mclennan_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_mclennan_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_MCLENNAN_DINPUT *mclennan_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        mclennan_dinput = (MX_MCLENNAN_DINPUT *)
				malloc( sizeof(MX_MCLENNAN_DINPUT) );

        if ( mclennan_dinput == (MX_MCLENNAN_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MCLENNAN_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = mclennan_dinput;
        record->class_specific_function_list
			= &mxd_mclennan_din_digital_input_function_list;

        digital_input->record = record;
	mclennan_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_mclennan_din_read()";

	MX_MCLENNAN_DINPUT *mclennan_dinput;
	MX_MCLENNAN *mclennan;
	char response[80];
	char input_char;
	int offset;
	int port_number;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mclennan = NULL;
	mclennan_dinput = NULL;

	mx_status = mxd_mclennan_din_get_pointers( dinput,
					&mclennan_dinput, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mclennan->num_dinput_ports == 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Mclennan controller '%s' used by digital input record '%s' "
		"does not support digital input ports.",
			mclennan->record->name,
			dinput->record->name );
	}

	port_number = mclennan_dinput->port_number;

	if ((port_number < 1) || (port_number > mclennan->num_dinput_ports)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %d used by digital input record '%s' is outside "
	"the legal range of 1 to %d.", port_number,
			dinput->record->name,
			mclennan->num_dinput_ports );
	}

	mx_status = mxd_mclennan_command( mclennan, "RP",
				response, sizeof( response ),
				MXD_MCLENNAN_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	offset = mclennan->num_dinput_ports - port_number;

	input_char = response[ offset ];

	if ( input_char == '1' ) {
		dinput->value = 1;
	} else
	if ( input_char == '0' ) {
		dinput->value = 0;
	} else {
		dinput->value = 0;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Characters other than '0' or '1' were found in the response "
	"to an RP command from motor controller '%s' for digital input '%s'."
	"  Response = '%s'",
			mclennan->record->name,
			dinput->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_mclennan_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_mclennan_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_MCLENNAN_DOUTPUT *mclennan_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        mclennan_doutput = (MX_MCLENNAN_DOUTPUT *)
			malloc( sizeof(MX_MCLENNAN_DOUTPUT) );

        if ( mclennan_doutput == (MX_MCLENNAN_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_MCLENNAN_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = mclennan_doutput;
        record->class_specific_function_list
			= &mxd_mclennan_dout_digital_output_function_list;

        digital_output->record = record;
	mclennan_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mclennan_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_mclennan_dout_write()";

	MX_MCLENNAN_DOUTPUT *mclennan_doutput;
	MX_MCLENNAN *mclennan;
	char command[80];
	int i, j, num_output_ports;
	int port_number;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	mclennan = NULL;
	mclennan_doutput = NULL;

	mx_status = mxd_mclennan_dout_get_pointers( doutput,
					&mclennan_doutput, &mclennan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mclennan->num_doutput_ports == 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Mclennan controller '%s' used by digital output record '%s' "
		"does not support digital output ports.",
			mclennan->record->name,
			doutput->record->name );
	}

	port_number = mclennan_doutput->port_number;

	if ((port_number < 1) || (port_number > mclennan->num_doutput_ports)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %d used by digital output record '%s' is outside "
	"the legal range of 1 to %d.", port_number,
			doutput->record->name,
			mclennan->num_doutput_ports );
	}

	if ( doutput->value != 0 ) {
		doutput->value = 1;
	}

	strcpy( command, "WP" );

	num_output_ports = mclennan->num_doutput_ports;

	for ( i = 0; i < num_output_ports; i++ ) {

		j = num_output_ports - i;

		if ( j == port_number ) {
			if ( doutput->value == 0 ) {
				command[i+2] = '0';
			} else {
				command[i+2] = '1';
			}
		} else {
			command[i+2] = '2';
		}
	}

	command[num_output_ports+2] = '\0';

	mx_status = mxd_mclennan_command( mclennan, command,
					NULL, 0, MXD_MCLENNAN_DIO_DEBUG );

	return mx_status;
}

