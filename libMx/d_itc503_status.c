/*
 * Name:    d_itc503_status.c
 *
 * Purpose: MX analog input driver for the Oxford Instruments ITC503
 *          temperature controller.
 *
 *          Please note that this driver only reads status values.  It does
 *          not attempt to change the temperature settings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define ITC503_STATUS_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_analog_input.h"
#include "i_isobus.h"
#include "d_itc503_motor.h"
#include "d_itc503_status.h"

MX_RECORD_FUNCTION_LIST mxd_itc503_status_record_function_list = {
	NULL,
	mxd_itc503_status_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_itc503_status_resynchronize
};

MX_ANALOG_INPUT_FUNCTION_LIST
	mxd_itc503_status_analog_input_function_list =
{
	mxd_itc503_status_read
};

MX_RECORD_FIELD_DEFAULTS mxd_itc503_status_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_ITC503_STATUS_STANDARD_FIELDS
};

long mxd_itc503_status_num_record_fields
		= sizeof( mxd_itc503_status_field_default )
		/ sizeof( mxd_itc503_status_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_itc503_status_rfield_def_ptr
			= &mxd_itc503_status_field_default[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_itc503_status_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_itc503_status_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_ITC503_STATUS *itc503_status;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        itc503_status = (MX_ITC503_STATUS *)
				malloc( sizeof(MX_ITC503_STATUS) );

        if ( itc503_status == (MX_ITC503_STATUS *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_ITC503_STATUS structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = itc503_status;
        record->class_specific_function_list
			= &mxd_itc503_status_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_status_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_itc503_status_resynchronize()";

	MX_ITC503_STATUS *itc503_status;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	itc503_status =
		(MX_ITC503_STATUS *) record->record_type_struct;

	if ( itc503_status == (MX_ITC503_STATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_STATUS pointer for analog input '%s' is NULL",
			record->name );
	}

	mx_status = mx_resynchronize_record(
			itc503_status->itc503_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_status_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_itc503_status_read()";

	MX_ITC503_STATUS *itc503_status;
	MX_ITC503_MOTOR *itc503_motor;
	MX_RECORD *itc503_motor_record;
	MX_RECORD *isobus_record;
	MX_ISOBUS *isobus;
	char command[80];
	char response[80];
	int num_items;
	double double_value;
	mx_status_type mx_status;

	/*-----------------------------------------------------*/

	/* Find and check a bunch of pointers. */

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed was NULL." );
	}
	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	itc503_status = (MX_ITC503_STATUS *) ainput->record->record_type_struct;

	if ( itc503_status == (MX_ITC503_STATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_STATUS pointer for analog input '%s' is NULL",
			ainput->record->name );
	}

	itc503_motor_record = itc503_status->itc503_motor_record;

	if ( itc503_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"itc503_motor_record pointer for analog input '%s' is NULL.",
			ainput->record->name );
	}

	if ( itc503_motor_record->mx_type != MXT_MTR_ITC503 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"itc503_motor_record '%s' for ITC503 status record '%s' "
	"is not an 'itc503_motor' record.  Instead, it is of type '%s'.",
			itc503_motor_record->name, ainput->record->name,
			mx_get_driver_name( itc503_motor_record ) );
	}

	itc503_motor = (MX_ITC503_MOTOR *)
				itc503_motor_record->record_type_struct;

	if ( itc503_motor == (MX_ITC503_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ITC503_MOTOR pointer for ITC503 motor '%s' "
		"used by ITC503 status record '%s' is NULL.",
			itc503_motor_record->name,
			ainput->record->name );
	}

	isobus_record = itc503_motor->isobus_record;

	if ( isobus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The isobus_record pointer for ITC503 motor '%s' is NULL.",
			itc503_motor_record->name );
	}

	isobus = isobus_record->record_type_struct;

	if ( isobus == (MX_ISOBUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ISOBUS pointer for ISOBUS record '%s' is NULL.",
			isobus_record->name );
	}

	/*-----------------------------------------------------*/

	if ( ( itc503_status->parameter_type < 0 )
	  || ( itc503_status->parameter_type > 13 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal parameter type %ld requested for ITC503 status record '%s'.  "
	"Allowed parameter types are from 0 to 13.  Read the description of "
	"the 'R' command in the ITC503 manual for more information.",
			itc503_status->parameter_type, ainput->record->name );
	}

	/* The requested parameter type is used directly to construct
	 * an ITC503 'R' command.
	 */

	snprintf( command, sizeof(command),
			"R%ld", itc503_status->parameter_type );

	/* Send the READ command to the controller. */

	mx_status = mxi_isobus_command( isobus,
					itc503_motor->isobus_address,
					command, response, sizeof(response),
					itc503_motor->maximum_retries,
					ITC503_STATUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "R%lg", &double_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not parse the response by ITC503 controller '%s' to "
		"the READ command '%s'.  Response = '%s'",
			itc503_motor->record->name, command, response );
	}

	ainput->raw_value.double_value = double_value;

	return MX_SUCCESSFUL_RESULT;
}

