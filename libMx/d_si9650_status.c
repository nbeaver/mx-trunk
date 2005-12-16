/*
 * Name:    d_si9650_status.c
 *
 * Purpose: MX driver for the Scientific Instruments 9650 temperature
 *          controller.
 *
 *          Please note that this driver only reads status values.  It does
 *          not attempt to change the temperature settings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define SI9650_STATUS_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "mx_analog_input.h"
#include "d_si9650_motor.h"
#include "d_si9650_status.h"

MX_RECORD_FUNCTION_LIST mxd_si9650_status_record_function_list = {
	NULL,
	mxd_si9650_status_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_si9650_status_resynchronize
};

MX_ANALOG_INPUT_FUNCTION_LIST
	mxd_si9650_status_analog_input_function_list =
{
	mxd_si9650_status_read
};

MX_RECORD_FIELD_DEFAULTS mxd_si9650_status_rf_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SI9650_STATUS_STANDARD_FIELDS
};

long mxd_si9650_status_num_record_fields
		= sizeof( mxd_si9650_status_rf_default )
		/ sizeof( mxd_si9650_status_rf_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_si9650_status_rfield_def_ptr
			= &mxd_si9650_status_rf_default[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_si9650_status_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_si9650_status_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_SI9650_STATUS *si9650_status;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        si9650_status = (MX_SI9650_STATUS *)
				malloc( sizeof(MX_SI9650_STATUS) );

        if ( si9650_status == (MX_SI9650_STATUS *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_SI9650_STATUS structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = si9650_status;
        record->class_specific_function_list
			= &mxd_si9650_status_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_si9650_status_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_si9650_status_resynchronize()";

	MX_SI9650_STATUS *si9650_status;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	si9650_status =
		(MX_SI9650_STATUS *) record->record_type_struct;

	if ( si9650_status == (MX_SI9650_STATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SI9650_STATUS pointer for analog input '%s' is NULL",
			record->name );
	}

	mx_status = mx_resynchronize_record(
			si9650_status->si9650_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_si9650_status_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_si9650_status_read()";

	MX_SI9650_STATUS *si9650_status;
	MX_SI9650_MOTOR *si9650_motor;
	MX_RECORD *si9650_motor_record;
	MX_DRIVER *driver;
	char driver_name[ MXU_DRIVER_NAME_LENGTH+1 ];
	char command[80];
	char response[80];
	int num_items;
	double status_value;
	mx_status_type mx_status;

	/* Find and check a bunch of pointers. */

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed was NULL." );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	si9650_status =
		(MX_SI9650_STATUS *) ainput->record->record_type_struct;

	if ( si9650_status == (MX_SI9650_STATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SI9650_STATUS pointer for analog input '%s' is NULL",
			ainput->record->name );
	}

	si9650_motor_record = si9650_status->si9650_motor_record;

	if ( si9650_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"si9650_motor_record pointer for analog input '%s' is NULL.",
			ainput->record->name );
	}

	if ( si9650_motor_record->mx_type != MXT_MTR_SI9650 ) {
		driver = mx_get_driver_by_type( MXT_MTR_SI9650 );

		if ( driver == (MX_DRIVER *) NULL ) {
			strlcpy( driver_name, "unknown",
					MXU_DRIVER_NAME_LENGTH );
		} else {
			strlcpy( driver_name, driver->name,
					MXU_DRIVER_NAME_LENGTH );
		}

		return mx_error( MXE_TYPE_MISMATCH, fname,
	"si9650_motor_record '%s' for SI 9650 status record '%s' "
	"is not of type '%s'.  Instead, it is of type '%s'.",
			si9650_motor_record->name,
			ainput->record->name, driver_name,
			mx_get_driver_name( si9650_motor_record ) );
	}

	si9650_motor = (MX_SI9650_MOTOR *)
				si9650_motor_record->record_type_struct;

	if ( si9650_motor == (MX_SI9650_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_SI9650_MOTOR pointer for SI 9650 motor '%s' "
	"used by SI 9650 status record '%s' is NULL.",
			si9650_motor_record->name,
			ainput->record->name );
	}

	/* Request the status of the controller. */

	switch( si9650_status->parameter_type ) {
	case MXT_SI9650_TEMPERATURE_SENSOR1:
		strcpy( command, "T" );
		break;

	case MXT_SI9650_TEMPERATURE_SENSOR2:
		strcpy( command, "t" );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Unrecognized parameter type %d for 'si9650_status' record '%s'.  "
"The allowed values are in the range (1-2).",
			si9650_status->parameter_type,
			ainput->record->name );
		break;
	}

	mx_status = mxd_si9650_motor_command( si9650_motor, command,
						response, sizeof(response),
						SI9650_STATUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &status_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response returned by SI 9650 controller '%s' "
		"to command '%s'.  Response = '%s'.",
			ainput->record->name, command, response );
	}

	ainput->raw_value.double_value = 0.1 * status_value;

	return MX_SUCCESSFUL_RESULT;
}

