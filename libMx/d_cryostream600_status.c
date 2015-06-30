/*
 * Name:    d_cryostream600_status.c
 *
 * Purpose: MX driver for the Oxford Cryosystems 600 series Cryostream
 *          controller.
 *
 *          Please note that this driver only reads status values.  It does
 *          not attempt to change the temperature settings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002-2006, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "mx_analog_input.h"
#include "d_cryostream600_motor.h"
#include "d_cryostream600_status.h"

MX_RECORD_FUNCTION_LIST mxd_cryostream600_status_record_function_list = {
	NULL,
	mxd_cryostream600_status_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_cryostream600_status_resynchronize
};

MX_ANALOG_INPUT_FUNCTION_LIST
	mxd_cryostream600_status_analog_input_function_list =
{
	mxd_cryostream600_status_read
};

MX_RECORD_FIELD_DEFAULTS mxd_cryostream600_status_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_CRYOSTREAM600_STATUS_STANDARD_FIELDS
};

long mxd_cryostream600_status_num_record_fields
		= sizeof( mxd_cryostream600_status_field_default )
		/ sizeof( mxd_cryostream600_status_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_cryostream600_status_rfield_def_ptr
			= &mxd_cryostream600_status_field_default[0];

#define CRYOSTREAM600_STATUS_DEBUG	FALSE

#define MAX_RESPONSE_TOKENS		10
#define RESPONSE_TOKEN_LENGTH		10

/* ===== */

MX_EXPORT mx_status_type
mxd_cryostream600_status_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_cryostream600_status_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_CRYOSTREAM600_STATUS *cryostream600_status;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        cryostream600_status = (MX_CRYOSTREAM600_STATUS *)
				malloc( sizeof(MX_CRYOSTREAM600_STATUS) );

        if ( cryostream600_status == (MX_CRYOSTREAM600_STATUS *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_CRYOSTREAM600_STATUS structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = cryostream600_status;
        record->class_specific_function_list
			= &mxd_cryostream600_status_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cryostream600_status_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_cryostream600_status_resynchronize()";

	MX_CRYOSTREAM600_STATUS *cryostream600_status;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	cryostream600_status =
		(MX_CRYOSTREAM600_STATUS *) record->record_type_struct;

	if ( cryostream600_status == (MX_CRYOSTREAM600_STATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CRYOSTREAM600_STATUS pointer for analog input '%s' is NULL",
			record->name );
	}

	mx_status = mx_resynchronize_record(
			cryostream600_status->cryostream600_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cryostream600_status_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_cryostream600_status_read()";

	MX_CRYOSTREAM600_STATUS *cryostream600_status;
	MX_CRYOSTREAM600_MOTOR *cryostream600_motor;
	MX_RECORD *cryostream600_motor_record;
	MX_DRIVER *driver;
	char driver_name[ MXU_DRIVER_NAME_LENGTH+1 ];
	char response[80];
	char **token_array;
	char *identifier, *mode, *ice_block_token;
	double set_temperature, temperature_error, final_temperature;
	double ramp_rate, evaporator_temperature;
	long tokens_returned;
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

	cryostream600_status =
		(MX_CRYOSTREAM600_STATUS *) ainput->record->record_type_struct;

	if ( cryostream600_status == (MX_CRYOSTREAM600_STATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CRYOSTREAM600_STATUS pointer for analog input '%s' is NULL",
			ainput->record->name );
	}

	cryostream600_motor_record
			= cryostream600_status->cryostream600_motor_record;

	if ( cryostream600_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"cryostream_motor_record pointer for analog input '%s' is NULL.",
			ainput->record->name );
	}

	if ( cryostream600_motor_record->mx_type != MXT_MTR_CRYOSTREAM600 ) {
		driver = mx_get_driver_by_type( MXT_MTR_CRYOSTREAM600 );

		if ( driver == (MX_DRIVER *) NULL ) {
			strlcpy( driver_name, "unknown",
					MXU_DRIVER_NAME_LENGTH );
		} else {
			strlcpy( driver_name, driver->name,
					MXU_DRIVER_NAME_LENGTH );
		}

		return mx_error( MXE_TYPE_MISMATCH, fname,
	"cryostream600_motor_record '%s' for Cryostream status record '%s' "
	"is not of type '%s'.  Instead, it is of type '%s'.",
			cryostream600_motor_record->name,
			ainput->record->name, driver_name,
			mx_get_driver_name( cryostream600_motor_record ) );
	}

	cryostream600_motor = (MX_CRYOSTREAM600_MOTOR *)
				cryostream600_motor_record->record_type_struct;

	if ( cryostream600_motor == (MX_CRYOSTREAM600_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_CRYOSTREAM600_MOTOR pointer for Cryostream motor '%s' "
	"used by Cryostream status record '%s' is NULL.",
			cryostream600_motor_record->name,
			ainput->record->name );
	}

	/* Request the status of the controller. */

	token_array = cryostream600_motor->response_token_array;

	mx_status = mxd_cryostream600_motor_command( cryostream600_motor, "S$",
						response, sizeof(response),
						CRYOSTREAM600_STATUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[0] != 'S' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Cryostream device '%s' did not return the expected "
		"status message in its response to the S$ command.  "
		"Response = '%s'.", ainput->record->name, response );
	}

	mx_status = mxd_cryostream600_motor_parse_response( cryostream600_motor,
					response, CRYOSTREAM600_STATUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	identifier = token_array[0];
	mode = token_array[1];
	set_temperature = atof( token_array[2] );
	temperature_error = atof( token_array[3] );
	final_temperature = atof( token_array[4] );
	ramp_rate = atof( token_array[5] );
	evaporator_temperature = atof( token_array[6] );
	ice_block_token = token_array[7];

	MXW_SUPPRESS_SET_BUT_NOT_USED(identifier);
	MXW_SUPPRESS_SET_BUT_NOT_USED(mode);

#if 0
	MX_DEBUG(-2,("%s: identifier = '%s'", fname, identifier));
	MX_DEBUG(-2,("%s: mode = '%s'", fname, mode));
	MX_DEBUG(-2,("%s: set_temperature = %g", fname, set_temperature));
	MX_DEBUG(-2,("%s: temperature_error = %g", fname, temperature_error));
	MX_DEBUG(-2,("%s: final_temperature = %g", fname, final_temperature));
	MX_DEBUG(-2,("%s: ramp_rate = %g", fname, ramp_rate));
	MX_DEBUG(-2,("%s: evaporator_temperature = %g",
					fname, evaporator_temperature));
	MX_DEBUG(-2,("%s: ice_block_token = '%s'", fname, ice_block_token));
#endif

	tokens_returned = cryostream600_motor->num_tokens_returned;

	switch( cryostream600_status->parameter_type ) {
	case MXT_CRYOSTREAM600_TEMPERATURE:
		ainput->raw_value.double_value =
				set_temperature + temperature_error;
		break;
	case MXT_CRYOSTREAM600_SET_TEMPERATURE:
		ainput->raw_value.double_value = set_temperature;
		break;
	case MXT_CRYOSTREAM600_TEMPERATURE_ERROR:
		ainput->raw_value.double_value = temperature_error;
		break;
	case MXT_CRYOSTREAM600_FINAL_TEMPERATURE:
		ainput->raw_value.double_value = final_temperature;
		break;
	case MXT_CRYOSTREAM600_RAMP_RATE:
		ainput->raw_value.double_value = ramp_rate;
		break;
	case MXT_CRYOSTREAM600_EVAPORATOR_TEMPERATURE:
		if ( tokens_returned >= 7 ) {
			ainput->raw_value.double_value = evaporator_temperature;
		} else {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Cryostream device '%s' did not report back what the evaporator "
	"temperature was in its response to the status command S$.",
				ainput->record->name );
		}
		break;
	case MXT_CRYOSTREAM600_ICE_BLOCK:
		if ( tokens_returned >= 8 ) {
			if ( ice_block_token[1] == '1' ) {
				ainput->raw_value.double_value = 1.0;
			} else {
				ainput->raw_value.double_value = 0.0;
			}
		} else {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Cryostream device '%s' did not report back whether or not "
	"there is an ice block in its response to the status command S$.",
				ainput->record->name );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Unrecognized parameter type %ld for 'cryostream600_status' record '%s'.  "
"The allowed values are in the range (1-7).",
			cryostream600_status->parameter_type,
			ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

