/*
 * Name:    d_smartmotor_dio.c
 *
 * Purpose: MX input and output drivers to control the digital I/O ports
 *          on an Animatics SmartMotor motor controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2012, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SMARTMOTOR_DIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "d_smartmotor.h"
#include "d_smartmotor_dio.h"

/* Initialize the SmartMotor digital I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_smartmotor_din_record_function_list = {
	NULL,
	mxd_smartmotor_din_create_record_structures,
	mxd_smartmotor_dout_finish_record_initialization
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_smartmotor_din_digital_input_function_list = {
	mxd_smartmotor_din_read
};

MX_RECORD_FIELD_DEFAULTS mxd_smartmotor_din_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_SMARTMOTOR_DINPUT_STANDARD_FIELDS
};

long mxd_smartmotor_din_num_record_fields
		= sizeof( mxd_smartmotor_din_record_field_defaults )
			/ sizeof( mxd_smartmotor_din_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_din_rfield_def_ptr
			= &mxd_smartmotor_din_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_smartmotor_dout_record_function_list = {
	NULL,
	mxd_smartmotor_dout_create_record_structures,
	mxd_smartmotor_dout_finish_record_initialization
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_smartmotor_dout_digital_output_function_list = {
	mxd_smartmotor_dout_read,
	mxd_smartmotor_dout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_smartmotor_dout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_SMARTMOTOR_DOUTPUT_STANDARD_FIELDS
};

long mxd_smartmotor_dout_num_record_fields
		= sizeof( mxd_smartmotor_dout_record_field_defaults )
			/ sizeof( mxd_smartmotor_dout_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_dout_rfield_def_ptr
			= &mxd_smartmotor_dout_record_field_defaults[0];

static mx_status_type
mxd_smartmotor_din_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_SMARTMOTOR_DINPUT **smartmotor_dinput,
			MX_SMARTMOTOR **smartmotor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_smartmotor_din_get_pointers()";

	MX_RECORD *smartmotor_record;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor_dinput == (MX_SMARTMOTOR_DINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SMARTMOTOR_DINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor == (MX_SMARTMOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SMARTMOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dinput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*smartmotor_dinput = (MX_SMARTMOTOR_DINPUT *)
				dinput->record->record_type_struct;

	if ( *smartmotor_dinput == (MX_SMARTMOTOR_DINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SMARTMOTOR_DINPUT pointer for record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	smartmotor_record = (*smartmotor_dinput)->smartmotor_record;

	if ( smartmotor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SMARTMOTOR pointer for SMARTMOTOR digital input "
		"record '%s' passed by '%s' is NULL.",
			dinput->record->name, calling_fname );
	}

	if ( smartmotor_record->mx_type != MXT_MTR_SMARTMOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"smartmotor_record '%s' for SmartMotor digital input '%s' "
		"is not a SmartMotor record.  Instead, it is a '%s' record.",
			smartmotor_record->name, dinput->record->name,
			mx_get_driver_name( smartmotor_record ) );
	}

	*smartmotor = (MX_SMARTMOTOR *) smartmotor_record->record_type_struct;

	if ( *smartmotor == (MX_SMARTMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SMARTMOTOR pointer for SmartMotor record '%s' used by "
	"SmartMotor digital input record '%s' and passed by '%s' is NULL.",
			smartmotor_record->name,
			dinput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_smartmotor_dout_get_pointers( MX_DIGITAL_OUTPUT *doutput,
			MX_SMARTMOTOR_DOUTPUT **smartmotor_doutput,
			MX_SMARTMOTOR **smartmotor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_smartmotor_dout_get_pointers()";

	MX_RECORD *smartmotor_record;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor_doutput == (MX_SMARTMOTOR_DOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SMARTMOTOR_DOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor == (MX_SMARTMOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SMARTMOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_DIGITAL_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*smartmotor_doutput = (MX_SMARTMOTOR_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( *smartmotor_doutput == (MX_SMARTMOTOR_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SMARTMOTOR_DOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	smartmotor_record = (*smartmotor_doutput)->smartmotor_record;

	if ( smartmotor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SMARTMOTOR pointer for SMARTMOTOR digital input "
		"record '%s' passed by '%s' is NULL.",
			doutput->record->name, calling_fname );
	}

	if ( smartmotor_record->mx_type != MXT_MTR_SMARTMOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"smartmotor_record '%s' for SmartMotor digital input '%s' "
		"is not a SmartMotor record.  Instead, it is a '%s' record.",
			smartmotor_record->name, doutput->record->name,
			mx_get_driver_name( smartmotor_record ) );
	}

	*smartmotor = (MX_SMARTMOTOR *) smartmotor_record->record_type_struct;

	if ( *smartmotor == (MX_SMARTMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SMARTMOTOR pointer for SmartMotor record '%s' used by "
	"SmartMotor digital input record '%s' and passed by '%s' is NULL.",
			smartmotor_record->name,
			doutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_smartmotor_din_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_smartmotor_din_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_SMARTMOTOR_DINPUT *smartmotor_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        smartmotor_dinput = (MX_SMARTMOTOR_DINPUT *)
				malloc( sizeof(MX_SMARTMOTOR_DINPUT) );

        if ( smartmotor_dinput == (MX_SMARTMOTOR_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SMARTMOTOR_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = smartmotor_dinput;
        record->class_specific_function_list
			= &mxd_smartmotor_din_digital_input_function_list;

        digital_input->record = record;
	smartmotor_dinput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_din_finish_record_initialization( MX_RECORD *record )
{
        MX_SMARTMOTOR_DINPUT *smartmotor_dinput;
	char *port_name;
	int i;
	size_t length;
	mx_status_type mx_status;

	smartmotor_dinput = (MX_SMARTMOTOR_DINPUT *)
					record->record_type_struct;

	/* Force the port name to upper case. */

	port_name = smartmotor_dinput->port_name;

	length = strlen(port_name);

	for ( i = 0; i < length; i++ ) {
		if ( islower( (int)(port_name[i]) ) ) {
			port_name[i] = toupper( (int)(port_name[i]) );
		}
	}

	mx_status = mxd_smartmotor_check_port_name( record,
					smartmotor_dinput->port_name );

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_din_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_smartmotor_din_read()";

	MX_SMARTMOTOR_DINPUT *smartmotor_dinput;
	MX_SMARTMOTOR *smartmotor;
	char command[80];
	char response[80];
	char *port_name;
	int num_items;
	unsigned long port_status;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	smartmotor = NULL;
	smartmotor_dinput = NULL;

	mx_status = mxd_smartmotor_din_get_pointers( dinput,
				&smartmotor_dinput, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_name = smartmotor_dinput->port_name;

	if ( port_name[0] == 'U' ) {
		snprintf( command, sizeof(command), "z=%sI Rz", port_name );

	} else if ( strcmp( port_name, "TEMP" ) == 0 ) {
		snprintf( command, sizeof(command), "z=TEMP Rz" );
	} else {
		snprintf( command, sizeof(command), "RDIN%s", port_name );
	}

	mx_status = mxd_smartmotor_command( smartmotor, command,
				response, sizeof( response ),
				MXD_SMARTMOTOR_DIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &port_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to parse response to a '%s' command sent to "
		"SmartMotor controller '%s'.  Response = '%s'",
			command, smartmotor->record->name, response );
	}

	dinput->value = port_status;

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_smartmotor_dout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_smartmotor_dout_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_SMARTMOTOR_DOUTPUT *smartmotor_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
					malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        smartmotor_doutput = (MX_SMARTMOTOR_DOUTPUT *)
			malloc( sizeof(MX_SMARTMOTOR_DOUTPUT) );

        if ( smartmotor_doutput == (MX_SMARTMOTOR_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SMARTMOTOR_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = smartmotor_doutput;
        record->class_specific_function_list
			= &mxd_smartmotor_dout_digital_output_function_list;

        digital_output->record = record;
	smartmotor_doutput->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_dout_finish_record_initialization( MX_RECORD *record )
{
        MX_SMARTMOTOR_DOUTPUT *smartmotor_doutput;
	char *port_name;
	int i;
	size_t length;
	mx_status_type mx_status;

	smartmotor_doutput = (MX_SMARTMOTOR_DOUTPUT *)
					record->record_type_struct;

	/* Force the port name to upper case. */

	port_name = smartmotor_doutput->port_name;

	length = strlen(port_name);

	for ( i = 0; i < length; i++ ) {
		if ( islower( (int)(port_name[i]) ) ) {
			port_name[i] = toupper( (int)(port_name[i]) );
		}
	}

	mx_status = mxd_smartmotor_check_port_name( record,
					smartmotor_doutput->port_name );

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_dout_read( MX_DIGITAL_OUTPUT *doutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_dout_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_smartmotor_dout_write()";

	MX_SMARTMOTOR_DOUTPUT *smartmotor_doutput;
	MX_SMARTMOTOR *smartmotor;
	char command[80];
	char *port_name;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	smartmotor = NULL;
	smartmotor_doutput = NULL;

	mx_status = mxd_smartmotor_dout_get_pointers( doutput,
				&smartmotor_doutput, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_name = smartmotor_doutput->port_name;

	if ( port_name[0] == 'U' ) {
		snprintf( command, sizeof(command),
			"%sI=%lu", port_name, doutput->value );
	} else {
		snprintf( command, sizeof(command),
			"DOUT%s,%lu", port_name, doutput->value );
	}

	mx_status = mxd_smartmotor_command( smartmotor, command,
					NULL, 0, MXD_SMARTMOTOR_DIO_DEBUG );

	return mx_status;
}

