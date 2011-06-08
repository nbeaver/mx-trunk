/*
 * Name:    d_itc503_motor.c 
 *
 * Purpose: MX motor driver to control Oxford Instruments ITC503 and
 *          Cryojet temperature controllers as if they were motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2008, 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define ITC503_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_ascii.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_isobus.h"
#include "i_itc503.h"
#include "d_itc503_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_itc503_motor_record_function_list = {
	NULL,
	mxd_itc503_motor_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	mxd_itc503_motor_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_itc503_motor_motor_function_list = {
	NULL,
	mxd_itc503_motor_move_absolute,
	NULL,
	NULL,
	mxd_itc503_motor_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_itc503_motor_get_extended_status
};

/* ITC503 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_itc503_motor_recfield_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_ITC503_MOTOR_STANDARD_FIELDS
};

long mxd_itc503_motor_num_record_fields
		= sizeof( mxd_itc503_motor_recfield_defaults )
		/ sizeof( mxd_itc503_motor_recfield_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_itc503_motor_rfield_def_ptr
			= &mxd_itc503_motor_recfield_defaults[0];

/* ===== */

static mx_status_type
mxd_itc503_motor_get_pointers( MX_MOTOR *motor,
				MX_ITC503_MOTOR **itc503_motor,
				MX_ITC503 **itc503,
				MX_ISOBUS **isobus,
				const char *calling_fname )
{
	static const char fname[] = "mxd_itc503_motor_get_pointers()";

	MX_ITC503_MOTOR *itc503_motor_ptr;
	MX_RECORD *controller_record, *isobus_record;
	MX_ITC503 *itc503_ptr;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed was NULL." );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_RECORD pointer for the MX_MOTOR pointer passed was NULL." );
	}

	itc503_motor_ptr = (MX_ITC503_MOTOR *)
				motor->record->record_type_struct;

	if ( itc503_motor_ptr == (MX_ITC503_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_MOTOR pointer for analog output '%s' is NULL",
			motor->record->name );
	}

	if ( itc503_motor != (MX_ITC503_MOTOR **) NULL ) {
		*itc503_motor = itc503_motor_ptr;
	}

	controller_record = itc503_motor_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"controller_record pointer for analog output '%s' is NULL.",
			motor->record->name );
	}

	switch( controller_record->mx_type ) {
	case MXI_CTRL_ITC503:
	case MXI_CTRL_CRYOJET:
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"controller_record '%s' for ITC503 control record '%s' "
		"is not an 'itc503' record.  Instead, it is of type '%s'.",
			controller_record->name, motor->record->name,
			mx_get_driver_name( controller_record ) );
		break;
	}

	itc503_ptr = (MX_ITC503 *) controller_record->record_type_struct;

	if ( itc503_ptr == (MX_ITC503 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ITC503 pointer for ITC503 controller '%s' "
		"used by ITC503 status record '%s' is NULL.",
			controller_record->name,
			motor->record->name );
	}

	if ( itc503 != (MX_ITC503 **) NULL ) {
		*itc503 = itc503_ptr;
	}

	if ( isobus != (MX_ISOBUS **) NULL ) {
		isobus_record = itc503_ptr->isobus_record;

		if ( isobus_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The isobus_record pointer for ITC503 "
			"controller record '%s' is NULL.",
				controller_record->name );
		}

		*isobus = isobus_record->record_type_struct;

		if ( (*isobus) == (MX_ISOBUS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ISOBUS pointer for ISOBUS record '%s' "
			"is NULL.", isobus_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_itc503_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_itc503_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_ITC503_MOTOR *itc503_motor = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	itc503_motor = (MX_ITC503_MOTOR *)
				malloc( sizeof(MX_ITC503_MOTOR) );

	if ( itc503_motor == (MX_ITC503_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ITC503_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = itc503_motor;
	record->class_specific_function_list
				= &mxd_itc503_motor_motor_function_list;

	motor->record = record;
	itc503_motor->record = record;

	/* The ITC503 temperature controller motor is treated
	 * as an analog motor.
	 */

	motor->subclass = MXC_MTR_ANALOG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[]
		= "mxd_itc503_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_ITC503_MOTOR *itc503_motor = NULL;
	MX_ITC503 *itc503 = NULL;
	MX_ISOBUS *isobus = NULL;
	double position, move_deadband, busy_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_itc503_motor_get_pointers( motor,
				&itc503_motor, &itc503, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = ITC503_MOTOR.\n\n");
	fprintf(file, "  name               = %s\n", record->name);
	fprintf(file, "  ISOBUS record      = %s\n", isobus->record->name);

	busy_deadband = motor->scale * itc503_motor->busy_deadband;

	fprintf(file, "  busy deadband      = %g %s  (%g K)\n",
		busy_deadband, motor->units,
		itc503_motor->busy_deadband );

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position           = %g %s  (%g K)\n",
		motor->position, motor->units,
		motor->raw_position.analog );
	fprintf(file, "  scale              = %g %s per K.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
		motor->offset, motor->units);

	fprintf(file, "  backlash           = %g %s  (%g K)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog);

	fprintf(file, "  negative limit     = %g %s  (%g K)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit     = %g %s  (%g K)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband      = %g %s  (%g K)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_itc503_motor_move_absolute()";

	MX_ITC503_MOTOR *itc503_motor = NULL;
	MX_ITC503 *itc503 = NULL;
	MX_ISOBUS *isobus = NULL;
	char command[80];
	char response[80];
	unsigned long raw_destination;
	mx_status_type mx_status;

	mx_status = mxd_itc503_motor_get_pointers( motor,
				&itc503_motor, &itc503, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is in remote control mode before trying
	 * to send a move command by getting the controller status.
	 */

	mx_status = mxi_isobus_command( isobus, itc503->isobus_address,
					"X", response, sizeof(response),
					itc503->maximum_retries,
					ITC503_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response[4] != 'C' ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"ITC503 motor '%s' did not return the expected "
		"status message in its response to the X command.  "
		"Response = '%s'.", motor->record->name, response );
	}

	/* If the character after 'C' is either '1' or '3', we are in
	 * remote control mode and can proceed.
	 */

	if ( ( response[5] != '1' ) && ( response[5] != '3' ) ) {

		return mx_error( MXE_PERMISSION_DENIED, fname,
		"ITC503 motor '%s' cannot change the temperature "
		"since the controller is not in remote control mode.  "
		"To fix this, go to the front panel of the ITC503 "
		"controller, push the LOC/REM button, and select REMOTE.",
		motor->record->name );
	}

	/* Send the move command. */

	raw_destination = mx_round( 100.0 * motor->raw_destination.analog );

	snprintf( command, sizeof(command), "T%05lu", raw_destination );

	mx_status = mxi_isobus_command( isobus, itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the motor as being busy. */

	motor->busy = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_soft_abort( MX_MOTOR *motor )
{
	double position;
	mx_status_type mx_status;

	/* Get the current_temperature. */

	mx_status = mxd_itc503_motor_get_extended_status( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Command the ITC503 to move to the current temperature. */

	motor->raw_destination.analog = motor->raw_position.analog;

	position = motor->offset + motor->raw_position.analog * motor->scale;

	motor->position = position;
	motor->destination = position;

	mx_status = mxd_itc503_motor_move_absolute( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_itc503_motor_get_extended_status()";

	MX_ITC503_MOTOR *itc503_motor = NULL;
	MX_ITC503 *itc503 = NULL;
	MX_ISOBUS *isobus = NULL;
	char response[80];
	int num_items;
	double measured_temperature, set_temperature, temperature_error;
	mx_status_type mx_status;

	mx_status = mxd_itc503_motor_get_pointers( motor,
				&itc503_motor, &itc503, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the set temperature. */

	mx_status = mxi_isobus_command( isobus, itc503->isobus_address,
					"R0", response, sizeof(response),
					itc503->maximum_retries,
					ITC503_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "R%lg", &set_temperature );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Did not find the set temperature in the response to the R0 command "
	"sent to ITC503 controller '%s'.  Response = '%s'",
			motor->record->name, response );
	}

	/* Read the current temperature error. */

	mx_status = mxi_isobus_command( isobus, itc503->isobus_address,
					"R4", response, sizeof(response),
					itc503->maximum_retries,
					ITC503_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "R%lg", &temperature_error );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
	"Did not find the temperature error in the response to the R4 command "
	"sent to ITC503 controller '%s'.  Response = '%s'",
			motor->record->name, response );
	}

	measured_temperature = set_temperature - temperature_error;

	motor->raw_position.analog = measured_temperature;

	/* Check to see if the temperature error is less than
	 * the busy deadband.  If not, the motor is marked
	 * as busy.
	 */

	if ( fabs( temperature_error ) < itc503_motor->busy_deadband ) {
		motor->status = 0;
	} else {
		motor->status = MXSF_MTR_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

