/*
 * Name:    d_si9650_motor.c 
 *
 * Purpose: MX motor driver to control Scientific Instruments 9650
 *          temperature controllers as if they were motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define SI9650_MOTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "d_si9650_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_si9650_motor_record_function_list = {
	NULL,
	mxd_si9650_motor_create_record_structures,
	mxd_si9650_motor_finish_record_initialization,
	NULL,
	mxd_si9650_motor_print_motor_structure,
	NULL,
	NULL,
	mxd_si9650_motor_open,
	NULL,
	NULL,
	mxd_si9650_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_si9650_motor_motor_function_list = {
	NULL,
	mxd_si9650_motor_move_absolute,
	NULL,
	NULL,
	mxd_si9650_motor_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler,
	NULL,
	NULL,
	mxd_si9650_motor_get_extended_status
};

#define MAX_RESPONSE_TOKENS             10
#define RESPONSE_TOKEN_LENGTH           10

/* Scientific Instruments 9650 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_si9650_motor_recfield_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SI9650_MOTOR_STANDARD_FIELDS
};

long mxd_si9650_motor_num_record_fields
		= sizeof( mxd_si9650_motor_recfield_defaults )
		/ sizeof( mxd_si9650_motor_recfield_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_si9650_motor_rfield_def_ptr
			= &mxd_si9650_motor_recfield_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_si9650_motor_get_pointers( MX_MOTOR *motor,
			MX_SI9650_MOTOR **si9650_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_si9650_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( si9650_motor == (MX_SI9650_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SI9650_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MOTOR pointer passed is NULL." );
	}

	*si9650_motor = (MX_SI9650_MOTOR *) motor->record->record_type_struct;

	if ( *si9650_motor == (MX_SI9650_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SI9650_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_si9650_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_si9650_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_SI9650_MOTOR *si9650_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	si9650_motor = (MX_SI9650_MOTOR *)
				malloc( sizeof(MX_SI9650_MOTOR) );

	if ( si9650_motor == (MX_SI9650_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SI9650_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = si9650_motor;
	record->class_specific_function_list
				= &mxd_si9650_motor_motor_function_list;

	motor->record = record;
	si9650_motor->motor = motor;

	/* The Scientific Instruments 9650 temperature controller motor
	 * is treated as an analog motor.
	 */

	motor->subclass = MXC_MTR_ANALOG;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_si9650_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_si9650_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_SI9650_MOTOR *si9650_motor;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We do not need to check the validity of the motor pointer since
	 * mx_motor_finish_record_initialization() will have done it for us.
	 */

	motor = (MX_MOTOR *) record->record_class_struct;

	/* But we should check the si9650_motor pointer, just in case
	 * something has been overwritten.
	 */

	si9650_motor = (MX_SI9650_MOTOR *) record->record_type_struct;

	if ( si9650_motor == (MX_SI9650_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SI9650_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	/* Set the initial speeds and accelerations.  Please note that
	 * while the controller itself uses ramp rates in K per minute,
	 * the MX routines themselves use speeds in raw units per second.
	 * Thus, we have to convert the controller ramp rate into an
	 * internal K per second value.
	 */

	motor->raw_speed = si9650_motor->maximum_ramp_rate / 60.0;
	motor->raw_base_speed = 0.0;
	motor->raw_maximum_speed = 1.0e30;  /* Large enough not to matter. */

	motor->speed = fabs(motor->scale) * motor->raw_speed;
	motor->base_speed = fabs(motor->scale) * motor->raw_base_speed;
	motor->maximum_speed = fabs(motor->scale) * motor->raw_maximum_speed;

	motor->raw_acceleration_parameters[0] = 0.0;
	motor->raw_acceleration_parameters[1] = 0.0;
	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_si9650_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[]
		= "mxd_si9650_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_SI9650_MOTOR *si9650_motor;
	MX_INTERFACE *controller_interface;
	double position, move_deadband, busy_deadband;
	double speed, ramp_rate;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_si9650_motor_get_pointers(motor, &si9650_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	controller_interface = &(si9650_motor->controller_interface);

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = SI9650_MOTOR.\n\n");
	fprintf(file, "  name               = %s\n", record->name);

	if ( controller_interface->record->mx_class == MXI_RS232 ) {
		fprintf(file,
		      "  RS-232 interface   = %s\n",
		      controller_interface->record->name );

	} else if ( controller_interface->record->mx_class == MXI_GPIB ) {
		fprintf(file,
		      "  GPIB interface     = %s\n",
		      controller_interface->record->name );
		fprintf(file,
		      "  GPIB address       = %ld\n",
		      controller_interface->address );
	} else {
		fprintf(file,
		      "  Unsupported interface record '%s'\n",
		      controller_interface->record->name );
	}

	speed = motor->scale * motor->raw_speed;
	ramp_rate = 3600.0 * motor->raw_speed;

	fprintf(file, "  ramp rate          = %g %s/sec  (%g K/sec, %g K/min)\n",
		speed, motor->units,
		motor->raw_speed, ramp_rate );

	busy_deadband = motor->scale * si9650_motor->busy_deadband;

	fprintf(file, "  busy deadband      = %g %s  (%g K)\n",
		busy_deadband, motor->units,
		si9650_motor->busy_deadband );

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
mxd_si9650_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_si9650_motor_open()";

	MX_SI9650_MOTOR *si9650_motor;
	MX_INTERFACE *controller_interface;
	char response[80];
	int num_items;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	si9650_motor =
		(MX_SI9650_MOTOR *) record->record_type_struct;

	if ( si9650_motor == (MX_SI9650_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SI9650_MOTOR pointer for analog input '%s' is NULL",
			record->name );
	}

	controller_interface = &(si9650_motor->controller_interface);

	/* Throw away any unread and unwritten characters. */

	if ( controller_interface->record->mx_class == MXI_RS232 ) {
		mx_status = mx_rs232_discard_unread_input(
			controller_interface->record,
			SI9650_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_discard_unwritten_output(
			controller_interface->record,
			SI9650_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Ask for the analog sensor number. */

	mx_status = mxd_si9650_motor_command( si9650_motor, "s",
						response, sizeof(response),
						SI9650_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &(si9650_motor->sensor_number) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not see a temperature sensor number in the response by "
	"SI 9650 controller '%s' to an 's' command.  Response = '%s'.",
			record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_si9650_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_si9650_motor_move_absolute()";

	MX_SI9650_MOTOR *si9650_motor;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_si9650_motor_get_pointers( motor,
						&si9650_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the move command. */

	sprintf( command, "S04%ld", 
			mx_round( 10.0 * motor->raw_destination.analog ) );

	mx_status = mxd_si9650_motor_command( si9650_motor,
					command, NULL, 0, SI9650_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the motor as being busy. */

	motor->busy = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_si9650_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_si9650_motor_soft_abort()";

	MX_SI9650_MOTOR *si9650_motor;
	double position;
	mx_status_type mx_status;

	mx_status = mxd_si9650_motor_get_pointers( motor,
					&si9650_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current temperature. */

	mx_status = mx_motor_get_position( motor->record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Change the temperature setpoint to the current temperature. */

	mx_status = mx_motor_move_absolute( motor->record,
						position, MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the motor as not busy. */

	motor->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_si9650_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[]
		= "mxd_si9650_motor_get_extended_status()";

	MX_SI9650_MOTOR *si9650_motor;
	char command[40];
	char response[80];
	int num_items;
	double temperature_value, difference;
	mx_status_type mx_status;

	mx_status = mxd_si9650_motor_get_pointers( motor,
					&si9650_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the temperature of the control sensor. */

	if ( si9650_motor->sensor_number == 1 ) {
		strcpy( command, "T" );
	} else {
		strcpy( command, "t" );
	}

	mx_status = mxd_si9650_motor_command( si9650_motor, command,
						response, sizeof(response),
						SI9650_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &temperature_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see a temperature value in the response by "
		"SI 9650 controller '%s' to the command '%s'.  "
		"Response = '%s'", motor->record->name, command, response );
	}

	motor->raw_position.analog = temperature_value;

	/* If the motor is closer to the destination than the busy deadband,
	 * then the motor is not busy.
	 */

	if ( motor->busy ) {
		difference = motor->raw_destination.analog
				- motor->raw_position.analog;

		if ( fabs(difference) < si9650_motor->busy_deadband ) {
			motor->busy = FALSE;
		}
	}

	/* Set the motor status flags. */

	motor->status = 0;

	if ( motor->busy ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

/********/

MX_EXPORT mx_status_type
mxd_si9650_motor_command( MX_SI9650_MOTOR *si9650_motor,
						char *command,
						char *response,
						size_t max_response_length,
						int debug_flag )
{
	static const char fname[] = "mxd_si9650_motor_command()";

	MX_INTERFACE *controller_interface;
	int i, max_retries;
	unsigned long wait_ms, num_input_bytes_available;
	mx_status_type mx_status;

	if ( si9650_motor == (MX_SI9650_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SI9650_MOTOR pointer passed was NULL." );
	}
	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	controller_interface = &(si9650_motor->controller_interface);

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending command '%s'.", fname, command));
	}

	if ( controller_interface->record->mx_class == MXI_GPIB ) {
		mx_status = mx_gpib_putline( controller_interface->record,
						controller_interface->address,
						command, NULL,
						SI9650_MOTOR_DEBUG );
	} else {
		mx_status = mx_rs232_putline( controller_interface->record,
						command, NULL,
						SI9650_MOTOR_DEBUG );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If no response is expected, we may return now. */

	if ( response == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the response. */

	if ( controller_interface->record->mx_class == MXI_GPIB ) {
		mx_status = mx_gpib_getline( controller_interface->record,
						controller_interface->address,
						response, max_response_length,
						NULL, SI9650_MOTOR_DEBUG );
	} else {

		/* Wait for the response. */

		max_retries = 50;
		wait_ms = 100;

		for ( i = 0; i <= max_retries; i++ ) {
			mx_status = mx_rs232_num_input_bytes_available(
						controller_interface->record,
						&num_input_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_input_bytes_available > 0 ) 
				break;		 /* Exit the for() loop. */

			mx_msleep( wait_ms );
		}

		if ( i > max_retries ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Timed out waiting for a response to the command '%s' from "
	"the Scientific Instruments 9650 controller attached "
	"to RS-232 port '%s'.  "
	"Are you sure it is connected and turned on?",
			command, controller_interface->record->name );
		}

		/* Read the response. */

		mx_status = mx_rs232_getline( controller_interface->record,
				response, max_response_length, NULL, 0 );
	}

	return mx_status;
}

