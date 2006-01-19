/*
 * Name:    d_icplus_dio.c
 *
 * Purpose: MX input and output drivers to control Oxford Danfysik
 *          IC PLUS digital I/O ports.
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

#define MXD_ICPLUS_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_amplifier.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_icplus.h"
#include "d_icplus_dio.h"

/* Initialize the ICPLUS digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_icplus_din_record_function_list = {
	NULL,
	mxd_icplus_din_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_icplus_din_digital_input_function_list = {
	mxd_icplus_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_icplus_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_ICPLUS_DINPUT_STANDARD_FIELDS
};

long mxd_icplus_din_num_record_fields
		= sizeof( mxd_icplus_din_record_field_defaults )
			/ sizeof( mxd_icplus_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_icplus_din_rfield_def_ptr
			= &mxd_icplus_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_icplus_dout_record_function_list = {
	NULL,
	mxd_icplus_dout_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_icplus_dout_digital_output_function_list = {
	mxd_icplus_dout_read,
	mxd_icplus_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_icplus_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_ICPLUS_DOUTPUT_STANDARD_FIELDS
};

long mxd_icplus_dout_num_record_fields
		= sizeof( mxd_icplus_dout_record_field_defaults )
			/ sizeof( mxd_icplus_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_icplus_dout_rfield_def_ptr
			= &mxd_icplus_dout_record_field_defaults[0];

static mx_status_type
mxd_icplus_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_ICPLUS_DINPUT **icplus_dinput,
			MX_ICPLUS **icplus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_icplus_din_get_pointers()";

	MX_RECORD *icplus_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( icplus_dinput == (MX_ICPLUS_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( icplus == (MX_ICPLUS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*icplus_dinput = (MX_ICPLUS_DINPUT *)
				dinput->record->record_type_struct;

	if ( *icplus_dinput == (MX_ICPLUS_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ICPLUS_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	icplus_record = (*icplus_dinput)->icplus_record;

	if ( icplus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ICPLUS pointer for ICPLUS digital input "
		"record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( ( icplus_record->mx_type != MXT_AMP_ICPLUS )
	  && ( icplus_record->mx_type != MXT_AMP_QBPM ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"icplus_record '%s' for digital input '%s' is not an IC PLUS "
		"or QBPM record.  Instead, it is a '%s' record.",
			icplus_record->name, dinput->record->name,
			mx_get_driver_name( icplus_record ) );
	}

	*icplus = (MX_ICPLUS *) icplus_record->record_type_struct;

	if ( *icplus == (MX_ICPLUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ICPLUS pointer for IC PLUS record '%s' used by "
	"IC PLUS digital input record '%s' and passed by '%s' is NULL.",
			icplus_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_icplus_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_ICPLUS_DOUTPUT **icplus_doutput,
			MX_ICPLUS **icplus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_icplus_dout_get_pointers()";

	MX_RECORD *icplus_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( icplus_doutput == (MX_ICPLUS_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( icplus == (MX_ICPLUS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ICPLUS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*icplus_doutput = (MX_ICPLUS_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *icplus_doutput == (MX_ICPLUS_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_ICPLUS_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	icplus_record = (*icplus_doutput)->icplus_record;

	if ( icplus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ICPLUS pointer for ICPLUS digital input "
		"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( ( icplus_record->mx_type != MXT_AMP_ICPLUS )
	  && ( icplus_record->mx_type != MXT_AMP_QBPM ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"icplus_record '%s' for digital output '%s' is not an IC PLUS "
		"or QBPM record.  Instead, it is a '%s' record.",
			icplus_record->name, doutput->record->name,
			mx_get_driver_name( icplus_record ) );
	}

	*icplus = (MX_ICPLUS *) icplus_record->record_type_struct;

	if ( *icplus == (MX_ICPLUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ICPLUS pointer for IC PLUS record '%s' used by "
	"IC PLUS digital input record '%s' and passed by '%s' is NULL.",
			icplus_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_icplus_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_icplus_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_ICPLUS_DINPUT *icplus_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        icplus_dinput = (MX_ICPLUS_DINPUT *)
				malloc( sizeof(MX_ICPLUS_DINPUT) );

        if ( icplus_dinput == (MX_ICPLUS_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ICPLUS_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = icplus_dinput;
        record->class_specific_function_list
			= &mxd_icplus_din_digital_input_function_list;

        digital_input->record = record;
	icplus_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_icplus_din_read()";

	MX_ICPLUS_DINPUT *icplus_dinput;
	MX_ICPLUS *icplus;
	char command[40];
	char response[80];
	int port_number, num_items;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	icplus = NULL;
	icplus_dinput = NULL;

	mx_status = mxd_icplus_din_get_pointers( dinput,
					&icplus_dinput, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_number = icplus_dinput->port_number;

	if ((port_number < 0) || (port_number > 2)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %d used by digital input record '%s' is outside "
	"the legal range of 0 to 2.", port_number, dinput->record->name );
	}

	sprintf( command, ":SENS%d:STAT%d?", icplus->address, port_number );

	mx_status = mxd_icplus_command( icplus, command,
				response, sizeof( response ),
				MXD_ICPLUS_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &(dinput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"IC PLUS digital input port '%s' returned "
			"an unrecognizable response to a '%s' command.  "
			"Response = '%s'.", dinput->record->name,
				command, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_icplus_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_icplus_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_ICPLUS_DOUTPUT *icplus_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        icplus_doutput = (MX_ICPLUS_DOUTPUT *)
			malloc( sizeof(MX_ICPLUS_DOUTPUT) );

        if ( icplus_doutput == (MX_ICPLUS_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ICPLUS_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = icplus_doutput;
        record->class_specific_function_list
			= &mxd_icplus_dout_digital_output_function_list;

        digital_output->record = record;
	icplus_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_icplus_dout_read()";

	MX_ICPLUS_DOUTPUT *icplus_doutput;
	MX_ICPLUS *icplus;
	char command[40];
	char response[80];
	int port_number, num_items;
	mx_status_type mx_status;

	mx_status = mxd_icplus_dout_get_pointers( doutput,
					&icplus_doutput, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_number = icplus_doutput->port_number;

	if ((port_number < 0) || (port_number > 2)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %d used by digital output record '%s' is outside "
	"the legal range of 0 to 2.", port_number, doutput->record->name );
	}

	sprintf( command, ":SOUR%d:STAT%d?", icplus->address, port_number );

	mx_status = mxd_icplus_command( icplus, command,
				response, sizeof( response ),
				MXD_ICPLUS_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &(doutput->value) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"IC PLUS digital output port '%s' returned "
			"an unrecognizable response to a '%s' command.  "
			"Response = '%s'.", doutput->record->name,
				command, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_icplus_dout_write()";

	MX_ICPLUS_DOUTPUT *icplus_doutput;
	MX_ICPLUS *icplus;
	char command[80];
	int port_number;
	mx_status_type mx_status;

	mx_status = mxd_icplus_dout_get_pointers( doutput,
					&icplus_doutput, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_number = icplus_doutput->port_number;

	if ((port_number < 0) || (port_number > 2)) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Port number %d used by digital output record '%s' is outside "
	"the legal range of 0 to 2.", port_number, doutput->record->name );
	}

	if ( doutput->value != 0 ) {
		doutput->value = 1;
	}

	sprintf( command, ":SOUR%d:STAT%d %ld",
			icplus->address, port_number, doutput->value );

	mx_status = mxd_icplus_command( icplus, command,
					NULL, 0, MXD_ICPLUS_DIO_DEBUG );

	return mx_status;
}

