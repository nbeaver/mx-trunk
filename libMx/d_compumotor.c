/*
 * Name:    d_compumotor.c
 *
 * Purpose: MX driver for the Compumotor 6000 and 6K series of motor
 *          controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define COMPUMOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "d_compumotor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_compumotor_record_function_list = {
	NULL,
	mxd_compumotor_create_record_structures,
	mxd_compumotor_finish_record_initialization,
	NULL,
	mxd_compumotor_print_structure,
	NULL,
	NULL,
	mxd_compumotor_open,
	mxd_compumotor_close,
	NULL,
	mxd_compumotor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_compumotor_motor_function_list = {
	mxd_compumotor_motor_is_busy,
	mxd_compumotor_move_absolute,
	mxd_compumotor_get_position,
	mxd_compumotor_set_position,
	mxd_compumotor_soft_abort,
	mxd_compumotor_immediate_abort,
	mxd_compumotor_positive_limit_hit,
	mxd_compumotor_negative_limit_hit,
	mxd_compumotor_find_home_position,
	mxd_compumotor_constant_velocity_move,
	mxd_compumotor_get_parameter,
	mxd_compumotor_set_parameter,
	mxd_compumotor_simultaneous_start,
	mxd_compumotor_get_status
};

/* Compumotor motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_compumotor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_COMPUMOTOR_STANDARD_FIELDS
};

long mxd_compumotor_num_record_fields
		= sizeof( mxd_compumotor_record_field_defaults )
			/ sizeof( mxd_compumotor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_compumotor_rfield_def_ptr
			= &mxd_compumotor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_compumotor_get_pointers( MX_MOTOR *motor,
			MX_COMPUMOTOR **compumotor,
			MX_COMPUMOTOR_INTERFACE **compumotor_interface,
			const char *calling_fname )
{
	static const char fname[] = "mxd_compumotor_get_pointers()";

	MX_RECORD *compumotor_interface_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( compumotor == (MX_COMPUMOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_COMPUMOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_COMPUMOTOR_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*compumotor = (MX_COMPUMOTOR *) (motor->record->record_type_struct);

	if ( *compumotor == (MX_COMPUMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_COMPUMOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	compumotor_interface_record =
			(*compumotor)->compumotor_interface_record;

	if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The compumotor_interface_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			compumotor_interface_record->record_type_struct;

	if ( *compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_COMPUMOTOR_INTERFACE pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_compumotor_check_error_response( MX_MOTOR *motor,
					char *response,
					const char *calling_fname )
{
	char *name;
	int error_seen;

	name = motor->record->name;
	error_seen = FALSE;

	if ( response[13] == '1' ) {
		mx_warning( "Motor '%s' stall detected.", name );
		error_seen = TRUE;
	}
	if ( response[15] == '1' ) {
		mx_warning( "Motor '%s' drive shut down.", name );
		error_seen = TRUE;
	}
	if ( response[16] == '1' ) {
		mx_warning( "Motor '%s' drive fault occurred.", name );
		error_seen = TRUE;
	}
	if ( response[27] == '1' ) {
		mx_warning( "Motor '%s' position error exceeded.", name );
		error_seen = TRUE;
	}
	if ( response[30] == '1' ) {
		mx_warning( "Motor '%s' target zone timeout occurred.", name );
		error_seen = TRUE;
	}

	if ( error_seen ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, calling_fname,
"Motor '%s' axis status command 'TAS' reported one or more errors.", name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxd_compumotor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_compumotor_create_record_structures()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR *compumotor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	compumotor = (MX_COMPUMOTOR *) malloc( sizeof(MX_COMPUMOTOR) );

	if ( compumotor == (MX_COMPUMOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_COMPUMOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = compumotor;
	record->class_specific_function_list
				= &mxd_compumotor_motor_function_list;

	motor->record = record;

	/* A Compumotor motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* The Compumotor reports accelerations in units/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	compumotor->controller_index = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_compumotor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	MX_COMPUMOTOR *compumotor;
	MX_RECORD **motor_array;
	int i, j, num_axes, controller_index;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find the array index for this controller number. */

	mx_status = mxi_compumotor_get_controller_index( compumotor_interface,
					compumotor->controller_number,
					&controller_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	compumotor->controller_index = controller_index;

	/* Add the motor record to the interface record's motor_array. */

	if ( compumotor_interface->motor_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The motor_array pointer for the Compumotor interface '%s' is NULL.",
			compumotor_interface->record->name );
	}

	i = compumotor->controller_index;
	j = compumotor->axis_number - 1;

	num_axes = compumotor_interface->num_axes[i];

	motor_array = compumotor_interface->motor_array[i];

	if ( motor_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The motor_array pointer for index %d on the "
			"Compumotor interface '%s' is NULL.",
			i, compumotor_interface->record->name );
	}

	motor_array[j] = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_compumotor_print_structure()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = COMPUMOTOR motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  port name         = %s\n",
					compumotor_interface->record->name);
	fprintf(file, "  controller number = %d\n",
					compumotor->controller_number);
	fprintf(file, "  axis number       = %d\n", compumotor->axis_number);
	fprintf(file, "  flags             = %#lx\n", compumotor->flags);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%g)\n",
			motor->position, motor->units,
			motor->raw_position.analog );
	fprintf(file, "  scale             = %g %s per raw unit.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	
	fprintf(file, "  negative limit    = %g %s  (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit    = %g %s  (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband     = %g %s  (%g)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed             = %g %s/s  (%g raw units/s)\n",
		speed, motor->units,
		motor->raw_speed );

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_compumotor_check_for_servo( MX_COMPUMOTOR_INTERFACE *compumotor_interface,
				MX_COMPUMOTOR *compumotor )
{
	static const char fname[] = "mxd_compumotor_check_for_servo()";

	int i, j, k, l, num_axes, skipped;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	i = compumotor->controller_index;

	switch( (compumotor_interface->controller_type)[i] ) {
	case MXT_COMPUMOTOR_ZETA_6000:
	case MXT_COMPUMOTOR_6000_SERIES:
		compumotor->is_servo = FALSE;

		compumotor->flags
		    |= MXF_COMPUMOTOR_ROUND_POSITION_OUTPUT_TO_NEAREST_INTEGER;

		break;

	case MXT_COMPUMOTOR_6K:
		j = compumotor->axis_number - 1;

		num_axes = (int) (compumotor_interface->num_axes)[i];

		sprintf( command, "%d_!AXSDEF", compumotor->controller_number );

		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof response, COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Find the character in the response corresponding to
		 * this axis.  There may be an underscore '_' character
		 * in the response, so we must skip over that.
		 */

		l = 0; skipped = 0;

		for ( k = 0; k <= j; k++ ) {

			l = k + skipped;

			if ( response[l] == '_' ) {
				skipped++;
			}

			l = k + skipped;
		}

#if COMPUMOTOR_DEBUG
		MX_DEBUG(-2,("%s: response[%d] = '%c'",
				fname, l, response[l]));
#endif

		if ( response[l] == '1' ) {
			compumotor->is_servo = TRUE;
		} else
		if ( response[l] == '0' ) {
			compumotor->is_servo = FALSE;
		} else {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Bad character '%c' found in response '%s' from Compumotor controller '%s'.",
				response[l], response,
				compumotor_interface->record->name );
		}
		break;
	default:
		compumotor->is_servo = FALSE;
		break;
	}

#if COMPUMOTOR_DEBUG
	MX_DEBUG(-2,("%s: compumotor->is_servo = %d",
		fname, compumotor->is_servo));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_compumotor_servo_initialization(
			MX_COMPUMOTOR_INTERFACE *compumotor_interface,
			MX_COMPUMOTOR *compumotor )
{
	static const char fname[] = "mxd_compumotor_servo_initialization()";

	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	sprintf( command, "%d_!%dERES", compumotor->controller_number,
					compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof response, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &(compumotor->axis_resolution) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to parse response '%s' to Compumotor command '%s'.",
			response, command );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_compumotor_stepper_initialization(
			MX_COMPUMOTOR_INTERFACE *compumotor_interface,
			MX_COMPUMOTOR *compumotor )
{
	static const char fname[] = "mxd_compumotor_stepper_initialization()";

	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	sprintf( command, "%d_!%dDRES", compumotor->controller_number,
					compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof response, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &(compumotor->axis_resolution) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to parse response '%s' to Compumotor command '%s'.",
			response, command );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_compumotor_open()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the controller to use absolute positioning mode for
	 * this motor.
	 */

	sprintf( command, "%d_!%dMA1", compumotor->controller_number,
					compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			NULL, 0, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, disable the hardware limit switches.  Otherwise,
	 * enable them.
	 */

	if ( compumotor->flags & MXF_COMPUMOTOR_DISABLE_HARDWARE_LIMITS ) {

		sprintf( command, "%d_!%dLH0", compumotor->controller_number,
						compumotor->axis_number );
	} else {
		sprintf( command, "%d_!%dLH3", compumotor->controller_number,
						compumotor->axis_number );
	}

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			NULL, 0, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Attempt to determine whether this motor is a stepper motor
	 * or a servo motor.
	 */

	mx_status = mxd_compumotor_check_for_servo( compumotor_interface,
							compumotor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stepper and servo motors have different initialization steps. */

	if ( compumotor->is_servo ) {
		mx_status = mxd_compumotor_servo_initialization(
					compumotor_interface, compumotor );
	} else {
		mx_status = mxd_compumotor_stepper_initialization(
					compumotor_interface, compumotor );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the internal values of the speed, base speed,
	 * and acceleration.
	 */

	mx_status = mx_motor_get_speed( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_base_speed( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_raw_acceleration_parameters( record, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_compumotor_close()";

	MX_MOTOR *motor;
	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	double position;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current position.  This has the side effect of
	 * updating the position value in the MX_MOTOR structure.
	 */

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* At present, the position is the only parameter we read out.
	 * We rely on the non-volatile memory of the controller
	 * for the rest of the controller parameters.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_compumotor_resynchronize()";

	MX_COMPUMOTOR *compumotor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	compumotor = (MX_COMPUMOTOR *) record->record_type_struct;

	if ( compumotor == (MX_COMPUMOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_COMPUMOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxi_compumotor_resynchronize(
			compumotor->compumotor_interface_record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_compumotor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_motor_is_busy()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char response[80];
	int length;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%d_!%dTAS", compumotor->controller_number,
					compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			response, sizeof response, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Zero length response to motor status command.");
	}

	if ( response[0] == '1' ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	mx_status = mxd_compumotor_check_error_response( motor, response, fname );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_move_absolute()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	double destination;
	long flags;
	int i, length, command_buffer_left;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Change the Compumotor axis to preset mode. */

	mx_status = mxd_compumotor_enable_continuous_mode( compumotor,
						compumotor_interface, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the position to move to. */

	flags = compumotor->flags;

	if (flags & MXF_COMPUMOTOR_ROUND_POSITION_OUTPUT_TO_NEAREST_INTEGER) {

		destination = (double) mx_round(motor->raw_destination.analog);
	} else {
		destination = motor->raw_destination.analog;
	}
	sprintf( command, "%d_!%dD%g", compumotor->controller_number,
			compumotor->axis_number, destination );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the move command. */

	sprintf( command, "%d_!GO", compumotor->controller_number );

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {

		length = strlen( command );
		command_buffer_left = sizeof( command ) - length - 1;

		if ( (i+1) == compumotor->axis_number ) {
			strncat( command, "1", command_buffer_left );
		} else {
			strncat( command, "X", command_buffer_left );
		}
	}

	command[ length + MX_MAX_COMPUMOTOR_AXES ] = '\0';

	/* Command the move to start. */

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, COMPUMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_get_position()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char response[80];
	int num_tokens;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( compumotor->flags & MXF_COMPUMOTOR_USE_ENCODER_POSITION ) {

		sprintf( command, "%d_!%dTPE",
			compumotor->controller_number,
			compumotor->axis_number );
	} else {
		sprintf( command, "%d_!%dTPM",
			compumotor->controller_number,
			compumotor->axis_number );
	}

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			response, sizeof response, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_tokens = sscanf( response, "%lg", &(motor->raw_position.analog) );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to '%s' command.  "
		"Response seen = '%s'", command, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_set_position()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	MX_RECORD **motor_array;
	MX_MOTOR *other_motor;
	double new_set_position;
	double other_motor_position;
	char command[100];
	char buffer[80];
	long flags;
	int i, j, num_axes, length, command_buffer_left;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( compumotor_interface->motor_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The motor_array pointer for the Compumotor interface '%s' is NULL.",
			compumotor_interface->record->name );
	}

	i = compumotor->controller_index;

	num_axes = compumotor_interface->num_axes[i];

	motor_array = compumotor_interface->motor_array[i];

	flags = compumotor->flags;

	if (flags & MXF_COMPUMOTOR_ROUND_POSITION_OUTPUT_TO_NEAREST_INTEGER) {

		new_set_position = (double)
				mx_round( motor->raw_set_position.analog );
	} else {
		new_set_position = motor->raw_set_position.analog;
	}

	/* Construct a PSET command. */

	sprintf( command, "%d_!PSET", compumotor->controller_number );

	for ( j = 0; j < num_axes; j++ ) {

		if ( j != 0 ) {
			length = strlen( command );
			command_buffer_left = sizeof(buffer) - length - 1;
			strncat( command, ",", command_buffer_left );
		}
		if ( j+1 == compumotor->axis_number ) {
			sprintf( buffer, "%g", new_set_position );
		} else {
			if ( motor_array[j] == (MX_RECORD *) NULL ) {
				strcpy( buffer, "0" );
			} else {
				mx_status = mx_motor_get_position(
							motor_array[j], NULL );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				other_motor = (MX_MOTOR *)
					motor_array[j]->record_class_struct;

				other_motor_position =
					other_motor->raw_position.analog;

				sprintf( buffer, "%g", other_motor_position );
			}
		}
		length = strlen( command );
		command_buffer_left = sizeof( command ) - length - 1;

		strncat( command, buffer, command_buffer_left );
	}

	/* Send the PSET command. */

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			NULL, 0, COMPUMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_soft_abort()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	int i, length;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct a stop command that only stops this axis. */

	sprintf( command, "%d_!S", compumotor->controller_number );

	length = strlen( command );

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {

		if ( (i+1) == compumotor->axis_number ) {
			command[ length + i ] = '1';
		} else {
			command[ length + i ] = '0';
		}
	}

	command[ length + MX_MAX_COMPUMOTOR_AXES ] = '\0';

	/* Send the stop command. */

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, COMPUMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_immediate_abort()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a kill motion command that halts all axes attached to
	 * this controller.
	 */

	sprintf( command, "%d_!K", compumotor->controller_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, COMPUMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_positive_limit_hit()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char response[80];
	int length;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%d_!%dTAS", compumotor->controller_number,
					compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			response, sizeof response, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Zero length response to motor status command.");
	}

	/* When constructing bit indices for the output of the TAS
	 * command, remember that the output of TAS also has
	 * embedded underscores in it that you must skip over too.
	 */

	if ( ( response[17] == '1' ) || ( response[20] == '1' ) ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	mx_status = mxd_compumotor_check_error_response( motor, response, fname );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_negative_limit_hit()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char response[80];
	int length;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%d_!%dTAS", compumotor->controller_number,
					compumotor->axis_number );


	mx_status = mxi_compumotor_command( compumotor_interface, command,
			response, sizeof response, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Zero length response to motor status command.");
	}

	/* When constructing bit indices for the output of the TAS
	 * command, remember that the output of TAS also has
	 * embedded underscores in it that you must skip over too.
	 */

	if ( ( response[18] == '1' ) || ( response[21] == '1' ) ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	mx_status = mxd_compumotor_check_error_response( motor, response, fname );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_find_home_position()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	int i, length;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the home search command. */

	sprintf( command, "%d_!HOM", compumotor->controller_number );

	length = strlen( command );

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {

		if ( (i+1) == compumotor->axis_number ) {
			switch( motor->home_search ) {
			case 1:
				command[ length + i ] = '0';
				break;
			case (-1):
				command[ length + i ] = '1';
				break;
			default:
				command[ length + i ] = 'X';
			}
		} else {
			command[ length + i ] = 'X';
		}
	}

	command[ length + MX_MAX_COMPUMOTOR_AXES ] = '\0';

	/* Command the home search to start. */

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, COMPUMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_constant_velocity_move()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	int i, length, command_buffer_left;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Change the Compumotor axis to continuous mode. */

	mx_status = mxd_compumotor_enable_continuous_mode( compumotor,
						compumotor_interface, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the direction of the move. */

	if ( motor->constant_velocity_move >= 0 ) {
		sprintf( command, "%d_!%dD+", compumotor->controller_number,
						compumotor->axis_number );
	} else {
		sprintf( command, "%d_!%dD-", compumotor->controller_number,
						compumotor->axis_number );
	}
		
	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the move command. */

	sprintf( command, "%d_!GO", compumotor->controller_number );

	for ( i = 0; i < MX_MAX_COMPUMOTOR_AXES; i++ ) {

		length = strlen( command );
		command_buffer_left = sizeof( command ) - length - 1;

		if ( (i+1) == compumotor->axis_number ) {
			strncat( command, "1", command_buffer_left );
		} else {
			strncat( command, "X", command_buffer_left );
		}
	}

	command[ length + MX_MAX_COMPUMOTOR_AXES ] = '\0';

	/* Command the move to start. */

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					NULL, 0, COMPUMOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_get_parameter()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char response[80];
	int i, num_items;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		sprintf( command, "%d_!%dV", compumotor->controller_number,
						compumotor->axis_number );

		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof(response), COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned speed for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->raw_speed = double_value * compumotor->axis_resolution;
		break;

	case MXLV_MTR_BASE_SPEED:
		i = compumotor->controller_index;

		switch( (compumotor_interface->controller_type)[i] ) {
		case MXT_COMPUMOTOR_ZETA_6000:
			sprintf( command, "%d_!%dSSV",
						compumotor->controller_number,
						compumotor->axis_number );

			mx_status = mxi_compumotor_command( compumotor_interface,
				command, response, sizeof(response),
				COMPUMOTOR_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lg", &double_value );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned base speed for motor '%s'.  Response = '%s'",
					motor->record->name, response );
			}

			motor->raw_base_speed = double_value
						* compumotor->axis_resolution;
			break;
		default:
			/* All others, including the 6K series, return zero. */

			motor->raw_base_speed = 0.0;
			break;
		}
		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		/* As documented in the manual. */

		motor->raw_maximum_speed = 1600000;  /* steps per second */
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		sprintf( command, "%d_!%dA", compumotor->controller_number,
						compumotor->axis_number );

		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof(response), COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse returned acceleration for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->raw_acceleration_parameters[0]
				= double_value * compumotor->axis_resolution;

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		sprintf( command, "%d_!%dSGP", compumotor->controller_number,
						compumotor->axis_number );
		
		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof(response), COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse returned proportional gain for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->proportional_gain = double_value;
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		sprintf( command, "%d_!%dSGI", compumotor->controller_number,
						compumotor->axis_number );
		
		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof(response), COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse returned integral gain for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->integral_gain = double_value;
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		sprintf( command, "%d_!%dSGV", compumotor->controller_number,
						compumotor->axis_number );
		
		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof(response), COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse returned differential gain for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->derivative_gain = double_value;
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		sprintf( command, "%d_!%dSGVF", compumotor->controller_number,
						compumotor->axis_number );
		
		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof(response), COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse returned velocity_feedforward gain for motor '%s'.  "
"Response = '%s'", motor->record->name, response );
		}

		motor->velocity_feedforward_gain = double_value;
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		sprintf( command, "%d_!%dSGAF", compumotor->controller_number,
						compumotor->axis_number );
		
		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof(response), COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse returned acceleration_feedforward gain for motor '%s'.  "
"Response = '%s'", motor->record->name, response );
		}

		motor->acceleration_feedforward_gain = double_value;
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		sprintf( command, "%d_!%dSGILIM", compumotor->controller_number,
						compumotor->axis_number );
		
		mx_status = mxi_compumotor_command( compumotor_interface, command,
				response, sizeof(response), COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Unable to parse returned integral limit for motor '%s'.  "
"Response = '%s'", motor->record->name, response );
		}

		motor->integral_limit = double_value;
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_compumotor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_set_parameter()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	int i;
	double double_value, new_raw_value;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		new_raw_value = motor->raw_speed;

		double_value = mx_divide_safely( motor->raw_speed,
						compumotor->axis_resolution );

		sprintf( command, "%d_!%dV%g", compumotor->controller_number,
						compumotor->axis_number,
						double_value );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Verify that the speed was actually changed by reading
		 * back the current speed.
		 */

		mx_status = mxd_compumotor_get_parameter( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->speed = fabs(motor->scale) * motor->raw_speed;

		if ( mx_difference( motor->raw_speed,
				new_raw_value ) > 0.001 )
		{
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"Attempt to set the speed of motor '%s' to %g %s/s (%g steps/s) failed.  "
"Current speed = %g %s/s (%g steps/s).",
				motor->record->name,
				new_raw_value * fabs(motor->scale),
				motor->units,
				new_raw_value,
				motor->speed,
				motor->units,
				motor->raw_speed );
		}
		break;

	case MXLV_MTR_BASE_SPEED:
		i = compumotor->controller_index;

		switch( (compumotor_interface->controller_type)[i] ) {
		case MXT_COMPUMOTOR_ZETA_6000:
			new_raw_value = motor->raw_base_speed;

			double_value = mx_divide_safely( motor->raw_base_speed,
						compumotor->axis_resolution );

			sprintf( command, "%d_!%dSSV%g",
						compumotor->controller_number,
						compumotor->axis_number,
						double_value );

			mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Verify that the base speed was actually changed
			 * by reading back the current base speed.
			 */
	
			mx_status = mxd_compumotor_get_parameter( motor );
	
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
	
			motor->base_speed = fabs(motor->scale)
						* motor->raw_base_speed;
	
			if ( mx_difference( motor->raw_base_speed,
						new_raw_value ) > 0.001 )
			{
				return mx_error(MXE_DEVICE_ACTION_FAILED,fname,
"Attempt to set the base speed of motor '%s' to %g %s/s (%g steps/s) failed.  "
"Current base speed = %g %s/s (%g steps/s).",
					motor->record->name,
					new_raw_value * fabs(motor->scale),
					motor->units,
					new_raw_value,
					motor->base_speed,
					motor->units,
					motor->raw_base_speed );
			}
			break;
		default:
			/* All other types, including the 6K controller,
			 * ignore the base speed change request.
			 */

			return MX_SUCCESSFUL_RESULT;
			break;
		}
		break;

	case MXLV_MTR_MAXIMUM_SPEED:

		motor->raw_maximum_speed = 1600000;  /* steps per second */

		return MX_SUCCESSFUL_RESULT;
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		/* First make sure that deceleration equals acceleration. */

		sprintf( command, "%d_!%dAD0", compumotor->controller_number,
					compumotor->axis_number );
		
		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Then, format the set acceleration command. */

		new_raw_value = motor->raw_acceleration_parameters[0];

		double_value = mx_divide_safely(
					motor->raw_acceleration_parameters[0],
					compumotor->axis_resolution );

		sprintf( command, "%d_!%dA%g", compumotor->controller_number,
					compumotor->axis_number,
					double_value );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Verify that the acceleration was actually changed by reading
		 * back the current acceleration.
		 */

		mx_status = mxd_compumotor_get_parameter( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mx_difference( motor->raw_acceleration_parameters[0],
				new_raw_value ) > 0.001 )
		{
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"Attempt to set the acceleration of motor '%s' to %g steps/s^2 failed.  "
"Current acceleration = %g steps/s^2.",
				motor->record->name,
				new_raw_value,
				motor->raw_acceleration_parameters[0] );
		}
		break;

	/* All of the axis faults and enables are reset by the DRIVE command. */

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = 1;
		}
		sprintf( command, "%d_!%dDRIVE%d",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->axis_enable );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	case MXLV_MTR_CLOSED_LOOP:
		if ( motor->closed_loop ) {
			motor->closed_loop = 1;
		}
		sprintf( command, "%d_!%dDRIVE%d",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->closed_loop );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	case MXLV_MTR_FAULT_RESET:
		if ( motor->fault_reset ) {
			motor->fault_reset = 1;
		}
		sprintf( command, "%d_!%dDRIVE%d",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->fault_reset );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	/* Servo loop gains. */

	case MXLV_MTR_PROPORTIONAL_GAIN:
		sprintf( command, "%d_!%dSGP%f",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->proportional_gain );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		sprintf( command, "%d_!%dSGI%f",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->integral_gain );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		sprintf( command, "%d_!%dSGV%f",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->derivative_gain );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		sprintf( command, "%d_!%dSGVF%f",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->velocity_feedforward_gain );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		sprintf( command, "%d_!%dSGAF%f",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->acceleration_feedforward_gain );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		sprintf( command, "%d_!%dSGILIM%f",
				compumotor->controller_number,
				compumotor->axis_number,
				motor->integral_limit );

		mx_status = mxi_compumotor_command( compumotor_interface,
				command, NULL, 0, COMPUMOTOR_DEBUG );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_simultaneous_start( int num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				int flags )
{
	static const char fname[] = "mxd_compumotor_simultaneous_start()";

	MX_RECORD *motor_record, *current_interface_record;
	MX_MOTOR *motor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	MX_COMPUMOTOR_INTERFACE *current_compumotor_interface;
	MX_COMPUMOTOR *current_compumotor;
	int i, controller_index, controller_number, current_controller_number;

	mx_status_type mx_status;

	compumotor_interface = NULL;
	controller_number = -1;

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor_record->mx_type != MXT_MTR_COMPUMOTOR ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				motor_record->name );
		}

		current_compumotor = (MX_COMPUMOTOR *)
					motor_record->record_type_struct;

		current_interface_record
			= current_compumotor->compumotor_interface_record;

		current_compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			current_interface_record->record_type_struct;

		current_controller_number
			= current_compumotor->controller_number;

		if ( compumotor_interface == (MX_COMPUMOTOR_INTERFACE *) NULL )
		{
			compumotor_interface = current_compumotor_interface;
			controller_number = current_controller_number;
		}

		if ( compumotor_interface != current_compumotor_interface ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different Compumotor interfaces, "
		"namely '%s' and '%s'.",
				motor_record_array[0]->name,
				motor_record->name,
				compumotor_interface->record->name,
				current_interface_record->name );
		}
		if ( controller_number != current_controller_number ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different controller numbers, "
		"namely %d and %d, on Compumotor interface '%s'.",
				motor_record_array[0]->name,
				motor_record->name,
				controller_number,
				current_controller_number,
				compumotor_interface->record->name );
		}
	}

	/* Find the specified controller number in the array of controllers. */

	mx_status = mxi_compumotor_get_controller_index( compumotor_interface,
					controller_number,
					&controller_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the move. */

	mx_status = mxi_compumotor_multiaxis_move( compumotor_interface,
					controller_index,
					num_motor_records,
					motor_record_array,
					position_array,
					TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_compumotor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_compumotor_get_status()";

	MX_COMPUMOTOR *compumotor;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface;
	char command[80];
	char response[80];
	int length;
	mx_status_type mx_status;

	mx_status = mxd_compumotor_get_pointers( motor, &compumotor,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%d_!%dTAS", compumotor->controller_number,
					compumotor->axis_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			response, sizeof response, COMPUMOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Zero length response to motor status command.");
	}

	if ( response[0] == '1' ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	/* Check all of the status bits that we are interested in. */

	motor->status = 0;
	 
	/* Remember when computing bit offsets that the ASCII versions of
	 * the bits are separated into groups of four by '_' characters.
	 */

	/* Bit 1: Moving/Not Moving */

	if ( response[0] == '1' ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* Bits 2 to 4: (ignored) */

	/* Bit 5: Home Successful */

	if ( response[5] == '1' ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	/* Bits 6 to 11: (ignored) */

	/* Bit 12: Stall Detected */

	if ( response[13] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 13: Drive Shut Down */

	if ( response[15] == '1' ) {
		motor->status |= (MXSF_MTR_AXIS_DISABLED | MXSF_MTR_OPEN_LOOP);
	}

	/* Bit 14: Drive Fault Occurred */

	if ( response[16] == '1' ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* Bit 15: Positive-direction Hardware Limit Hit */

	if ( response[17] == '1' ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* Bit 16: Negative-direction Hardware Limit Hit */

	if ( response[18] == '1' ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bit 17: Positive-direction Software Limit Hit */

	if ( response[20] == '1' ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

	/* Bit 18: Negative-direction Software Limit Hit */

	if ( response[21] == '1' ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	/* Bits 19 to 22: (ignored) */

	/* Bit 23: Position Error Exceeded */

	if ( response[27] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 24: (ignored) */

	/* Bit 25: Target Zone Timeout Occurred */

	if ( response[30] == '1' ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bits 26 to 32: (ignored) */

	/* Print out a verbose message to the warning log. */

	mx_status = mxd_compumotor_check_error_response( motor,
							response, fname );

	return mx_status;
}

/*======================================================================*/

MX_EXPORT mx_status_type
mxd_compumotor_enable_continuous_mode( MX_COMPUMOTOR *compumotor,
				MX_COMPUMOTOR_INTERFACE *compumotor_interface,
				int enable_flag )
{
	static const char fname[] = "mxd_compumotor_enable_continuous_mode()";

	MX_RECORD **motor_array;
	char command[80];
	int i, num_axes;
	mx_status_type mx_status;

	/* Are we being commanded to change to the positioner mode
	 * that we are already in?  If so, return without sending any
	 * commands out.
	 */

	if ( compumotor->continuous_mode_enabled == enable_flag )
		return MX_SUCCESSFUL_RESULT;

	if ( (enable_flag != FALSE) && (enable_flag != TRUE) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal value %d for enable_flag argument.",
			enable_flag );
	}

	/* Construct the MC command that we will use to change the mode. */

	if ( compumotor_interface->motor_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The motor_array pointer for the Compumotor interface '%s' is NULL.",
			compumotor_interface->record->name );
	}

	i = compumotor->controller_index;

	num_axes = compumotor_interface->num_axes[i];

	motor_array = compumotor_interface->motor_array[i];

	sprintf( command, "%d_!%dMC%d", compumotor->controller_number,
					compumotor->axis_number,
					enable_flag );

	/* Send the MC command. */

	mx_status = mxi_compumotor_command( compumotor_interface, command,
			NULL, 0, COMPUMOTOR_DEBUG );

	if ( mx_status.code == MXE_SUCCESS ) {
		compumotor->continuous_mode_enabled = enable_flag;
	}

	return mx_status;
}

