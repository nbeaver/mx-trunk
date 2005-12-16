/*
 * Name:    d_mcu2.c
 *
 * Purpose: MX driver for the Advanced Control Systems MCU-2 motor controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MCU2_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "d_mcu2.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_mcu2_record_function_list = {
	NULL,
	mxd_mcu2_create_record_structures,
	mxd_mcu2_finish_record_initialization,
	NULL,
	mxd_mcu2_print_structure,
	NULL,
	NULL,
	mxd_mcu2_open,
	NULL,
	NULL,
	mxd_mcu2_open
};

MX_MOTOR_FUNCTION_LIST mxd_mcu2_motor_function_list = {
	NULL,
	mxd_mcu2_move_absolute,
	mxd_mcu2_get_position,
	mxd_mcu2_set_position,
	mxd_mcu2_soft_abort,
	mxd_mcu2_immediate_abort,
	NULL,
	NULL,
	mxd_mcu2_find_home_position,
	mxd_mcu2_constant_velocity_move,
	mxd_mcu2_get_parameter,
	mxd_mcu2_set_parameter,
	NULL,
	mxd_mcu2_get_status
};

/* MCU-2 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mcu2_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_MCU2_STANDARD_FIELDS
};

long mxd_mcu2_num_record_fields
		= sizeof( mxd_mcu2_record_field_defaults )
			/ sizeof( mxd_mcu2_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mcu2_rfield_def_ptr
			= &mxd_mcu2_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_mcu2_get_pointers( MX_MOTOR *motor,
			MX_MCU2 **mcu2,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mcu2_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mcu2 == (MX_MCU2 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCU2 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mcu2 = (MX_MCU2 *) motor->record->record_type_struct;

	if ( *mcu2 == (MX_MCU2 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCU2 pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_mcu2_command( MX_MCU2 *mcu2,
		char *command,
		char *response,
		size_t max_response_length,
		int debug_flag )
{
	static const char fname[] = "mxd_mcu2_command()";

	char local_command_buffer[100];
	char local_response_buffer[100];
	char *address_ptr, *response_ptr;
	unsigned long flags;
	mx_status_type mx_status;

	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command buffer pointer passed was NULL." );
	}

	flags = mcu2->mcu2_flags;

	if ( flags & MXF_MCU2_NO_START_CHARACTER ) {
		sprintf( local_command_buffer, "%02d%s",
					mcu2->axis_address, command );

		address_ptr = local_command_buffer;
	} else {
		sprintf( local_command_buffer, "#%02d%s",
					mcu2->axis_address, command );

		address_ptr = local_command_buffer + 1;
	}

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, local_command_buffer, mcu2->record->name));
	}

	mx_status = mx_rs232_putline( mcu2->rs232_record,
					local_command_buffer, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read back the response. */

	mx_status = mx_rs232_getline( mcu2->rs232_record,
					local_response_buffer,
					sizeof(local_response_buffer) - 1,
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, local_response_buffer, mcu2->record->name ));
	}

	/* If we got a valid response, the first two characters should match
	 * the number at the start of the command.
	 */

	if ( ( address_ptr[0] != local_response_buffer[0] )
	  || ( address_ptr[1] != local_response_buffer[1] ) )
	{
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The axis address in the response '%s' does not match the "
		"axis address in the original command '%s' for MCU-2 '%s'.",
			local_response_buffer, local_command_buffer,
			mcu2->record->name );
	}

	if ( response != (char *) NULL ) {
		response_ptr = &local_response_buffer[2];

		strlcpy( response, response_ptr, max_response_length );
	}

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxd_mcu2_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mcu2_create_record_structures()";

	MX_MOTOR *motor;
	MX_MCU2 *mcu2;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	mcu2 = (MX_MCU2 *) malloc( sizeof(MX_MCU2) );

	if ( mcu2 == (MX_MCU2 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MCU2 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = mcu2;
	record->class_specific_function_list = &mxd_mcu2_motor_function_list;

	motor->record = record;
	mcu2->record = record;

	/* A MCU-2 motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The MCU-2 reports acceleration in steps/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcu2_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_mcu2_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_MCU2 *mcu2;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcu2_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_mcu2_print_structure()";

	MX_MOTOR *motor;
	MX_MCU2 *mcu2;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = MCU-2 motor.\n\n");

	fprintf(file, "  name               = %s\n", record->name);
	fprintf(file, "  RS-232 record name = %s\n",
					mcu2->rs232_record->name);
	fprintf(file, "  axis address       = %d\n",
					mcu2->axis_address);
	fprintf(file, "  mcu2_flags         = %#lx\n", mcu2->mcu2_flags);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position           = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
	fprintf(file, "  scale              = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash           = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit     = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit     = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband      = %g %s  (%ld steps)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed              = %g %s/s  (%g steps/s)\n",
		speed, motor->units,
		motor->raw_speed );

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcu2_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mcu2_open()";

	MX_MOTOR *motor;
	MX_MCU2 *mcu2;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any outstanding characters. */

	mx_status = mx_rs232_discard_unwritten_output( mcu2->rs232_record,
							MXD_MCU2_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( mcu2->rs232_record,
							MXD_MCU2_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the controller is there by asking for the
	 * current position.
	 */

	mx_status = mx_motor_get_position( record, NULL );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_mcu2_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_move_absolute()";

	MX_MCU2 *mcu2;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "G%+ld",
			motor->raw_destination.stepper );

	mx_status = mxd_mcu2_command( mcu2, command, NULL, 0, MXD_MCU2_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcu2_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_get_position()";

	MX_MCU2 *mcu2;
	char response[80];
	long motor_steps;
	int num_items;
	char *ptr;
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mcu2_command( mcu2, "P",
					response, sizeof response,
					MXD_MCU2_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command 'P' by "
		"MCU-2 '%s'.  Response = '%s'",
			motor->record->name, response );
	}

	ptr++;

	num_items = sscanf( ptr, "%ld", &motor_steps );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to 'P' command.  "
		"Response seen = '%s'", response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcu2_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_set_position()";

	MX_MCU2 *mcu2;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "P=%+ld",
			motor->raw_set_position.stepper );

	mx_status = mxd_mcu2_command( mcu2, command, NULL, 0, MXD_MCU2_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcu2_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_soft_abort()";

	MX_MCU2 *mcu2;
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mcu2_command( mcu2, "F", NULL, 0, MXD_MCU2_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcu2_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_immediate_abort()";

	MX_MCU2 *mcu2;
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_mcu2_command( mcu2, "Q", NULL, 0, MXD_MCU2_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcu2_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_find_home_position()";

	MX_MCU2 *mcu2;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mcu2->mcu2_flags & MXF_MCU2_HOME_TO_LIMIT_SWITCH ) {
		if ( motor->home_search >= 0 ) {
			strcpy( command, "L+" );
		} else {
			strcpy( command, "L-" );
		}
	} else {
		if ( motor->home_search >= 0 ) {
			strcpy( command, "H+" );
		} else {
			strcpy( command, "H-" );
		}
	}

	mx_status = mxd_mcu2_command( mcu2, command, NULL, 0, MXD_MCU2_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcu2_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_constant_velocity_move()";

	MX_MCU2 *mcu2;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the direction of the move. */

	if ( motor->constant_velocity_move >= 0 ) {
		strcpy( command, "S+" );
	} else {
		strcpy( command, "S-" );
	}

	mx_status = mxd_mcu2_command( mcu2, command, NULL, 0, MXD_MCU2_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcu2_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_get_parameter()";

	MX_MCU2 *mcu2;
	char command[80];
	char response[80];
	char *ptr;
	int num_items;
	unsigned long ulong_value;
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		strcpy( command, "V01" );

		mx_status = mxd_mcu2_command( mcu2, command,
						response, sizeof(response),
						MXD_MCU2_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"MCU-2 '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the speed for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_speed = (double) ulong_value;
		break;
	case MXLV_MTR_BASE_SPEED:
		strcpy( command, "B" );

		mx_status = mxd_mcu2_command( mcu2, command,
						response, sizeof(response),
						MXD_MCU2_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"MCU-2 '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the base speed for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_base_speed = (double) ulong_value;
		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		strcpy( command, "R01" );

		mx_status = mxd_mcu2_command( mcu2, command,
						response, sizeof(response),
						MXD_MCU2_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr = strchr( response, '=' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"No equals sign seen in response to command '%s' by "
			"MCU-2 '%s'.  Response = '%s'",
				command, motor->record->name, response );
		}

		ptr++;

		num_items = sscanf( ptr, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get the acceleration for motor '%s' "
			"in the response to the command '%s'.  "
			"Response = '%s'",
				motor->record->name, command, response );
		}

		motor->raw_acceleration_parameters[0]
				= (double) ( 1000L * ulong_value );

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcu2_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_set_parameter()";

	MX_MCU2 *mcu2;
	char command[80];
	unsigned long ulong_value;
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		ulong_value = (unsigned long) mx_round( motor->raw_speed );

		/* Set the speed for the G command. */

		sprintf( command, "V01=%lu", ulong_value );
		
		mx_status = mxd_mcu2_command( mcu2, command,
						NULL, 0, MXD_MCU2_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the speed for the S command. */

		sprintf( command, "V00=%lu", ulong_value );
		
		mx_status = mxd_mcu2_command( mcu2, command,
						NULL, 0, MXD_MCU2_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	case MXLV_MTR_BASE_SPEED:
		ulong_value = (unsigned long) mx_round( motor->raw_base_speed );

		sprintf( command, "B=%lu", ulong_value );
		
		mx_status = mxd_mcu2_command( mcu2, command,
						NULL, 0, MXD_MCU2_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:

		ulong_value = (unsigned long)
		    mx_round( 0.001 * motor->raw_acceleration_parameters[0] );

		/* Set the acceleration for the G command. */

		sprintf( command, "R01=%lu", ulong_value );
		
		mx_status = mxd_mcu2_command( mcu2, command,
						NULL, 0, MXD_MCU2_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the acceleration for the S command. */

		sprintf( command, "R00=%lu", ulong_value );
		
		mx_status = mxd_mcu2_command( mcu2, command,
						NULL, 0, MXD_MCU2_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			motor->axis_enable = 1;
			strcpy( command, "W=1" );
		} else {
			strcpy( command, "W=0" );
		}

		mx_status = mxd_mcu2_command( mcu2, command,
						NULL, 0, MXD_MCU2_DEBUG );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcu2_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mcu2_get_status()";

	MX_MCU2 *mcu2;
	char command[80];
	char response[80];
	char *ptr;
	int status_length;
	mx_status_type mx_status;

	mx_status = mxd_mcu2_get_pointers( motor, &mcu2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strcpy( command, "E" );

	mx_status = mxd_mcu2_command( mcu2, command,
					response, sizeof response,
					MXD_MCU2_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = strchr( response, '=' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No equals sign seen in response to command '%s' by "
		"MCU-2 '%s'.  Response = '%s'",
			command, motor->record->name, response );
	}

	ptr++;

	status_length = strlen( ptr );

	if ( status_length != 5 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The motion status string '%s' for MCU-2 '%s' should be "
		"5 bytes long.  Instead, it is %d bytes long.",
			ptr, motor->record->name, status_length );
	}

	/* Check all of the status bytes. */

	motor->status = 0;

	/* Byte 0: Negative limit status */

	switch( ptr[0] ) {
	case '-':
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		break;

	case '0': /* Negative limit not hit. */
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal negative limit status byte '%c' seen for "
			"MCU-2 '%s'.  The expected values are '-' or '0'.",
				ptr[0], motor->record->name );
		break;
	}

	/* Byte 1: Home switch status */

	switch( ptr[1] ) {
	case 'H':
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
		break;

	case '0': /* Home switch not hit. */
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal home switch status byte '%c' seen for "
			"MCU-2 '%s'.  The expected values are 'H' or '0'.",
				ptr[1], motor->record->name );
		break;
	}

	/* Byte 2: Positive limit status */

	switch( ptr[2] ) {
	case '+':
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		break;

	case '0': /* Positive limit not hit. */
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal positive limit status byte '%c' seen for "
			"MCU-2 '%s'.  The expected values are '+' or '0'.",
				ptr[2], motor->record->name );
		break;
	}

	/* Byte 3: Controller status */

	switch( ptr[3] ) {
	case 'A': /* Controller is in AUTO mode. */
	case 'M': /* Controller is in MAN mode. */
		break;

	case '0': /* Controller is in OFF mode. */
		motor->status |= MXSF_MTR_AXIS_DISABLED;
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal controller status byte '%c' seen for MCU-2 "
			"'%s'.  The expected values are '0', 'A', or 'M'.",
				ptr[0], motor->record->name );
		break;
	}

	/* Byte 4: Motion status */

	switch( ptr[4] ) {
	case 'S':
	case 'R':
		motor->status |= MXSF_MTR_IS_BUSY;
		break;
	case '0':
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Illegal motion status byte '%c' seen for MCU-2 "
			"'%s'.  The expected values are '0', 'R', or 'S'.",
				ptr[0], motor->record->name );
		break;
	}

	MX_DEBUG( 2,("%s: MX status word = %#lx", fname, motor->status));

	return MX_SUCCESSFUL_RESULT;
}

