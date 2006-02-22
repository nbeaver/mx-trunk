/*
 * Name:    d_newport.c
 *
 * Purpose: MX driver for the Newport MM3000, MM4000/4005 and ESP series
 *          of multiaxis motor controllers.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define NEWPORT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "d_newport.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_newport_record_function_list = {
	NULL,
	mxd_newport_create_record_structures,
	mxd_newport_finish_record_initialization,
	NULL,
	mxd_newport_print_structure,
	NULL,
	NULL,
	mxd_newport_open,
	NULL,
	NULL,
	mxd_newport_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_newport_motor_function_list = {
	mxd_newport_motor_is_busy,
	mxd_newport_move_absolute,
	mxd_newport_get_position,
	mxd_newport_set_position,
	mxd_newport_soft_abort,
	mxd_newport_immediate_abort,
	mxd_newport_positive_limit_hit,
	mxd_newport_negative_limit_hit,
	mxd_newport_find_home_position,
	mxd_newport_constant_velocity_move,
	mxd_newport_get_parameter,
	mxd_newport_set_parameter
};

/* MM3000 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mm3000_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_NEWPORT_STANDARD_FIELDS
};

long mxd_mm3000_num_record_fields
		= sizeof( mxd_mm3000_record_field_defaults )
			/ sizeof( mxd_mm3000_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mm3000_rfield_def_ptr
			= &mxd_mm3000_record_field_defaults[0];

/* MM4000 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mm4000_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_NEWPORT_STANDARD_FIELDS
};

long mxd_mm4000_num_record_fields
		= sizeof( mxd_mm4000_record_field_defaults )
			/ sizeof( mxd_mm4000_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mm4000_rfield_def_ptr
			= &mxd_mm4000_record_field_defaults[0];

/* ESP motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_esp_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_NEWPORT_STANDARD_FIELDS
};

long mxd_esp_num_record_fields
		= sizeof( mxd_esp_record_field_defaults )
			/ sizeof( mxd_esp_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_esp_rfield_def_ptr
			= &mxd_esp_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_newport_get_pointers( MX_MOTOR *motor,
			MX_NEWPORT_MOTOR **newport_motor,
			MX_NEWPORT **newport,
			const char *calling_fname )
{
	const char fname[] = "mxd_newport_get_pointers()";

	MX_RECORD *newport_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( newport_motor == (MX_NEWPORT_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NEWPORT_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( newport == (MX_NEWPORT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NEWPORT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*newport_motor = (MX_NEWPORT_MOTOR *) motor->record->record_type_struct;

	if ( *newport_motor == (MX_NEWPORT_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NEWPORT_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	newport_record = (*newport_motor)->newport_record;

	if ( newport_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The newport_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*newport = (MX_NEWPORT *) newport_record->record_type_struct;

	if ( *newport == (MX_NEWPORT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NEWPORT pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*============================================*/

MX_EXPORT mx_status_type
mxd_newport_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_newport_create_record_structures()";

	MX_MOTOR *motor;
	MX_NEWPORT_MOTOR *newport_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	newport_motor = (MX_NEWPORT_MOTOR *) malloc( sizeof(MX_NEWPORT_MOTOR) );

	if ( newport_motor == (MX_NEWPORT_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NEWPORT_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = newport_motor;
	record->class_specific_function_list
				= &mxd_newport_motor_function_list;

	motor->record = record;

	/* This record treats MM3000 motors as if they were steppers with
	 * the encoder tick size as the step size.
	 *
	 * MM4000s are treated as analog motors, since only positioning
	 * commands in engineering units are available.
	 */

	if ( record->mx_type == MXT_MTR_MM3000 ) {
		motor->subclass = MXC_MTR_STEPPER;

	} else {	/* ESP and MM4000 */

		motor->subclass = MXC_MTR_ANALOG;
	}

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_newport_finish_record_initialization()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if the axis number is valid. */

	newport_motor = (MX_NEWPORT_MOTOR *) (record->record_type_struct);

	if ( newport_motor == (MX_NEWPORT_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NEWPORT_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	if ( ( newport_motor->axis_number < 1 )
	  || ( newport_motor->axis_number > 4 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Illegal axis number %d for MX_NEWPORT_MOTOR.  Allowed range from 1 to 4.",
			newport_motor->axis_number );
	}

	/* Save a pointer to the motor's MX_RECORD structure in the MX_NEWPORT
	 * structure, so that the four motors can find each other.
	 */

	if ( newport_motor->newport_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"NEWPORT interface record pointer is NULL for motor '%s'.",
			record->name );
	}

	newport = (MX_NEWPORT *)
		(newport_motor->newport_record->record_type_struct);

	if ( newport == (MX_NEWPORT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NEWPORT pointer for interface record '%s' is NULL.",
			newport_motor->newport_record->name );
	}

	newport->motor_record[ newport_motor->axis_number - 1 ] = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_newport_print_structure()";

	MX_MOTOR *motor;
	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	if ( record->mx_type == MXT_MTR_MM3000 ) {
		fprintf(file, "  Motor type     = MM3000 motor.\n\n");
	} else if ( record->mx_type == MXT_MTR_MM4000 ) {
		fprintf(file, "  Motor type     = MM4000 motor.\n\n");
	} else if ( record->mx_type == MXT_MTR_ESP ) {
		fprintf(file, "  Motor type     = ESP motor.\n\n");
	} else {
		fprintf(file, "  Motor type     = Unknown (value = %ld)",
			record->mx_type);
	}

	fprintf(file, "  name           = %s\n", record->name);
	fprintf(file, "  port name      = %s\n", newport->record->name);
	fprintf(file, "  axis number    = %d\n", newport_motor->axis_number);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'", record->name);
	}
	
	if ( record->mx_type == MXT_MTR_MM3000 ) {
		fprintf(file, "  position       = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
		fprintf(file, "  scale          = %g %s per step.\n",
			motor->scale, motor->units);
		fprintf(file, "  offset         = %g %s.\n",
			motor->offset, motor->units);
	
		fprintf(file, "  backlash       = %g %s  (%ld steps)\n",
			motor->backlash_correction, motor->units,
			motor->raw_backlash_correction.stepper);
	
		fprintf(file, "  negative limit = %g %s  (%ld steps)\n",
			motor->negative_limit, motor->units,
			motor->raw_negative_limit.stepper);

		fprintf(file, "  positive limit = %g %s  (%ld steps)\n",
			motor->positive_limit, motor->units,
			motor->raw_positive_limit.stepper);

		move_deadband = motor->scale *
				(double) motor->raw_move_deadband.stepper;

		fprintf(file, "  move deadband  = %g %s  (%ld steps)\n\n",
			move_deadband, motor->units,
			motor->raw_move_deadband.stepper );

	} else {	/* ESP and MM4000 */
	
		fprintf(file, "  position       = %g %s  (%g)\n",
			motor->position, motor->units,
			motor->raw_position.analog );
		fprintf(file, "  scale          = %g %s per controller unit.\n",
			motor->scale, motor->units);
		fprintf(file, "  offset         = %g %s.\n",
			motor->offset, motor->units);
		fprintf(file, "  backlash       = %g %s  (%g)\n",
			motor->backlash_correction, motor->units,
			motor->raw_backlash_correction.analog );
		fprintf(file, "  negative limit = %g %s  (%g)\n",
			motor->negative_limit, motor->units,
			motor->raw_negative_limit.analog );
		fprintf(file, "  positive limit = %g %s  (%g)\n",
			motor->positive_limit, motor->units,
			motor->raw_positive_limit.analog );

		move_deadband = motor->scale * motor->raw_move_deadband.analog;

		fprintf(file, "  move deadband  = %g %s  (%g)\n\n",
			move_deadband, motor->units,
			motor->raw_move_deadband.analog );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_open( MX_RECORD *record )
{
	const char fname[] = "mxd_newport_open()";

	MX_MOTOR *motor;
	MX_NEWPORT_MOTOR *newport_motor;
	MX_NEWPORT *newport;
	char command[40];
	char response[40];
	int num_items;
	unsigned long limits_configuration;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the values of the speed, base speed, and acceleration
	 * from the controller.
	 */

	mx_status = mx_motor_get_speed( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_base_speed( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_raw_acceleration_parameters( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if hardware limits are active high or active low. */

	if ( record->mx_type != MXT_MTR_ESP ) {

		/* The MM3000 and MM4000 drivers do not actually
		 * use this parameter.
		 */

		newport_motor->hardware_limits_active_high = TRUE;

	} else {	/* Newport ESP */

		sprintf( command, "%dZH?", newport_motor->axis_number );

		
		mx_status = mxi_newport_command( newport, command,
				response, sizeof response, NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lxH", &limits_configuration );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Cannot parse hardware limit configuration value for motor '%s' from "
	"response = '%s'.", record->name, response );
		}

		MX_DEBUG( 2,("%s: limits_configuration = %#lx",
			fname, limits_configuration));

		if ( limits_configuration & 0x100000 ) {
			newport_motor->hardware_limits_active_high = TRUE;
		} else {
			newport_motor->hardware_limits_active_high = FALSE;
		}

		MX_DEBUG( 2,("%s: hardware_limits_active_high = %d",
		fname, newport_motor->hardware_limits_active_high ));
	}

	/* If this is an ESP series motor, send an MO command to turn on
	 * the motor.
	 */

	if ( record->mx_type == MXT_MTR_ESP ) {
		sprintf( command, "%dMO", newport_motor->axis_number );
		
		mx_status = mxi_newport_command( newport, command,
						NULL, 0, NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_newport_resynchronize()";

	MX_MOTOR *motor;
	MX_NEWPORT_MOTOR *newport_motor;
	MX_NEWPORT *newport;
	char command[40];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Forward the request on to the resynchronize function for
	 * the interface.
	 */

	mx_status = mxi_newport_resynchronize( newport_motor->newport_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this is an ESP series motor, send an MO command to turn on
	 * the motor.
	 */

	if ( record->mx_type == MXT_MTR_ESP ) {
		sprintf( command, "%dMO", newport_motor->axis_number );
		
		mx_status = mxi_newport_command( newport, command,
						NULL, 0, NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_newport_motor_is_busy( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_motor_is_busy()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	char command[20];
	char response[20];
	int length, num_items, done_flag;
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->record->mx_type == MXT_MTR_ESP ) {
		sprintf( command, "%dMD?", newport_motor->axis_number );
	} else {
		sprintf( command, "%dMS", newport_motor->axis_number );
	}

	mx_status = mxi_newport_command( newport, command,
				response, sizeof response, NEWPORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Zero length response to motor status command.");
	}

	if ( motor->record->mx_type == MXT_MTR_ESP ) {
		num_items = sscanf( response, "%d", &done_flag );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to parse motion done status for motor '%s'.  "
		"Response = '%s'.", motor->record->name, response );
		}
		if ( done_flag ) {
			motor->busy = FALSE;
		} else {
			motor->busy = TRUE;
		}
	} else {
		if ( (response[0] & 0x1) == 0 ) {
			motor->busy = FALSE;
		} else {
			motor->busy = TRUE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_move_absolute()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	double raw_position;
	long motor_steps;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->record->mx_type == MXT_MTR_MM3000 ) {

		motor_steps = motor->raw_destination.stepper;

		sprintf( command, "%dPA%ld",
				newport_motor->axis_number, motor_steps );

	} else {	/* ESP and MM4000 */

		raw_position = motor->raw_destination.analog;

		sprintf( command, "%dPA%g",
			newport_motor->axis_number, raw_position );
	}

	mx_status = mxi_newport_command( newport, command,
					NULL, 0, NEWPORT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_newport_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_get_position()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	double raw_position;
	long motor_steps;
	char command[20];
	char response[50];
	int num_tokens;
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%dTP", newport_motor->axis_number );

	mx_status = mxi_newport_command( newport, command,
				response, sizeof response, NEWPORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->record->mx_type == MXT_MTR_MM3000 ) {

		num_tokens = sscanf( response, "%ld", &motor_steps );

		if ( num_tokens != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No position value seen in response to 'TP' command.  "
			"Response seen = '%s'", response );
		}

		motor->raw_position.stepper = motor_steps;

	} else {	/* ESP and MM4000 */

		num_tokens = sscanf( response, "%lg", &raw_position );

		if ( num_tokens != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No position value seen in response to 'TP' command.  "
			"Response seen = '%s'", response );
		}

		motor->raw_position.analog = raw_position;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_set_position()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	char command[20];
	double position;
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	position = motor->raw_set_position.analog;

	if ( motor->record->mx_type == MXT_MTR_MM3000 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Newport MM3000s do not support setting the position value "
		"in any way." );
	}

	if ( motor->record->mx_type == MXT_MTR_MM4000 ) {

		if ( fabs(position) > MX_MOTOR_ANALOG_FUZZ ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Newport MM4000s do not support setting the current position "
		"to any value other than zero." );
		}

		sprintf( command, "%dZP", newport_motor->axis_number );

	} else {	/* ESP */

		sprintf( command, "%dDH%g",
				newport_motor->axis_number, position );
	}

	mx_status = mxi_newport_command( newport, command,
					NULL, 0, NEWPORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_soft_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_soft_abort()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%dST", newport_motor->axis_number );

	mx_status = mxi_newport_command( newport, command,
					NULL, 0, NEWPORT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_newport_immediate_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_immediate_abort()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->record->mx_type ) {
	case MXT_MTR_MM3000:
		sprintf( command, "%dAB", newport_motor->axis_number );
		break;
	case MXT_MTR_MM4000:
	case MXT_MTR_ESP:
		/* The following actually aborts all of the motors. */

		sprintf( command, "AB" );
		break;
	default:
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Motor '%s' is not a Newport MM3000, MM4000, or ESP controlled motor.  "
	"Actual motor type = %ld.", motor->record->name,
				motor->record->mx_type );
	}

	mx_status = mxi_newport_command( newport, command,
					NULL, 0, NEWPORT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_newport_positive_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_positive_limit_hit()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	char command[20];
	char response[20];
	int length;
#if 0
	int num_items;
	unsigned long limits_status, mask;
#endif
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->record->mx_type ) {
	case MXT_MTR_ESP:
		/* The following doesn't seem to work, but the ESP will
		 * report hitting a limit when TB is executed as part of
		 * mxi_newport_command(), so this isn't a big problem
		 * for the moment.
		 */
#if 0
		mx_status = mxi_newport_command( newport, "PH",
				response, sizeof response, NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lxH", &limits_status );

		MX_DEBUG(-2,("%s: limits_status = %#lx",
				fname, limits_status));

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
    "Cannot parse hardware limit status for motor '%s' from response = '%s'.",
				motor->record->name, response );
		}

		mask = ( 1 << (newport_motor->axis_number - 1) );

		MX_DEBUG(-2,("%s: mask = %#lx", fname, mask));
		MX_DEBUG(-2,("%s: hardware_limits_active_high = %d",
			fname, newport_motor->hardware_limits_active_high));

		if ( limits_status & mask ) {
			if ( newport_motor->hardware_limits_active_high ) {
				motor->positive_limit_hit = TRUE;
			} else {
				motor->positive_limit_hit = FALSE;
			}
		} else {
			if ( newport_motor->hardware_limits_active_high ) {
				motor->positive_limit_hit = FALSE;
			} else {
				motor->positive_limit_hit = TRUE;
			}
		}

		MX_DEBUG(-2,("%s: positive_limit_hit = %d",
			fname, motor->positive_limit_hit));

		MX_DEBUG(-2,("%s: overriding positive_limit_hit value.",fname));
#endif

		motor->positive_limit_hit = FALSE;
		break;

	case MXT_MTR_MM3000:
	case MXT_MTR_MM4000:
		sprintf( command, "%dMS", newport_motor->axis_number );

		mx_status = mxi_newport_command( newport, command,
				response, sizeof response, NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		length = strlen( response );

		if ( length <= 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Zero length response to motor status command.");
		}

		if ( (response[0] & 0x8) == 0 ) {
			motor->positive_limit_hit = FALSE;
		} else {
			motor->positive_limit_hit = TRUE;

			if ( motor->record->mx_type == MXT_MTR_MM4000 ) {
				/* Automatically turn the motors back on. */

				mx_status = mxi_newport_turn_motor_drivers_on(
						newport, NEWPORT_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		}
		break;

	default:
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Motor '%s' is not a Newport MM3000, MM4000, or ESP controlled motor.  "
	"Actual motor type = %ld.", motor->record->name,
				motor->record->mx_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_negative_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_negative_limit_hit()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	char command[20];
	char response[20];
	int length;
#if 0
	int num_items;
	unsigned long limits_status, mask;
#endif
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->record->mx_type ) {
	case MXT_MTR_ESP:
		/* The following doesn't seem to work, but the ESP will
		 * report hitting a limit when TB is executed as part of
		 * mxi_newport_command(), so this isn't a big problem
		 * for the moment.
		 */
#if 0
		mx_status = mxi_newport_command( newport, "PH",
				response, sizeof response, NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lxH", &limits_status );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
    "Cannot parse hardware limit status for motor '%s' from response = '%s'.",
				motor->record->name, response );
		}

		MX_DEBUG(-2,("%s: limits_status = %#lx",
				fname, limits_status));

		mask = ( 1 << (newport_motor->axis_number + 7) );

		MX_DEBUG(-2,("%s: mask = %#lx", fname, mask));
		MX_DEBUG(-2,("%s: hardware_limits_active_high = %d",
			fname, newport_motor->hardware_limits_active_high));

		if ( limits_status & mask ) {
			if ( newport_motor->hardware_limits_active_high ) {
				motor->negative_limit_hit = TRUE;
			} else {
				motor->negative_limit_hit = FALSE;
			}
		} else {
			if ( newport_motor->hardware_limits_active_high ) {
				motor->negative_limit_hit = FALSE;
			} else {
				motor->negative_limit_hit = TRUE;
			}
		}

		MX_DEBUG(-2,("%s: negative_limit_hit = %d",
			fname, motor->negative_limit_hit));

		MX_DEBUG(-2,("%s: overriding negative_limit_hit value.",fname));
#endif

		motor->negative_limit_hit = FALSE;
		break;

	case MXT_MTR_MM3000:
	case MXT_MTR_MM4000:
		sprintf( command, "%dMS", newport_motor->axis_number );

		mx_status = mxi_newport_command( newport, command,
				response, sizeof response, NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		length = strlen( response );

		if ( length <= 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Zero length response to motor status command.");
		}

		if ( (response[0] & 0x10) == 0 ) {
			motor->negative_limit_hit = FALSE;
		} else {
			motor->negative_limit_hit = TRUE;

			if ( motor->record->mx_type == MXT_MTR_MM4000 ) {
				/* Automatically turn the motors back on. */

				mx_status = mxi_newport_turn_motor_drivers_on(
						newport, NEWPORT_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		}
		break;

	default:
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Motor '%s' is not a Newport MM3000, MM4000, or ESP controlled motor.  "
	"Actual motor type = %ld.", motor->record->name,
				motor->record->mx_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_find_home_position()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%dOR", newport_motor->axis_number );

	mx_status = mxi_newport_command( newport, command,
					NULL, 0, NEWPORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_constant_velocity_move( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_constant_velocity_move()";

	MX_NEWPORT *newport;
	MX_NEWPORT_MOTOR *newport_motor;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( motor->record->mx_type ) {
	case MXT_MTR_MM3000:
	case MXT_MTR_ESP:
		if ( motor->constant_velocity_move >= 0 ) {
			sprintf( command, "%dMV+", newport_motor->axis_number );
		} else {
			sprintf( command, "%dMV-", newport_motor->axis_number );
		}

		mx_status = mxi_newport_command( newport, command,
						NULL, 0, NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXT_MTR_MM4000:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Constant velocity moves are not supported by the MM4000 or MM4005." );

	default:
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Motor '%s' is not a Newport MM3000, MM4000, or ESP controlled motor.  "
	"Actual motor type = %ld.", motor->record->name,
				motor->record->mx_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_get_parameter( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_get_parameter()";

	MX_NEWPORT_MOTOR *newport_motor;
	MX_NEWPORT *newport;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:

		if ( motor->record->mx_type == MXT_MTR_ESP ) {
			sprintf( command, "%dVA?", newport_motor->axis_number );
		} else {
			sprintf( command, "%dDV", newport_motor->axis_number );
		}

		mx_status = mxi_newport_command( newport, command,
				response, sizeof(response), NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( motor->record->mx_type == MXT_MTR_MM3000 )
		  && ( strchr( response, '-' ) != NULL ) )
		{
			/* MM3000 controlled stepper motor. */

			num_items = sscanf( response, "%*s-%lg",
							&(motor->raw_speed) );
		} else {
			/* All others. */

			num_items = sscanf( response, "%lg",
							&(motor->raw_speed) );
		}

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned speed for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}
		break;

	case MXLV_MTR_BASE_SPEED:

		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {

			sprintf( command, "%dDV", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport, command,
				response, sizeof(response), NEWPORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( strchr( response, '-' ) != NULL ) {

				/* MM3000 controlled stepper motor. */

				num_items = sscanf( response, "%lg-%*s",
						&(motor->raw_base_speed) );

				if ( num_items != 1 ) {
					return mx_error( MXE_DEVICE_IO_ERROR,
					fname, "Unable to parse returned base "
					"speed for motor '%s'.  "
					"Response = '%s'",
					motor->record->name, response );
				}
			} else {
				/* MM3000 controlled servo motor. */

				motor->raw_base_speed = 0.0;
			}
		} else if ( motor->record->mx_type == MXT_MTR_ESP ) {

			sprintf( command, "%dVB?", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport, command,
				response, sizeof(response), NEWPORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
						&(motor->raw_base_speed) );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned base speed for motor '%s'.  Response = '%s'",
					motor->record->name, response );
			}
		} else {				/* MM4000 */
			motor->raw_base_speed = 0.0;
		}
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		if ( motor->record->mx_type == MXT_MTR_ESP ) {
			sprintf( command, "%dAC?", newport_motor->axis_number );
		} else {
			sprintf( command, "%dDA", newport_motor->axis_number );
		}

		mx_status = mxi_newport_command( newport, command,
				response, sizeof(response), NEWPORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
				&(motor->raw_acceleration_parameters[0]) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse returned acceleration for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;

		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {
			motor->proportional_gain = 0.0;
		} else {
			if ( motor->record->mx_type == MXT_MTR_ESP ) {
				sprintf( command, "%dKP?",
					newport_motor->axis_number );
			} else {
				sprintf( command, "%dXP",
					newport_motor->axis_number );
			}

			mx_status = mxi_newport_command( newport, command,
					response, sizeof(response),
					NEWPORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
					&(motor->proportional_gain) );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned gain for motor '%s'.  Response = '%s'",
					motor->record->name, response );
			}
		}
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {
			motor->integral_gain = 0.0;
		} else {
			if ( motor->record->mx_type == MXT_MTR_ESP ) {
				sprintf( command, "%dKI?",
					newport_motor->axis_number );
			} else {
				sprintf( command, "%dXI",
					newport_motor->axis_number );
			}

			mx_status = mxi_newport_command( newport, command,
					response, sizeof(response),
					NEWPORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
					&(motor->integral_gain) );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned gain for motor '%s'.  Response = '%s'",
					motor->record->name, response );
			}
		}
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {
			motor->derivative_gain = 0.0;
		} else {
			if ( motor->record->mx_type == MXT_MTR_ESP ) {
				sprintf( command, "%dKP?",
					newport_motor->axis_number );
			} else {
				sprintf( command, "%dXP",
					newport_motor->axis_number );
			}

			mx_status = mxi_newport_command( newport, command,
					response, sizeof(response),
					NEWPORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
					&(motor->derivative_gain) );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned gain for motor '%s'.  Response = '%s'",
					motor->record->name, response );
			}
		}
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		if ( motor->record->mx_type != MXT_MTR_ESP ) {
			motor->velocity_feedforward_gain = 0.0;

		} else {
			sprintf( command, "%dVF?", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport, command,
					response, sizeof(response),
					NEWPORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
					&(motor->velocity_feedforward_gain) );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned gain for motor '%s'.  Response = '%s'",
					motor->record->name, response );
			}
		}
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		if ( motor->record->mx_type != MXT_MTR_ESP ) {
			motor->acceleration_feedforward_gain = 0.0;

		} else {
			sprintf( command, "%dAF?", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport, command,
					response, sizeof(response),
					NEWPORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
				&(motor->acceleration_feedforward_gain) );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned gain for motor '%s'.  Response = '%s'",
					motor->record->name, response );
			}
		}
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {
			motor->integral_limit = 0.0;
		} else {
			sprintf( command, "%dKS?", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport, command,
					response, sizeof(response),
					NEWPORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg",
				&(motor->integral_limit) );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to parse returned integral limit for motor '%s'.  "
		"Response = '%s'", motor->record->name, response );
			}
		}
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_newport_set_parameter( MX_MOTOR *motor )
{
	const char fname[] = "mxd_newport_set_parameter()";

	MX_NEWPORT_MOTOR *newport_motor;
	MX_NEWPORT *newport;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_newport_get_pointers( motor, &newport_motor,
						&newport, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		sprintf( command, "%dVA%g", newport_motor->axis_number,
						motor->raw_speed );

		mx_status = mxi_newport_command( newport,
				command, NULL, 0, NEWPORT_DEBUG );
		break;

	case MXLV_MTR_BASE_SPEED:
		if ( motor->record->mx_type == MXT_MTR_MM4000 ) {
			motor->raw_base_speed = 0.0;

		} else {		/* ESP and MM3000 */

			sprintf( command, "%dVB%g", newport_motor->axis_number,
						motor->raw_base_speed );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );
		}
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		sprintf( command, "%dAC%g", newport_motor->axis_number,
					motor->raw_acceleration_parameters[0] );

		mx_status = mxi_newport_command( newport,
				command, NULL, 0, NEWPORT_DEBUG );
		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			strcpy( command, "MO" );
		} else {
			strcpy( command, "MF" );
		}

		mx_status = mxi_newport_command( newport, command,
						NULL, 0, NEWPORT_DEBUG );
		break;

	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			strcpy( command, "MO" );
		} else {
			strcpy( command, "MF" );
		}

		mx_status = mxi_newport_command( newport, command,
						NULL, 0, NEWPORT_DEBUG );
		break;

	case MXLV_MTR_FAULT_RESET:
		mx_status = mxi_newport_command( newport, "MO",
						NULL, 0, NEWPORT_DEBUG );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {

			motor->proportional_gain = 0.0;

		} else {		/* ESP and MM4000 */

			sprintf( command, "%dKP%f", newport_motor->axis_number,
					motor->proportional_gain );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );

			sprintf( command, "%dUF", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );
		}
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {

			motor->integral_gain = 0.0;

		} else {		/* ESP and MM4000 */

			sprintf( command, "%dKI%f", newport_motor->axis_number,
					motor->integral_gain );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );

			sprintf( command, "%dUF", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );
		}
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {

			motor->derivative_gain = 0.0;

		} else {		/* ESP and MM4000 */

			sprintf( command, "%dKD%f", newport_motor->axis_number,
					motor->derivative_gain );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );

			sprintf( command, "%dUF", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );
		}
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		if ( motor->record->mx_type != MXT_MTR_ESP ) {

			motor->velocity_feedforward_gain = 0.0;

		} else {		/* ESP */

			sprintf( command, "%dVF%f", newport_motor->axis_number,
					motor->velocity_feedforward_gain );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );

			sprintf( command, "%dUF", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );
		}
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		if ( motor->record->mx_type != MXT_MTR_ESP ) {

			motor->acceleration_feedforward_gain = 0.0;

		} else {		/* ESP */

			sprintf( command, "%dAF%f", newport_motor->axis_number,
					motor->acceleration_feedforward_gain );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );

			sprintf( command, "%dUF", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );
		}
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		if ( motor->record->mx_type == MXT_MTR_MM3000 ) {

			motor->integral_limit = 0.0;

		} else {
			sprintf( command, "%dKS%f", newport_motor->axis_number,
					motor->integral_limit );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );

			sprintf( command, "%dUF", newport_motor->axis_number );

			mx_status = mxi_newport_command( newport,
					command, NULL, 0, NEWPORT_DEBUG );
		}
		break;

	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

