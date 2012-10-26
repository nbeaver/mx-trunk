/*
 * Name:    d_smartmotor_aio.c
 *
 * Purpose: MX analog input driver to read the analog input and output ports
 *          attached to an Animatics SmartMotor motor controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2008, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SMARTMOTOR_AIO_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "d_smartmotor.h"
#include "d_smartmotor_aio.h"

/* Initialize the SmartMotor analog I/O driver jump tables. */

MX_RECORD_FUNCTION_LIST mxd_smartmotor_ain_record_function_list = {
	NULL,
	mxd_smartmotor_ain_create_record_structures,
	mxd_smartmotor_ain_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_smartmotor_ain_analog_input_function_list = {
	mxd_smartmotor_ain_read
};

MX_RECORD_FIELD_DEFAULTS mxd_smartmotor_ain_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SMARTMOTOR_AINPUT_STANDARD_FIELDS
};

long mxd_smartmotor_ain_num_record_fields
		= sizeof( mxd_smartmotor_ain_record_field_defaults )
			/ sizeof( mxd_smartmotor_ain_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_ain_rfield_def_ptr
			= &mxd_smartmotor_ain_record_field_defaults[0];

/* === */

MX_RECORD_FUNCTION_LIST mxd_smartmotor_aout_record_function_list = {
	NULL,
	mxd_smartmotor_aout_create_record_structures,
	mxd_smartmotor_aout_finish_record_initialization
};

MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_smartmotor_aout_analog_output_function_list
  = {
	mxd_smartmotor_aout_read,
	mxd_smartmotor_aout_write
};

MX_RECORD_FIELD_DEFAULTS mxd_smartmotor_aout_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_SMARTMOTOR_AOUTPUT_STANDARD_FIELDS
};

long mxd_smartmotor_aout_num_record_fields
		= sizeof( mxd_smartmotor_aout_record_field_defaults )
			/ sizeof( mxd_smartmotor_aout_record_field_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_smartmotor_aout_rfield_def_ptr
			= &mxd_smartmotor_aout_record_field_defaults[0];

static mx_status_type
mxd_smartmotor_ain_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_SMARTMOTOR_AINPUT **smartmotor_ainput,
			MX_SMARTMOTOR **smartmotor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_smartmotor_ain_get_pointers()";

	MX_RECORD *smartmotor_record;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor_ainput == (MX_SMARTMOTOR_AINPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SMARTMOTOR_AINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor == (MX_SMARTMOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SMARTMOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*smartmotor_ainput = (MX_SMARTMOTOR_AINPUT *)
				ainput->record->record_type_struct;

	if ( *smartmotor_ainput == (MX_SMARTMOTOR_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SMARTMOTOR_AINPUT pointer for record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	smartmotor_record = (*smartmotor_ainput)->smartmotor_record;

	if ( smartmotor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SMARTMOTOR pointer for SMARTMOTOR analog input "
		"record '%s' passed by '%s' is NULL.",
			ainput->record->name, calling_fname );
	}

	if ( smartmotor_record->mx_type != MXT_MTR_SMARTMOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"smartmotor_record '%s' for SmartMotor analog input '%s' "
		"is not a SmartMotor record.  Instead, it is a '%s' record.",
			smartmotor_record->name, ainput->record->name,
			mx_get_driver_name( smartmotor_record ) );
	}

	*smartmotor = (MX_SMARTMOTOR *) smartmotor_record->record_type_struct;

	if ( *smartmotor == (MX_SMARTMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SMARTMOTOR pointer for SmartMotor record '%s' used by "
	"SmartMotor analog input record '%s' and passed by '%s' is NULL.",
			smartmotor_record->name,
			ainput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_smartmotor_aout_get_pointers( MX_ANALOG_OUTPUT *aoutput,
			MX_SMARTMOTOR_AOUTPUT **smartmotor_aoutput,
			MX_SMARTMOTOR **smartmotor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_smartmotor_aout_get_pointers()";

	MX_RECORD *smartmotor_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor_aoutput == (MX_SMARTMOTOR_AOUTPUT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SMARTMOTOR_AOUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( smartmotor == (MX_SMARTMOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SMARTMOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_ANALOG_OUTPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*smartmotor_aoutput = (MX_SMARTMOTOR_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( *smartmotor_aoutput == (MX_SMARTMOTOR_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SMARTMOTOR_AOUTPUT pointer for record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	smartmotor_record = (*smartmotor_aoutput)->smartmotor_record;

	if ( smartmotor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SMARTMOTOR pointer for SMARTMOTOR analog output "
		"record '%s' passed by '%s' is NULL.",
			aoutput->record->name, calling_fname );
	}

	if ( smartmotor_record->mx_type != MXT_MTR_SMARTMOTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"smartmotor_record '%s' for SmartMotor analog output '%s' "
		"is not a SmartMotor record.  Instead, it is a '%s' record.",
			smartmotor_record->name, aoutput->record->name,
			mx_get_driver_name( smartmotor_record ) );
	}

	*smartmotor = (MX_SMARTMOTOR *) smartmotor_record->record_type_struct;

	if ( *smartmotor == (MX_SMARTMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SMARTMOTOR pointer for SmartMotor record '%s' used by "
	"SmartMotor analog output record '%s' and passed by '%s' is NULL.",
			smartmotor_record->name,
			aoutput->record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Input functions. ===== */

MX_EXPORT mx_status_type
mxd_smartmotor_ain_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_smartmotor_ain_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_SMARTMOTOR_AINPUT *smartmotor_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        smartmotor_ainput = (MX_SMARTMOTOR_AINPUT *)
				malloc( sizeof(MX_SMARTMOTOR_AINPUT) );

        if ( smartmotor_ainput == (MX_SMARTMOTOR_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SMARTMOTOR_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = smartmotor_ainput;
        record->class_specific_function_list
			= &mxd_smartmotor_ain_analog_input_function_list;

        analog_input->record = record;
	smartmotor_ainput->record = record;

	/* Raw analog input values are stored as longs. */

	analog_input->subclass = MXT_AIN_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_ain_finish_record_initialization( MX_RECORD *record )
{
        MX_SMARTMOTOR_AINPUT *smartmotor_ainput;
	char *port_name;
	int i;
	size_t length;
	mx_status_type mx_status;

	smartmotor_ainput = (MX_SMARTMOTOR_AINPUT *) record->record_type_struct;

	/* Force the port name to upper case. */

	port_name = smartmotor_ainput->port_name;

	length = strlen(port_name);

	for ( i = 0; i < length; i++ ) {
		if ( islower( (int)(port_name[i]) ) ) {
			port_name[i] = toupper( (int)(port_name[i]) );
		}
	}

	mx_status = mxd_smartmotor_check_port_name( record,
					smartmotor_ainput->port_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_analog_input_finish_record_initialization( record );

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_ain_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_smartmotor_ain_read()";

	MX_SMARTMOTOR_AINPUT *smartmotor_ainput;
	MX_SMARTMOTOR *smartmotor;
	char command[80];
	char response[80];
	char *port_name;
	int num_items;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	smartmotor = NULL;
	smartmotor_ainput = NULL;

	mx_status = mxd_smartmotor_ain_get_pointers( ainput,
				&smartmotor_ainput, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_name = smartmotor_ainput->port_name;

	if ( port_name[0] == 'U' ) {
		sprintf( command, "z=%sA Rz", port_name );

	} else if ( strcmp( port_name, "TEMP" ) == 0 ) {
		sprintf( command, "z=TEMP Rz" );
	} else {
		sprintf( command, "RAIN%s", port_name );
	}

	mx_status = mxd_smartmotor_command( smartmotor, command,
					response, sizeof( response ),
					MXD_SMARTMOTOR_AIO_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf(response, "%ld", &(ainput->raw_value.long_value));

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Could not find a numerical value in the response to a '%s' "
		"command to SmartMotor controller '%s' for "
		"analog input record '%s'.  Response = '%s'",
			command,
			smartmotor->record->name,
			ainput->record->name,
			response );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== Output functions. ===== */

MX_EXPORT mx_status_type
mxd_smartmotor_aout_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_smartmotor_aout_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_SMARTMOTOR_AOUTPUT *smartmotor_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        smartmotor_aoutput = (MX_SMARTMOTOR_AOUTPUT *)
				malloc( sizeof(MX_SMARTMOTOR_AOUTPUT) );

        if ( smartmotor_aoutput == (MX_SMARTMOTOR_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SMARTMOTOR_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = smartmotor_aoutput;
        record->class_specific_function_list
			= &mxd_smartmotor_aout_analog_output_function_list;

        analog_output->record = record;
	smartmotor_aoutput->record = record;

	/* Raw analog output values are stored as longs. */

	analog_output->subclass = MXT_AOU_LONG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_aout_finish_record_initialization( MX_RECORD *record )
{
        MX_SMARTMOTOR_AOUTPUT *smartmotor_aoutput;
	char *port_name;
	int i;
	size_t length;
	mx_status_type mx_status;

	smartmotor_aoutput = (MX_SMARTMOTOR_AOUTPUT *)
					record->record_type_struct;

	/* Force the port name to upper case. */

	port_name = smartmotor_aoutput->port_name;

	length = strlen(port_name);

	for ( i = 0; i < length; i++ ) {
		if ( islower( (int)(port_name[i]) ) ) {
			port_name[i] = toupper( (int)(port_name[i]) );
		}
	}

	mx_status = mxd_smartmotor_check_port_name( record,
					smartmotor_aoutput->port_name );

        return mx_status;
}

MX_EXPORT mx_status_type
mxd_smartmotor_aout_read( MX_ANALOG_OUTPUT *aoutput )
{
	/* Just return the last value written. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_smartmotor_aout_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_smartmotor_aout_write()";

	MX_SMARTMOTOR_AOUTPUT *smartmotor_aoutput;
	MX_SMARTMOTOR *smartmotor;
	char command[80];
	char response[80];
	char *port_name;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	smartmotor = NULL;
	smartmotor_aoutput = NULL;

	mx_status = mxd_smartmotor_aout_get_pointers( aoutput,
				&smartmotor_aoutput, &smartmotor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	port_name = smartmotor_aoutput->port_name;

	if ( port_name[0] == 'U' ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"SmartMotor pin '%s' cannot be used as an analog output.",
			port_name );

	} else {
		sprintf( command, "AOUT%s,%ld",
			port_name, (long) aoutput->raw_value.long_value );
	}

	mx_status = mxd_smartmotor_command( smartmotor, command,
					response, sizeof( response ),
					MXD_SMARTMOTOR_AIO_DEBUG );

	return mx_status;
}

