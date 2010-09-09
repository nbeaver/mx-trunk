/*
 * Name:    d_pdi40.c
 *
 * Purpose: MX drivers for the PDI Model 40 DAQ interface.
 *
 *          Currently only the stepping motor devices are supported.
 *
 *          A major restriction of the PDI Model 40 hardware is that
 *          while a stepping motor is in motion, absolutely no other
 *          commands can be sent to the Model 40 until the motion
 *          is done.  Among other things, this precludes more than
 *          one motor from moving at a time.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "d_pdi40.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_pdi40motor_record_function_list = {
	NULL,
	mxd_pdi40motor_create_record_structures,
	mxd_pdi40motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_pdi40motor_open,
	mxd_pdi40motor_close
};

MX_MOTOR_FUNCTION_LIST mxd_pdi40motor_motor_function_list = {
	mxd_pdi40motor_motor_is_busy,
	mxd_pdi40motor_move_absolute,
	mxd_pdi40motor_get_position,
	mxd_pdi40motor_set_position,
	mxd_pdi40motor_soft_abort,
	mxd_pdi40motor_immediate_abort,
	mxd_pdi40motor_positive_limit_hit,
	mxd_pdi40motor_negative_limit_hit,
	mxd_pdi40motor_find_home_position
};

MX_RECORD_FIELD_DEFAULTS mxd_pdi40motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PDI40_MOTOR_STANDARD_FIELDS
};

long mxd_pdi40motor_num_record_fields
		= sizeof( mxd_pdi40motor_record_field_defaults )
			/ sizeof( mxd_pdi40motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pdi40motor_rfield_def_ptr
			= &mxd_pdi40motor_record_field_defaults[0];

MX_EXPORT mx_status_type
mxd_pdi40motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_pdi40motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_PDI40_MOTOR *pdi40motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	pdi40motor = (MX_PDI40_MOTOR *) malloc( sizeof(MX_PDI40_MOTOR) );

	if ( pdi40motor == (MX_PDI40_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PDI40_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = pdi40motor;
	record->class_specific_function_list
				= &mxd_pdi40motor_motor_function_list;

	motor->record = record;

	/* A PDI40 motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi40motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pdi40motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_PDI40_MOTOR *pdi40motor;
	MX_PDI40 *pdi40;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	pdi40motor = (MX_PDI40_MOTOR *) record->record_type_struct;

	pdi40 = (MX_PDI40 *) pdi40motor->pdi40_record->record_type_struct;

	/* Save a pointer to the motor's MX_MOTOR structure in the MX_PDI40
	 * structure, so that the three motors can find each other.
	 */

	switch ( pdi40motor->stepper_name ) {
	case 'A':	pdi40->stepper_motor[ MX_PDI40_STEPPER_A ] = motor;
			break;
	case 'B':	pdi40->stepper_motor[ MX_PDI40_STEPPER_B ] = motor;
			break;
	case 'C':	pdi40->stepper_motor[ MX_PDI40_STEPPER_C ] = motor;
			break;
	default:	return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Illegal PDI40 stepper name '%c'",
				pdi40motor->stepper_name );
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi40motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pdi40motor_open()";

	MX_MOTOR *motor;
	MX_PDI40_MOTOR *pdi40_motor;
	MX_PDI40 *pdi40;
	char buffer[80];
	char c;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	pdi40_motor = (MX_PDI40_MOTOR *) (record->record_type_struct);

	if ( pdi40_motor == (MX_PDI40_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"MX_PDI40_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	pdi40 = (MX_PDI40 *) pdi40_motor->pdi40_record->record_type_struct;

	if ( pdi40 == (MX_PDI40 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"MX_PDI40 pointer for record '%s' is NULL.",
			record->name );
	}

	MX_DEBUG(2, ("mxd_pdi40motor_write_parms_to_hardware:"));
	MX_DEBUG(2, ("  motor = %p, pdi40_motor = %p, pdi40 = %p",
		motor, pdi40_motor, pdi40) );
	MX_DEBUG(2, ("Record name = '%s', PDI40 name = '%s', name = '%c'",
		record->name, pdi40->record->name, pdi40_motor->stepper_name));
	MX_DEBUG(2, ("  stepper mode = '%c', speed = %ld, stop delay = %ld",
		pdi40->stepper_mode, pdi40->stepper_speed,
		pdi40->stepper_stop_delay));

	/* Send the command to the PDI 40. */

	snprintf( buffer, sizeof(buffer), "SE%c%c%ld;%ld",
			pdi40_motor->stepper_name,
			pdi40->stepper_mode,
			pdi40->stepper_speed,
			pdi40->stepper_stop_delay );

	MX_DEBUG(2, ("SE buffer = '%s'", buffer) );

	mx_status = mxi_pdi40_putline( pdi40, buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read out the response. It should be a blank line, "OK",
	 * a blank line, and then '>'.
	 */

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( buffer, "" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get blank line before the 'OK' response; instead saw '%s'",
			buffer );
	}

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( buffer, "OK" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get 'OK' response to PDI40 command.  Response was '%s'",
			buffer );
	}

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( buffer, "" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get blank line after the 'OK' response; instead saw '%s'",
			buffer );
	}

	mx_status = mxi_pdi40_getc( pdi40, &c, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != MX_PDI40_END_OF_RESPONSE ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not see the PDI40 end of response character; instead saw '%c'",
			c );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* Note that the values read in by mxd_pdi40motor_close()
 * apply to all three steppers, not just one.
 */

MX_EXPORT mx_status_type
mxd_pdi40motor_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_pdi40motor_close()";

	MX_MOTOR *motor;
	MX_PDI40_MOTOR *pdi40_motor;
	MX_PDI40 *pdi40;
	char buffer[80];
	char stepper_mode_string[40];
	int i, num_items, stepper_speed, stepper_stop_delay;
	char c;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	pdi40_motor = (MX_PDI40_MOTOR *) (record->record_type_struct);

	if ( pdi40_motor == (MX_PDI40_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"MX_PDI40_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	pdi40 = (MX_PDI40 *) pdi40_motor->pdi40_record->record_type_struct;

	if ( pdi40 == (MX_PDI40 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
			"MX_PDI40 pointer for record '%s' is NULL.",
			record->name );
	}

	/* Send the command to the PDI40. */

	mx_status = mxi_pdi40_putline( pdi40, "S?", FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read out the response.  It should be:
	 *  1.  A blank line.
	 *  2.  "OK"
	 *  3.  A line containing a verbose version of the stepper mode.
	 *  4.  A line containing the stepper speed and stop delay.
	 *  5.  Three lines containing the port values.
	 *  6.  A blank line.
	 *  7.  The character '>'.
	 */

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( buffer, "" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get blank line before the 'OK' response; instead saw '%s'",
			buffer );
	}

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( strcmp( buffer, "OK" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get 'OK' response to PDI40 command.  Response was '%s'",
			buffer );
	}

	/* Get the stepper mode. */

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( buffer, "%s", stepper_mode_string );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Couldn't get stepper mode from the PDI40; instead saw '%s'",
			buffer );
	}

	if ( strcmp( stepper_mode_string, "Monophasic" ) == 0 ) {
		pdi40->stepper_mode = 'M';
	} else
	if ( strcmp( stepper_mode_string, "Biphasic" ) == 0 ) {
		pdi40->stepper_mode = 'B';
	} else
	if ( strcmp( stepper_mode_string, "Half" ) == 0 ) {
		pdi40->stepper_mode = 'H';
	} else {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognized PDI40 stepper mode = '%s'", buffer );
	}

	/* Get the stepper speed and stop delay. */

	mx_status = mxi_pdi40_getline( pdi40, buffer, sizeof buffer, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( buffer,
		"   Speed (steps/s) = %d   Stop delay = %d",
		&stepper_speed, &stepper_stop_delay );

	if ( num_items != 2 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Couldn't get stepper speed and stop delay; instead saw '%s'",
			buffer );
	}

	pdi40->stepper_speed = stepper_speed;
	pdi40->stepper_stop_delay = stepper_stop_delay;

	/* We don't currently use the rest of the output of the PDI40,
	 * so we read it out and throw it away.
	 */

	for ( i = 0; i < 4; i++ ) {
		mx_status = mxi_pdi40_getline( pdi40,
				buffer, sizeof buffer, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mxi_pdi40_getc( pdi40, &c, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != MX_PDI40_END_OF_RESPONSE ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not see the PDI40 end of response character; instead saw '%c'",
			c );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_pdi40motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pdi40motor_motor_is_busy()";

	MX_PDI40 *pdi40;
	MX_PDI40_MOTOR *pdi40_motor;
	mx_bool_type a_motor_is_busy;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	pdi40_motor = (MX_PDI40_MOTOR *) (motor->record->record_type_struct);

	if ( pdi40_motor == (MX_PDI40_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PDI40_MOTOR pointer for motor is NULL." );
	}

	pdi40 = (MX_PDI40 *) pdi40_motor->pdi40_record->record_type_struct;

	if ( pdi40 == (MX_PDI40 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PDI40 pointer for motor is NULL." );
	}

	/* Check to see if any motor is moving.  If one is, then say 
	 * the motor we're asking about is busy, even if it isn't
	 * the one that is actually moving.  After all, even if
	 * our motor isn't moving, we still can't send commands
	 * to it until the other motor stops moving.
	 */

	mx_status = mxi_pdi40_is_any_motor_busy( pdi40, &a_motor_is_busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( a_motor_is_busy == FALSE ) {
		motor->busy = FALSE;
	} else {
		motor->busy = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi40motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pdi40motor_move_absolute()";

	MX_PDI40 *pdi40;
	MX_PDI40_MOTOR *pdi40_motor;
	char command[20];
	long motor_steps, current_position, relative_steps;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	pdi40_motor = (MX_PDI40_MOTOR *) (motor->record->record_type_struct);

	if ( pdi40_motor == (MX_PDI40_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PDI40_MOTOR pointer for motor is NULL." );
	}

	pdi40 = (MX_PDI40 *) pdi40_motor->pdi40_record->record_type_struct;

	if ( pdi40 == (MX_PDI40 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PDI40 pointer for motor is NULL." );
	}

	motor_steps = motor->raw_destination.stepper;

	/* Get where the motor is now and compute a relative move. */

	mx_status = mx_motor_get_position_steps(motor->record, &current_position);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	relative_steps = motor_steps - current_position;

	/* Format the move command and send it. */

	if ( relative_steps >= 0 ) {
		snprintf( command, sizeof(command),
				"S%cR%ld",
				pdi40_motor->stepper_name,
				relative_steps );
	} else {
		snprintf( command, sizeof(command),
				"S%cL%ld",
				pdi40_motor->stepper_name,
				- relative_steps );
	}

	mx_status = mxi_pdi40_putline( pdi40, command, FALSE );

	/* No other commands (besides abort move) may be sent to the PDI40
	 * until the currently moving motor finishes its motion, so we
	 * need to record which motor is moving in the MX_PDI40 structure.
	 */

	pdi40->currently_moving_stepper = pdi40_motor->stepper_name;
	pdi40->current_move_distance = relative_steps;

	return mx_status;
}

/* The motor positions are maintained entirely in software. */

MX_EXPORT mx_status_type
mxd_pdi40motor_get_position( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi40motor_set_position( MX_MOTOR *motor )
{
	motor->raw_position.stepper = motor->raw_set_position.stepper;

	return MX_SUCCESSFUL_RESULT;
}

/* There is only one kind of abort for the PDI Model 40 stepping motors. */

MX_EXPORT mx_status_type
mxd_pdi40motor_soft_abort( MX_MOTOR *motor )
{
	return mxd_pdi40motor_immediate_abort( motor );
}

MX_EXPORT mx_status_type
mxd_pdi40motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pdi40motor_immediate_abort()";

	MX_PDI40 *pdi40;
	MX_PDI40_MOTOR *pdi40_motor;
	MX_MOTOR *last_moving_motor;
	char response[50];
	char c;
	int num_items;
	long steps_to_go;
	mx_bool_type busy;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed is NULL." );
	}

	pdi40_motor = (MX_PDI40_MOTOR *) motor->record->record_type_struct;

	if ( pdi40_motor == (MX_PDI40_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PDI40MOTOR pointer for motor is NULL." );
	}

	pdi40 = (MX_PDI40 *) pdi40_motor->pdi40_record->record_type_struct;

	if ( pdi40 == (MX_PDI40 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PDI40 pointer for motor is NULL." );
	}

	/* Is the motor currently moving? */

	mx_status = mx_motor_is_busy( motor->record, &busy );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	/* Don't need to do anything if the motor is not moving. */

	if ( busy == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we have to send the soft stop command. */

	mx_status = mxi_pdi40_putline( pdi40, "S", FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The first line returned is blank. */

	mx_status = mxi_pdi40_getline( pdi40, response, sizeof response, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see blank line prior to the STEPS TO GO message." );
	}

	/* Read the number of steps to go that were left. */

	mx_status = mxi_pdi40_getline( pdi40, response, sizeof response, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld steps to go", &steps_to_go );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see number of steps to go; instead saw '%s'",
			response );
	}

	/* Update the software position of the motor that was moving.
	 * Note that the moving motor may not have been this motor.
	 */

	switch( pdi40->currently_moving_stepper ) {
	case 'A':	last_moving_motor
				= pdi40->stepper_motor[ MX_PDI40_STEPPER_A ];
			break;
	case 'B':	last_moving_motor
				= pdi40->stepper_motor[ MX_PDI40_STEPPER_B ];
			break;
	case 'C':	last_moving_motor
				= pdi40->stepper_motor[ MX_PDI40_STEPPER_C ];
			break;
	default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The PDI40 motor that supposedly just stopped has an illegal name = '%c'",
				pdi40->currently_moving_stepper );
	}

	if ( last_moving_motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
"The PDI40 motor that supposedly just stopped (%c) is not in the database.",
				pdi40->currently_moving_stepper );
	}

	/* Update the position. */

	last_moving_motor->raw_position.stepper
					+= pdi40->current_move_distance;

	if ( pdi40->current_move_distance >= 0 ) {
		last_moving_motor->raw_position.stepper -= steps_to_go;
	} else {
		last_moving_motor->raw_position.stepper += steps_to_go;
	}

	/* Clean up the status variables. */

	pdi40->currently_moving_stepper = '\0';
	pdi40->current_move_distance = 0;

	/* Finish the handshaking with the PDI40 and return. */

	mx_status = mxi_pdi40_getline( pdi40, response, sizeof response, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "OK" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see 'OK' response from PDI40; instead saw '%s'",
			response );
	}

	mx_status = mxi_pdi40_getline( pdi40, response, sizeof response, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "" ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not see blank line after the 'OK' message." );
	}

	mx_status = mxi_pdi40_getc( pdi40, &c, FALSE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != MX_PDI40_END_OF_RESPONSE ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not see the PDI40 end of response character; instead saw '%c'",
			c );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi40motor_positive_limit_hit( MX_MOTOR *motor )
{
	/* PDI Model 40 stepping motors do not have limit switches. */

	motor->positive_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi40motor_negative_limit_hit( MX_MOTOR *motor )
{
	/* PDI Model 40 stepping motors do not have limit switches. */

	motor->negative_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi40motor_find_home_position( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

