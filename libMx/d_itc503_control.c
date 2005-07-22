/*
 * Name:    d_itc503_control.c
 *
 * Purpose: MX analog output driver for the Oxford Instruments ITC503
 *          temperature controller.
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

#define ITC503_CONTROL_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_analog_output.h"
#include "d_itc503_motor.h"
#include "d_itc503_control.h"

MX_RECORD_FUNCTION_LIST mxd_itc503_control_record_function_list = {
	NULL,
	mxd_itc503_control_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_itc503_control_resynchronize
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
	mxd_itc503_control_analog_output_function_list =
{
	mxd_itc503_control_read,
	mxd_itc503_control_write
};

MX_RECORD_FIELD_DEFAULTS mxd_itc503_control_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_ITC503_CONTROL_STANDARD_FIELDS
};

long mxd_itc503_control_num_record_fields
		= sizeof( mxd_itc503_control_field_default )
		/ sizeof( mxd_itc503_control_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_itc503_control_rfield_def_ptr
			= &mxd_itc503_control_field_default[0];

/* ===== */

static mx_status_type
mxd_itc503_control_get_pointers( MX_ANALOG_OUTPUT *aoutput,
				MX_ITC503_CONTROL **itc503_control,
				MX_ITC503_MOTOR **itc503_motor,
				const char *calling_fname )
{
	static const char fname[] = "mxd_itc503_control_get_pointers()";

	MX_ITC503_CONTROL *local_itc503_control;
	MX_RECORD *itc503_record;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed was NULL." );
	}

	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_ANALOG_OUTPUT pointer passed was NULL." );
	}

	local_itc503_control = (MX_ITC503_CONTROL *)
				aoutput->record->record_type_struct;

	if ( local_itc503_control == (MX_ITC503_CONTROL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_CONTROL pointer for analog output '%s' is NULL",
			aoutput->record->name );
	}

	if ( itc503_control != (MX_ITC503_CONTROL **) NULL ) {
		*itc503_control = local_itc503_control;
	}

	itc503_record = local_itc503_control->itc503_record;

	if ( itc503_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"cryostream_motor_record pointer for analog output '%s' is NULL.",
			aoutput->record->name );
	}

	if ( itc503_record->mx_type != MXT_MTR_ITC503 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"itc503_record '%s' for ITC503 status record '%s' "
	"is not an 'itc503_motor' record.  Instead, it is of type '%s'.",
			itc503_record->name, aoutput->record->name,
			mx_get_driver_name( itc503_record ) );
	}

	if ( itc503_motor != (MX_ITC503_MOTOR **) NULL ) {
		*itc503_motor = (MX_ITC503_MOTOR *)
				itc503_record->record_type_struct;

		if ( (*itc503_motor) == (MX_ITC503_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ITC503_MOTOR pointer for ITC503 motor '%s' "
	"used by ITC503 status record '%s' is NULL.",
				itc503_record->name,
				aoutput->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_itc503_control_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_itc503_control_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_ITC503_CONTROL *itc503_control;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        itc503_control = (MX_ITC503_CONTROL *)
				malloc( sizeof(MX_ITC503_CONTROL) );

        if ( itc503_control == (MX_ITC503_CONTROL *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_ITC503_CONTROL structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = itc503_control;
        record->class_specific_function_list
			= &mxd_itc503_control_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_control_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_itc503_control_resynchronize()";

	MX_ITC503_CONTROL *itc503_control;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	itc503_control =
		(MX_ITC503_CONTROL *) record->record_type_struct;

	if ( itc503_control == (MX_ITC503_CONTROL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_CONTROL pointer for analog output '%s' is NULL",
			record->name );
	}

	mx_status = mx_resynchronize_record( itc503_control->itc503_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_control_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_itc503_control_read()";

	MX_ITC503_CONTROL *itc503_control;
	MX_ITC503_MOTOR *itc503_motor;
	char command[80];
	char response[80];
	char buffer[5];
	char parameter_type;
	double double_value;
	int num_items, parse_failure;
	mx_status_type mx_status;

	mx_status = mxd_itc503_control_get_pointers( aoutput,
				&itc503_control, &itc503_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	parameter_type = itc503_control->parameter_type;

	if ( islower( (int)parameter_type ) ) {
		parameter_type = toupper( parameter_type );
	}

	parse_failure = FALSE;

	switch( parameter_type ) {
	case 'A':	/* Auto/manual status */

		strcpy( command, "X" );

		mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_CONTROL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( response[0] != 'X' )
		  || ( response[2] != 'A' ) )
		{
			parse_failure = TRUE;
		} else {
			buffer[0] = response[3];
			buffer[1] = '\0';

			double_value = atof( buffer );
		}
		break;

	case 'C':	/* Local/remote/lock status */

		strcpy( command, "X" );

		mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_CONTROL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( response[0] != 'X' )
		  || ( response[2] != 'C' ) )
		{
			parse_failure = TRUE;
		} else {
			buffer[0] = response[3];
			buffer[1] = '\0';

			double_value = atof( buffer );
		}
		break;

	case 'G':	/* Gas flow */

		strcpy( command, "R7" );

		mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_CONTROL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	case 'O':	/* Heater output volts */

		strcpy( command, "R5" );

		mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_CONTROL_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Unsupported ITC503 control parameter type '%c' for record '%s'.",
			itc503_control->parameter_type, aoutput->record->name );
		break;
	}

	if ( parse_failure ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not parse the response by ITC503 controller '%s' to "
		"the command '%s'.  Response = '%s'",
			itc503_motor->record->name, command, response );
	}

	aoutput->raw_value.double_value = double_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_control_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_itc503_control_write()";

	MX_ITC503_CONTROL *itc503_control;
	MX_ITC503_MOTOR *itc503_motor;
	char command[80];
	char response[80];
	long parameter_value;
	char parameter_type;
	mx_status_type mx_status;

	mx_status = mxd_itc503_control_get_pointers( aoutput,
				&itc503_control, &itc503_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	parameter_type = itc503_control->parameter_type;

	if ( islower( (int)parameter_type ) ) {
		parameter_type = toupper( parameter_type );
	}

	switch( parameter_type ) {
	case 'A':	/* Auto/manual command */

		parameter_value = mx_round( aoutput->raw_value.double_value );

		if ( ( parameter_value < 0 )
		  || ( parameter_value > 3 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'A' auto/manual control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0 to 3.",
				aoutput->raw_value.double_value,
				aoutput->record->name );
		}

		sprintf( command, "A%ld", parameter_value );

		mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_CONTROL_DEBUG );
		break;

	case 'C':	/* Local/remote/lock command */

		parameter_value = mx_round( aoutput->raw_value.double_value );

		if ( ( parameter_value < 0 )
		  || ( parameter_value > 3 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'C' local/remote/lock control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0 to 3.",
				aoutput->raw_value.double_value,
				aoutput->record->name );
		}

		sprintf( command, "C%ld", parameter_value );

		mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_CONTROL_DEBUG );
		break;

	case 'G':	/* Gas flow command */

		parameter_value =
			mx_round( 10.0 * aoutput->raw_value.double_value );

		if ( ( parameter_value < 0 )
		  || ( parameter_value > 999 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'G' gas flow control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 99.9.",
				aoutput->raw_value.double_value,
				aoutput->record->name );
		}

		sprintf( command, "G%03ld", parameter_value );

		mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_CONTROL_DEBUG );
		break;
		
	case 'O':	/* Heater output command */

		parameter_value =
			mx_round( 10.0 * aoutput->raw_value.double_value );

		if ( ( parameter_value < 0 )
		  || ( parameter_value > 999 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'O' heater output control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 99.9.",
				aoutput->raw_value.double_value,
				aoutput->record->name );
		}

		sprintf( command, "O%03ld", parameter_value );

		mx_status = mxd_itc503_motor_command( itc503_motor, command,
						response, sizeof(response),
						ITC503_CONTROL_DEBUG );
		break;
		
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Unsupported ITC503 control parameter type '%c' for record '%s'.",
			itc503_control->parameter_type, aoutput->record->name );
		break;
	}
	return mx_status;
}

