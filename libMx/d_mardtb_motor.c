/*
 * Name:    d_mardtb_motor.c 
 *
 * Purpose: MX motor driver for motors controlled by the
 *          MarUSA Desktop Beamline goniostat controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MARDTB_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_dead_reckoning.h"
#include "mx_rs232.h"
#include "i_mardtb.h"
#include "d_mardtb_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mardtb_motor_record_function_list = {
	NULL,
	mxd_mardtb_motor_create_record_structures,
	mxd_mardtb_motor_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_mardtb_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_mardtb_motor_motor_function_list = {
	NULL,
	mxd_mardtb_motor_move_absolute,
	NULL,
	NULL,
	mxd_mardtb_motor_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler,
	NULL,
	NULL,
	mxd_mardtb_motor_get_extended_status
};

/* Mar Desktop Beamline motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mardtb_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_MARDTB_MOTOR_STANDARD_FIELDS
};

long mxd_mardtb_motor_num_record_fields
		= sizeof( mxd_mardtb_motor_record_field_defaults )
			/ sizeof( mxd_mardtb_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mardtb_motor_rfield_def_ptr
			= &mxd_mardtb_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_mardtb_motor_get_pointers( MX_MOTOR *motor,
			MX_MARDTB_MOTOR **mardtb_motor,
			MX_MARDTB **mardtb,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mardtb_motor_get_pointers()";

	MX_MARDTB_MOTOR *mardtb_motor_ptr;
	MX_RECORD *mardtb_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	mardtb_motor_ptr = (MX_MARDTB_MOTOR *)
				motor->record->record_type_struct;

	if ( mardtb_motor_ptr == (MX_MARDTB_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MARDTB_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( mardtb_motor != (MX_MARDTB_MOTOR **) NULL ) {
		*mardtb_motor = mardtb_motor_ptr;
	}

	if ( mardtb != (MX_MARDTB **) NULL ) {
		mardtb_record = mardtb_motor_ptr->mardtb_record;

		if ( mardtb_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The mardtb_record pointer for 'mardtb_motor' record '%s' is NULL.",
				motor->record->name );
		}

		*mardtb = (MX_MARDTB *) mardtb_record->record_type_struct;

		if ( *mardtb == (MX_MARDTB *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MARDTB pointer for 'mardtb' record '%s' is NULL.",
				mardtb_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_mardtb_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
			= "mxd_mardtb_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_MARDTB_MOTOR *mardtb_motor = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	mardtb_motor = (MX_MARDTB_MOTOR *) malloc( sizeof(MX_MARDTB_MOTOR) );

	if ( mardtb_motor == (MX_MARDTB_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MARDTB_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = mardtb_motor;
	record->class_specific_function_list
				= &mxd_mardtb_motor_motor_function_list;

	motor->record = record;

	/* A MarDTB motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The MARDTB_MOTOR reports accelerations in steps/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mardtb_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_mardtb_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_MARDTB_MOTOR *mardtb_motor = NULL;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_mardtb_motor_get_pointers( motor, &mardtb_motor,
							NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the initial speeds and accelerations. */

	motor->raw_speed = mx_divide_safely(
				(double) mardtb_motor->default_speed,
				MX_MARDTB_MOTOR_FUDGE_FACTOR );

	motor->speed = fabs( motor->scale ) * motor->raw_speed;

	motor->raw_acceleration_parameters[0] =
			(double) mardtb_motor->default_acceleration;

	motor->raw_acceleration_parameters[1] = 0.0;
	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	mx_status = mx_dead_reckoning_initialize(
				&(mardtb_motor->dead_reckoning), motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mardtb_motor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_mardtb_motor_resynchronize()";

	MX_MOTOR *motor;
	MX_MARDTB *mardtb = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_mardtb_motor_get_pointers(motor, NULL, &mardtb, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the currently_active_record pointer so that we can escape
	 * any deadlocks.
	 */

	mardtb->currently_active_record = NULL;

	/* Discard any unread RS-232 input or unwritten RS-232 output. */

	mx_status = mx_rs232_discard_unwritten_output( mardtb->rs232_record,
						MXD_MARDTB_MOTOR_DEBUG );

	if ( ( mx_status.code != MXE_SUCCESS )
	  && ( mx_status.code != MXE_UNSUPPORTED ) ) {
		return mx_status;
	}

	mx_status = mx_rs232_discard_unread_input( mardtb->rs232_record,
						MXD_MARDTB_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a get_extended_status command to verify that the controller
	 * is there and to update the current position in the motor structure.
	 */

	mx_status = mx_motor_get_extended_status( record, NULL, NULL );

	/* Check to see if the controller responded. */

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_READY:
		return mx_error( MXE_NOT_READY, fname,
"No response from the Mar goniostat controller for '%s'.  Is it turned on?",
			record->name );
	default:
		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mardtb_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mardtb_motor_move_absolute()";

	MX_MARDTB_MOTOR *mardtb_motor = NULL;
	MX_MARDTB *mardtb = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_mardtb_motor_get_pointers( motor, &mardtb_motor,
							&mardtb , fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));

	if ( mardtb->currently_active_record != NULL ) {
		return mx_error( MXE_TRY_AGAIN, fname,
	"The requested move for motor '%s' cannot begin since a previous move "
	"by motor '%s' is still in progress.  Try the move again after the "
	"move by motor '%s' is complete.",
			motor->record->name,
			mardtb->currently_active_record->name,
			mardtb->currently_active_record->name );
	}

	/* Send the move command. */

	sprintf( command, "stepper_cmd %ld,2,%ld,%ld,%ld",
			mardtb_motor->motor_number,
			motor->raw_destination.stepper,
			mx_round( motor->raw_speed
					* MX_MARDTB_MOTOR_FUDGE_FACTOR ),
			mx_round( motor->raw_acceleration_parameters[0] ) );

	mx_status = mxi_mardtb_command( mardtb, command,
					NULL, 0,
					MXD_MARDTB_MOTOR_DEBUG );

	MX_DEBUG( 2,("%s mx_status.code = %ld", fname, mx_status.code));

	(void) mx_dead_reckoning_start_motion( &(mardtb_motor->dead_reckoning),
						motor->position,
						motor->destination,
						NULL );

	/* Record the fact that a move has been commanded. */

	mardtb->currently_active_record = motor->record;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mardtb_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mardtb_motor_soft_abort()";

	MX_MARDTB_MOTOR *mardtb_motor = NULL;
	MX_MARDTB *mardtb = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_mardtb_motor_get_pointers( motor, &mardtb_motor,
							&mardtb, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "stepper_cmd %ld,1", mardtb_motor->motor_number );

#if 0
	mx_status = mxi_mardtb_command( mardtb, command,
					NULL, 0, MXD_MARDTB_MOTOR_DEBUG );

	return mx_status;
#else
	return mx_error( MXE_UNSUPPORTED, fname,
		"Soft aborts of motor motion for motor '%s' are _not_ "
		"currently possible since the Mar DTB controller does not "
		"pay attention to new commands while a move is in progress.  "
		"Sorry.", motor->record->name );
#endif
}

MX_EXPORT mx_status_type
mxd_mardtb_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_mardtb_motor_get_extended_status()";

	MX_MARDTB_MOTOR *mardtb_motor = NULL;
	MX_MARDTB *mardtb = NULL;
	char command[80];
	char response[100];
	char statusword[80];
	char *ptr;
	int num_items;
	int locno, stop_curr, oper_mode;  /* I am not sure what these do. */
	long motor_position;
	mx_bool_type move_in_progress, old_motor_is_busy;
	mx_status_type mx_status;

	mx_status = mxd_mardtb_motor_get_pointers( motor, &mardtb_motor,
							&mardtb, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s'", fname, motor->record->name));
	MX_DEBUG( 2,("%s: motor->busy = %d", fname, (int) motor->busy));

	old_motor_is_busy = motor->busy;

	if ( motor->busy ) {

		/* If this motor is supposed to be moving, update the
		 * predicted position.
		 */

		mx_status = mx_dead_reckoning_predict_motion(
					&(mardtb_motor->dead_reckoning),
					NULL, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Check to see if any commanded moves are in progress. */

	mx_status = mxi_mardtb_check_for_move_in_progress( mardtb,
							&move_in_progress );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: move_in_progress = %d",
		fname, (int) move_in_progress));

	if ( move_in_progress ) {

		/* If a move is is progress, then we cannot send any new
		 * commands to the controller at this time.  Since the
		 * function mx_dead_reckoning_predict_motion() may change
		 * the value of motor->busy, we restore motor->busy to
		 * the value it had when this function was invoked.
		 */

		motor->busy = old_motor_is_busy;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, then the current axis is _not_ moving. */

	motor->busy = FALSE;

	/* Send the stepper_state command to the goniostat controller. */

	sprintf( command, "stepper_state %ld,1", mardtb_motor->motor_number );

	mx_status = mxi_mardtb_command( mardtb, command,
					response, sizeof (response),
					MXD_MARDTB_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Skip over the first five characters, since this part of the
	 * string seems to have a varying format.
	 */

	ptr = response + 5;

	/* Parse the remaining part of status string. */

	num_items = sscanf( ptr, "%d %s %d %d %ld",
		&locno, statusword, &stop_curr, &oper_mode, &motor_position );

	if ( num_items != 5 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The response to the stepper_state command was not "
		"in the expected format.  Response = '%s'",
			response );
	}

	MX_DEBUG( 2,("%s: locno = %d, stop_curr = %d, oper_mode = %d",
			fname, locno, stop_curr, oper_mode));

	MX_DEBUG( 2,("%s: motor_position = %ld", fname, motor_position));
	MX_DEBUG( 2,("%s: statusword = '%s'", fname, statusword));

	/* Save the reported motor position. */

	motor->raw_position.stepper = motor_position;

	/* Parse the status word.
	 *
	 * FIXME: I am not sure of the identity of the individual bits yet.
	 * FIXME: The array indices below are not yet correct.
	 *
	 */

	motor->positive_limit_hit = FALSE;
	motor->negative_limit_hit = FALSE;

#if 0
	if ( statusword[0] == '1' ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

#endif
	return MX_SUCCESSFUL_RESULT;
}

