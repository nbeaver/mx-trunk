/*
 * Name:    d_vp9000.c
 *
 * Purpose: MX motor driver for the Velmex VP9000 series of motor controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "d_vp9000.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_vp9000_record_function_list = {
	NULL,
	mxd_vp9000_create_record_structures,
	mxd_vp9000_finish_record_initialization,
	NULL,
	mxd_vp9000_print_structure,
	NULL,
	NULL,
	mxd_vp9000_open,
	NULL,
	NULL,
	mxd_vp9000_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_vp9000_motor_function_list = {
	mxd_vp9000_motor_is_busy,
	mxd_vp9000_move_absolute,
	mxd_vp9000_get_position,
	mxd_vp9000_set_position,
	mxd_vp9000_soft_abort,
	mxd_vp9000_immediate_abort,
	mxd_vp9000_positive_limit_hit,
	mxd_vp9000_negative_limit_hit,
	mxd_vp9000_find_home_position
};

/* VP9000 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_vp9000_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_VP9000_STANDARD_FIELDS
};

long mxd_vp9000_num_record_fields
		= sizeof( mxd_vp9000_record_field_defaults )
			/ sizeof( mxd_vp9000_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_vp9000_rfield_def_ptr
			= &mxd_vp9000_record_field_defaults[0];

#define MXD_VP9000_DEBUG	FALSE

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxd_vp9000_get_record_pointers( MX_RECORD *record,
				MX_MOTOR **motor,
				const char *calling_fname )
{
	static const char fname[] = "mxd_vp9000_get_record_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( (record->mx_superclass != MXR_DEVICE)
	  || (record->mx_class != MXC_MOTOR)
	  || (record->mx_type != MXT_MTR_VP9000) ) {

		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' is not a Velmex VP9000 motor record.",
			record->name );
	}

	if ( motor == (MX_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*motor = (MX_MOTOR *) (record->record_class_struct);

	if ( *motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_vp9000_get_pointers( MX_MOTOR *motor,
				MX_VP9000_MOTOR **vp9000_motor,
				MX_VP9000 **vp9000,
				const char *calling_fname )
{
	static const char fname[] = "mxd_vp9000_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vp9000_motor == (MX_VP9000_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_VP9000_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vp9000_motor = (MX_VP9000_MOTOR *) (motor->record->record_type_struct);

	if ( *vp9000_motor == (MX_VP9000_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VP9000_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( (*vp9000_motor)->interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The interface record pointer for VP9000 motor '%s' is NULL.",
			motor->record->name );
	}

	if ( vp9000 == (MX_VP9000 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_VP9000_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vp9000 = (MX_VP9000 *)
		((*vp9000_motor)->interface_record->record_type_struct);

	if ( *vp9000 == (MX_VP9000 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_VP9000 pointer for interface record '%s' is NULL.",
			(*vp9000_motor)->interface_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_vp9000_check_encoder_registration( MX_MOTOR *motor,
			MX_VP9000 *vp9000,
			MX_VP9000_MOTOR *vp9000_motor )
{
	static const char fname[] = "mxd_vp9000_check_encoder_registration()";

	char response[20];
	mx_status_type status;

	if ( vp9000_motor->vp9000_flags & MXF_VP9000_DISABLE_ENCODER_CHECK )
		return MX_SUCCESSFUL_RESULT;

	/* Check to see if the motor and encoder counts agree */

	status = mxi_vp9000_command( vp9000, vp9000_motor->controller_number,
			"%", response, sizeof(response), MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	switch( response[0] ) {
	case '=':   /* Everything is OK. */
		break;
	case '<':
		return mx_error(MXE_DEVICE_ACTION_FAILED,fname,
		    "The motor '%s' did not reach the requested position.  "
		    "This probably means that it stalled.",
				motor->record->name );
		break;
	case '>':
		return mx_error(MXE_DEVICE_ACTION_FAILED,fname,
			"The motor '%s' overshot the requested position.",
			motor->record->name );
		break;
	default:
		return mx_error(MXE_INTERFACE_IO_ERROR, fname,
			"Got unexpected response '%s' to '%%' command.",
			response );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxd_vp9000_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_vp9000_create_record_structures()";

	MX_MOTOR *motor;
	MX_VP9000_MOTOR *vp9000_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	vp9000_motor = (MX_VP9000_MOTOR *) malloc( sizeof(MX_VP9000_MOTOR) );

	if ( vp9000_motor == (MX_VP9000_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VP9000_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = vp9000_motor;
	record->class_specific_function_list
				= &mxd_vp9000_motor_function_list;

	motor->record = record;

	/* A VP9000 motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vp9000_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_vp9000_finish_record_initialization()";

	MX_VP9000_MOTOR *vp9000_motor;
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	vp9000_motor = (MX_VP9000_MOTOR *) record->record_type_struct;

	if ( vp9000_motor == (MX_VP9000_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_VP9000_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	if ( vp9000_motor->controller_number <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal controller number %d.  "
		"The VP9000 controller number must be greater than zero.",
			vp9000_motor->controller_number );
	}
	if ( (vp9000_motor->motor_number < 1)
	  || (vp9000_motor->motor_number > 4) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal motor number %d.  "
		"The VP9000 motor number must be in the range [1-4].",
			vp9000_motor->motor_number );
	}

	vp9000_motor->motor_is_moving = FALSE;
	vp9000_motor->last_move_direction = 1;
	vp9000_motor->positive_limit_latch = FALSE;
	vp9000_motor->negative_limit_latch = FALSE;

	vp9000_motor->last_start_position = 0;
	vp9000_motor->last_start_time = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vp9000_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_vp9000_print_structure()";

	MX_MOTOR *motor;
	MX_GENERIC *generic;
	MX_RECORD *interface_record;
	MX_VP9000_MOTOR *vp9000_motor;
	double position, move_deadband;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	vp9000_motor = (MX_VP9000_MOTOR *) (record->record_type_struct);

	if ( vp9000_motor == (MX_VP9000_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_VP9000_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	interface_record = (MX_RECORD *)
			(vp9000_motor->interface_record);

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Interface record pointer for VP9000 motor '%s' is NULL.",
			record->name );
	}

	generic = (MX_GENERIC *)
			interface_record->record_class_struct;

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GENERIC pointer for record '%s' is NULL.", record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = VP9000 motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  port name         = %s\n", generic->record->name);
	fprintf(file, "  controller number = %d\n",
					vp9000_motor->controller_number);
	fprintf(file, "  motor number      = %d\n", vp9000_motor->motor_number);

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
	fprintf(file, "  scale             = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit    = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit    = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband     = %g %s  (%ld steps)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	fprintf(file, "  raw speed         = %ld steps/second\n",
		vp9000_motor->vp9000_speed );
	fprintf(file, "  raw acceleration  = %ld steps/(second^2)\n\n",
		vp9000_motor->vp9000_acceleration );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vp9000_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_vp9000_open()";

	MX_MOTOR *motor;
	MX_VP9000_MOTOR *vp9000_motor;
	MX_VP9000 *vp9000;
	char command[40];
	long acceleration_parameter;
	mx_status_type status;

	status = mxd_vp9000_get_record_pointers( record, &motor, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_vp9000_get_pointers(motor, &vp9000_motor, &vp9000, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Set the raw speed. */

	sprintf( command, "S%dM%ld", vp9000_motor->motor_number,
				vp9000_motor->vp9000_speed );

	status = mxi_vp9000_command( vp9000, vp9000_motor->controller_number,
					command, NULL, 0, MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Set the raw acceleration. */

	acceleration_parameter
		= ( vp9000_motor->vp9000_acceleration ) / 1000L;

	sprintf( command, "A%dM%ld", vp9000_motor->motor_number,
				acceleration_parameter );

	status = mxi_vp9000_command( vp9000, vp9000_motor->controller_number,
					command, NULL, 0, MXD_VP9000_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_vp9000_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_vp9000_resynchronize()";

	MX_VP9000_MOTOR *vp9000_motor;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	vp9000_motor = (MX_VP9000_MOTOR *) record->record_type_struct;

	if ( vp9000_motor == (MX_VP9000_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_VP9000_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	status = mxi_vp9000_resynchronize(
			vp9000_motor->interface_record );

	return status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_vp9000_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_motor_is_busy()";

	MX_GENERIC *generic;
	MX_VP9000 *vp9000;
	MX_VP9000_MOTOR *vp9000_motor;
	char c, c2;
	int i;
	unsigned long num_input_bytes_available;
	mx_status_type status;

	status = mxd_vp9000_get_pointers(motor, &vp9000_motor, &vp9000, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( (vp9000->active_controller == 0) && (vp9000->active_motor == 0)) {

		motor->busy = FALSE;
	} else {
		if ( vp9000_motor->motor_is_moving == FALSE ) {
			motor->busy = FALSE;
		} else {
			/* Is there any input available on the VP9000
			 * serial port?
			 */

			generic = (MX_GENERIC *)
			  vp9000_motor->interface_record->record_class_struct;

			status = mxi_vp9000_num_input_bytes_available( generic,
						&num_input_bytes_available );

			if ( status.code != MXE_SUCCESS )
				return status;

			if ( num_input_bytes_available == 0 ) {
			    motor->busy = TRUE;
			} else {
			    /* Read the character available on the
			     * interface.
			     */

			    status = mxi_vp9000_getc( vp9000,
					vp9000_motor->controller_number,
					&c, MXF_GENERIC_WAIT );

			    if ( status.code != MXE_SUCCESS )
				return status;

			    switch ( c ) {
			    case '^':
				/* The current move is complete. */

				vp9000_motor->motor_is_moving = FALSE;
				vp9000->active_controller = 0;
				vp9000->active_motor = 0;

				/* There is also a carriage return
				 * to throw away.
				 */

				status = mxi_vp9000_getc( vp9000,
					vp9000_motor->controller_number,
						&c2, MXF_GENERIC_WAIT );

				if ( status.code != MXE_SUCCESS )
					return status;

				break;
			    case 'O':
				/* The current move was stopped by
				 * a limit switch.
				 */

				vp9000_motor->motor_is_moving = FALSE;
				vp9000->active_controller = 0;
				vp9000->active_motor = 0;

				if (vp9000_motor->last_move_direction >= 0) {
				    vp9000_motor->positive_limit_latch = TRUE;
				    vp9000_motor->negative_limit_latch = FALSE;
				} else {
				    vp9000_motor->positive_limit_latch = FALSE;
				    vp9000_motor->negative_limit_latch = TRUE;
				}

				/* The 'O' character is followed
				 * by the three character sequence
				 * <CR>^<CR>.  Throw all three of
				 * these characters away.
				 */
				for ( i = 0; i < 3; i++ ) {
				    status = mxi_vp9000_getc( vp9000,
					vp9000_motor->controller_number,
					&c2, MXF_GENERIC_WAIT);

					if (status.code != MXE_SUCCESS)
					    return status;
				}
				break;
			    default:
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Unexpected character 0x%x '%c' received on VP9000 interface '%s'",
						c, c, vp9000->record->name );
				break;
			    }

			    /* Check to see if the motor and encoder counts
			     * agree with each other.
			     */

			    status = mxd_vp9000_check_encoder_registration(
						motor, vp9000, vp9000_motor );

			    if ( status.code != MXE_SUCCESS )
				return status;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vp9000_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_move_absolute()";

	MX_VP9000 *vp9000;
	MX_VP9000_MOTOR *vp9000_motor;
	char command[20];
	long absolute_steps, relative_steps;
	mx_status_type status;

	status = mxd_vp9000_get_pointers(motor, &vp9000_motor, &vp9000, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Set the position to move to. */

	absolute_steps = motor->raw_destination.stepper;

	relative_steps = absolute_steps - motor->raw_position.stepper;

	if ( relative_steps >= 0 ) {
		vp9000_motor->last_move_direction = 1;
	} else {
		vp9000_motor->last_move_direction = -1;
	}

	vp9000_motor->last_start_position = motor->raw_position.stepper;

	vp9000_motor->last_start_time = time( NULL );

	sprintf( command, "PM-0,IA%dM%ld,R",
			vp9000_motor->motor_number, absolute_steps );

	status = mxi_vp9000_command( vp9000, vp9000_motor->controller_number,
				command, NULL, 0, MXD_VP9000_DEBUG );

	vp9000->active_controller = vp9000_motor->controller_number;
	vp9000->active_motor = vp9000_motor->motor_number;

	vp9000_motor->motor_is_moving = TRUE;
	vp9000_motor->positive_limit_latch = FALSE;
	vp9000_motor->negative_limit_latch = FALSE;

	return status;
}

MX_EXPORT mx_status_type
mxd_vp9000_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_get_position()";

	MX_VP9000 *vp9000;
	MX_VP9000_MOTOR *vp9000_motor;
	char command[20];
	char response[50];
	long motor_steps, steps_moved;
	long estimated_position;
	int num_tokens;
	double time_since_move_started;
	double steps_moved_temp;
	time_t current_time;
	mx_status_type status;

	status = mxd_vp9000_get_pointers(motor, &vp9000_motor, &vp9000, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Is this motor in motion?  If so, estimate its current position
	 * using dead reckoning.  The acceleration time is ignored by
	 * this calculation.
	 */

	if ( vp9000_motor->motor_is_moving == TRUE ) {

		current_time = time( NULL );

		time_since_move_started
		    = difftime( current_time, vp9000_motor->last_start_time );

		steps_moved_temp = (double) vp9000_motor->vp9000_speed * 
						time_since_move_started;

		steps_moved = mx_round( steps_moved_temp );

		estimated_position = vp9000_motor->last_start_position
			+ ( vp9000_motor->last_move_direction * steps_moved );

		motor->raw_position.stepper = estimated_position;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If some other motor is in motion, it is not safe to send a query
	 * to the controller.  In this case, we keep the previously read
	 * value of the motor position and return.
	 */

	if ((vp9000->active_controller != 0) || (vp9000->active_motor != 0))
		return MX_SUCCESSFUL_RESULT;

	/* Otherwise, we send a command to the controller to get
	 * the position.
	 */

	switch ( vp9000_motor->motor_number ) {
	case 1: strcpy( command, "X" );
		break;
	case 2: strcpy( command, "Y" );
		break;
	case 3: strcpy( command, "Z" );
		break;
	case 4: strcpy( command, "T" );
		break;
	}

	status = mxi_vp9000_command( vp9000, vp9000_motor->controller_number,
			command, response, sizeof response, MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_tokens = sscanf( response, "%ld", &motor_steps );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to '%s' command "
		"for motor '%s'.  Response seen = '%s'",
			command, motor->record->name, response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vp9000_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_set_position()";

	MX_VP9000 *vp9000;
	MX_VP9000_MOTOR *vp9000_motor;
	char command[100];
	char response[20];
	long new_position;
	mx_status_type status;

	status = mxd_vp9000_get_pointers(motor, &vp9000_motor, &vp9000, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	/* This controller can only set the current position to zero.
	 * Reject any attempts to set it to anything else.
	 */

	new_position = motor->raw_set_position.stepper;

	if ( new_position != 0L ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The position of motor '%s' may be set to zero, but not to any other value.",
			motor->record->name );
	}

	/* If some motor is in motion, it is not safe to send a query
	 * to the controller.  In this case, we send an error back
	 * to the caller.
	 */

	if ((vp9000->active_controller != 0) || (vp9000->active_motor != 0)) {
		return mx_error( MXE_NOT_READY, fname,
			"Cannot set the motor position to zero for motor '%s' "
			"while controller %d, motor %d "
			"for interface '%s' is in motion.",
				motor->record->name,
				vp9000->active_controller,
				vp9000->active_motor,
				vp9000->record->name );
	}

	sprintf( command, "PM-0,IA%dM-0,R", vp9000_motor->motor_number );

	status = mxi_vp9000_command( vp9000, vp9000_motor->controller_number,
					command, NULL, 0, MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* The controller will return a ^<CR> combination.  Discard these
	 * two characters.
	 */

	status = mxi_vp9000_getline( vp9000, vp9000_motor->controller_number,
				response, sizeof response, MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( strcmp( response, "^" ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not get expected ^<CR> response from zero motor "
			"position command for motor '%s'.  Instead saw '%s'",
				motor->record->name, response );
	}

	return status;
}

MX_EXPORT mx_status_type
mxd_vp9000_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_soft_abort()";

	MX_VP9000 *vp9000;
	MX_VP9000_MOTOR *vp9000_motor;
	char response[20];
	mx_status_type status;

	status = mxd_vp9000_get_pointers(motor, &vp9000_motor, &vp9000, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	/* If we are not the active motor, then just return. */

	if ( vp9000_motor->motor_is_moving == FALSE )
		return MX_SUCCESSFUL_RESULT;

	/* Tell the motor to decelerate to a stop. */

	status = mxi_vp9000_putc( vp9000, vp9000_motor->controller_number,
					'D', MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* The controller will return a ^<CR> combination.  Discard these
	 * two characters and then reset the flags in the various structures.
	 */

	status = mxi_vp9000_getline( vp9000, vp9000_motor->controller_number,
				response, sizeof response, MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( strcmp( response, "^" ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not get expected ^<CR> response from soft abort "
			"command for motor '%s'.  Instead saw '%s'",
				motor->record->name, response );
	}

	vp9000->active_controller = 0;
	vp9000->active_motor = 0;

	vp9000_motor->motor_is_moving = FALSE;

	/* Check to see if the motor and encoder counts agree */

	status = mxd_vp9000_check_encoder_registration(
				motor, vp9000, vp9000_motor );

	return status;
}

MX_EXPORT mx_status_type
mxd_vp9000_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_immediate_abort()";

	MX_VP9000 *vp9000;
	MX_VP9000_MOTOR *vp9000_motor;
	char response[20];
	mx_status_type status;

	status = mxd_vp9000_get_pointers(motor, &vp9000_motor, &vp9000, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	/* If we are not the active motor, then just return. */

	if ( vp9000_motor->motor_is_moving == FALSE )
		return MX_SUCCESSFUL_RESULT;

	/* Tell the controller to kill the operation in progress. */

	status = mxi_vp9000_putc( vp9000, vp9000_motor->controller_number,
					'K', MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* The controller will return a ^<CR> combination.  Discard these
	 * two characters and then reset the flags in the various structures.
	 */

	status = mxi_vp9000_getline( vp9000, vp9000_motor->controller_number,
				response, sizeof response, MXD_VP9000_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( strcmp( response, "^" ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not get expected ^<CR> response from soft abort "
			"command for motor '%s'.  Instead saw '%s'",
				motor->record->name, response );
	}

	vp9000->active_controller = 0;
	vp9000->active_motor = 0;

	vp9000_motor->motor_is_moving = FALSE;

	/* Check to see if the motor and encoder counts agree */

	status = mxd_vp9000_check_encoder_registration(
				motor, vp9000, vp9000_motor );

	return status;
}

MX_EXPORT mx_status_type
mxd_vp9000_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_positive_limit_hit()";

	MX_VP9000_MOTOR *vp9000_motor;

	vp9000_motor = (MX_VP9000_MOTOR *) motor->record->record_type_struct;

	if ( vp9000_motor == (MX_VP9000_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VP9000_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( vp9000_motor->positive_limit_latch == TRUE ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	vp9000_motor->positive_limit_latch = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vp9000_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_negative_limit_hit()";

	MX_VP9000_MOTOR *vp9000_motor;

	vp9000_motor = (MX_VP9000_MOTOR *) motor->record->record_type_struct;

	if ( vp9000_motor == (MX_VP9000_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VP9000_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( vp9000_motor->negative_limit_latch == TRUE ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	vp9000_motor->negative_limit_latch = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vp9000_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_vp9000_find_home_position()";

	MX_VP9000 *vp9000;
	MX_VP9000_MOTOR *vp9000_motor;
	char command[20];
	mx_status_type status;

	status = mxd_vp9000_get_pointers(motor, &vp9000_motor, &vp9000, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->home_search >= 0 ) {
		sprintf( command, "I%dM0", vp9000_motor->motor_number );
	} else {
		sprintf( command, "I%dM-0", vp9000_motor->motor_number );
	}

	/* Command the home search to start. */

	status = mxi_vp9000_command( vp9000, vp9000_motor->controller_number,
				command, NULL, 0, MXD_VP9000_DEBUG );

	return status;
}

