/*
 * Name:    d_databox_motor.c
 *
 * Purpose: MX driver for motors attached to the Radix Instruments Databox.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2005-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DATABOX_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_mcs.h"
#include "i_databox.h"
#include "d_databox_motor.h"
#include "d_databox_mcs.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_databox_motor_record_function_list = {
	mxd_databox_motor_initialize_type,
	mxd_databox_motor_create_record_structures,
	mxd_databox_motor_finish_record_initialization,
	NULL,
	mxd_databox_motor_print_structure,
	NULL,
	NULL,
	NULL,
	mxd_databox_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_databox_motor_motor_function_list = {
	mxd_databox_motor_motor_is_busy,
	mxd_databox_motor_move_absolute,
	mxd_databox_motor_get_position,
	mxd_databox_motor_set_position,
	mxd_databox_motor_soft_abort,
	mxd_databox_motor_immediate_abort,
	mxd_databox_motor_positive_limit_hit,
	mxd_databox_motor_negative_limit_hit,
	mxd_databox_motor_find_home_position,
	mxd_databox_motor_constant_velocity_move,
	mxd_databox_motor_get_parameter,
	mxd_databox_motor_set_parameter
};

/* DATABOX motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_databox_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_DATABOX_MOTOR_STANDARD_FIELDS
};

long mxd_databox_motor_num_record_fields
		= sizeof( mxd_databox_motor_record_field_defaults )
			/ sizeof( mxd_databox_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_databox_motor_rfield_def_ptr
			= &mxd_databox_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_databox_motor_get_pointers( MX_MOTOR *motor,
			MX_DATABOX_MOTOR **databox_motor,
			MX_DATABOX **databox,
			const char *calling_fname )
{
	static const char fname[] = "mxd_databox_motor_get_pointers()";

	MX_RECORD *databox_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( databox_motor == (MX_DATABOX_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( databox == (MX_DATABOX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*databox_motor = (MX_DATABOX_MOTOR *)
				motor->record->record_type_struct;

	if ( *databox_motor == (MX_DATABOX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	databox_record = (*databox_motor)->databox_record;

	if ( databox_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The databox_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*databox = (MX_DATABOX *) databox_record->record_type_struct;

	if ( *databox == (MX_DATABOX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_databox_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_DATABOX_MOTOR *databox_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	databox_motor = (MX_DATABOX_MOTOR *) malloc( sizeof(MX_DATABOX_MOTOR) );

	if ( databox_motor == (MX_DATABOX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DATABOX_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = databox_motor;
	record->class_specific_function_list
			= &mxd_databox_motor_motor_function_list;

	motor->record = record;

	/* A Databox motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* The DATABOX reports accelerations in counts/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_databox_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_DATABOX *databox;
	MX_RECORD *databox_record;
	MX_DATABOX_MOTOR *databox_motor;
	int i;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_databox_motor_get_pointers( motor,
					&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if the record pointed to by
	 * databox_motor->databox_record is actually a "databox" interface
	 * record.
	 */

	databox_record = databox_motor->databox_record;

	if ( ( databox_record->mx_superclass != MXR_INTERFACE )
	  || ( databox_record->mx_class != MXI_CONTROLLER )
	  || ( databox_record->mx_type != MXI_CTRL_DATABOX ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The field 'databox_record' in record '%s' does not point to "
	"a 'databox' interface record.  Instead, it points to record '%s'",
			record->name, databox_record->name );
	}

	/* Store a pointer to this record in the MX_DATABOX structure. */

	if ( databox->motor_record_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The motor_record_array pointer for the DATABOX interface '%s' is NULL.",
			databox->record->name );
	}

	if ( islower( (int)(databox_motor->axis_name) ) ) {
		databox_motor->axis_name
			= toupper( (int)(databox_motor->axis_name) );
	}

	switch( databox_motor->axis_name ) {
	case 'X': i = 0; break;
	case 'Y': i = 1; break;
	case 'Z': i = 2; break;
	default: return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The axis name '%c' for Databox motor '%s' is illegal.  "
		"The only allowed names are 'X', 'Y', and 'Z'.",
			databox_motor->axis_name,
			record->name );
	}

	if ( databox->motor_record_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The motor_record_array pointer for DATABOX interface '%s' is NULL.",
			databox->record->name );
	}

	(databox->motor_record_array)[i] = record;

	if ( databox_motor->axis_name == 'X' ) {
		databox->degrees_per_x_step = mx_divide_safely( 1.0,
					databox_motor->steps_per_degree );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_databox_motor_print_structure()";

	MX_MOTOR *motor;
	MX_DATABOX_MOTOR *databox_motor;
	MX_DATABOX *databox;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_databox_motor_get_pointers( motor,
					&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = DATABOX_MOTOR\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  controller        = %s\n",
					databox->record->name);
	fprintf(file, "  axis name         = %c\n", databox_motor->axis_name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}
	
	fprintf(file, "  position          = %g %s  (%g deg)\n",
			motor->position, motor->units,
			motor->raw_position.analog );
	fprintf(file, "  scale             = %g %s per deg.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%g deg)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	
	fprintf(file, "  negative limit    = %g %s  (%g deg)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit    = %g %s  (%g deg)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband     = %g %s  (%g deg)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	fprintf(file, "  steps per degree  = %g\n",
		databox_motor->steps_per_degree );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_databox_motor_resynchronize()";

	MX_DATABOX_MOTOR *databox_motor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	databox_motor = (MX_DATABOX_MOTOR *) record->record_type_struct;

	if ( databox_motor == (MX_DATABOX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DATABOX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	/* Resynchronize the DATABOX interface. */

	mx_status = mxi_databox_resynchronize( databox_motor->databox_record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_databox_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_motor_is_busy()";

	MX_DATABOX_MOTOR *databox_motor;
	MX_DATABOX *databox;
	unsigned long num_input_bytes_available;
	mx_status_type mx_status;

	mx_status = mxd_databox_motor_get_pointers( motor,
					&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: databox->last_start_action = %d",
			fname, databox->last_start_action));
	MX_DEBUG( 2,("%s: databox->moving_motor = '%c'",
			fname, databox->moving_motor));

	motor->busy = TRUE;

	/* Check to see if a move of any axis is supposed to be in progress.
	 * If not, we return motor->busy set to FALSE.
	 */

	if ( databox->moving_motor != databox_motor->axis_name ) {
		motor->busy = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we are waiting for the CR-LF sequence that
	 * appears after the 'Stepping...' message from the controller.
	 */

	mx_status = mx_rs232_num_input_bytes_available( databox->rs232_record,
						&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If no input is available, the move is still in progress. */

	if ( num_input_bytes_available == 0 ) {
		motor->busy = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, the move should be complete.  Readout and
	 * discard the returned characters from the controller.
	 */

	motor->busy = FALSE;
	databox->moving_motor = '\0';
	databox->last_start_action = MX_DATABOX_NO_ACTION;

	MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

	mx_status = mxi_databox_discard_unread_input( databox,
							DATABOX_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_move_absolute()";

	MX_DATABOX_MOTOR *databox_motor;
	MX_DATABOX *databox;
	MX_RECORD *other_motor_record;
	const char stepping[] = "Stepping...";
	char command[40];
	char c;
	int i, message_length;
	double destination;
	mx_status_type mx_status;

	mx_status = mxd_databox_motor_get_pointers( motor,
					&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: databox->last_start_action = %d",
			fname, databox->last_start_action));
	MX_DEBUG( 2,("%s: databox->moving_motor = '%c'",
			fname, databox->moving_motor));

	/* If this is axis 'X' and a Databox MCS scan is scheduled to start,
	 * start the MCS scan.
	 */

	if ( databox->last_start_action == MX_DATABOX_MCS_START_ACTION ) {

		if ( databox_motor->axis_name == 'X' ) {

			mx_status = mxd_databox_mcs_start_sequence(
					databox->mcs_record,
					motor->raw_destination.analog,
					TRUE );

			databox->last_start_action = MX_DATABOX_MCS_MOVE_ACTION;

			MX_DEBUG( 2,
			  ("%s: assigned %d to databox->last_start_action",
				fname, databox->last_start_action));

			return mx_status;
		} else {
			databox->last_start_action = MX_DATABOX_NO_ACTION;

			MX_DEBUG( 2,
			  ("%s: assigned %d to databox->last_start_action",
				fname, databox->last_start_action));

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Attempted to move Databox motor '%s' during a Databox MCS scan.  "
	"Only the X axis can move during a Databox MCS scan.",
				motor->record->name );
		}
	}

	/* If a motor axis is already moving, we cannot start a new move. */

	if ( databox->moving_motor != '\0' ) {
		mx_status = mxi_databox_get_record_from_motor_name(
				databox, databox->moving_motor,
				&other_motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_TRY_AGAIN, fname,
	"Cannot move motor '%s' at this time since motor '%s' of Databox '%s' "
	"is already in motion.  Try again after the move completes.",
			motor->record->name,
			other_motor_record->name,
			databox->record->name );
	}

	if ( databox->command_mode != MX_DATABOX_CALIBRATE_MODE ) {
		mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_CALIBRATE_MODE, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	destination = motor->raw_destination.analog;

	sprintf( command, "G%c%g\r", databox_motor->axis_name, destination );

	mx_status = mxi_databox_command( databox, command,
					NULL, 0, DATABOX_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the move successfully started, we should have been sent
	 * the string 'Stepping...' by the Databox.  Read and discard
	 * characters until we see the 'S' character.
	 */

	for (;;) {
		mx_status = mxi_databox_getchar( databox,
						&c, DATABOX_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c == stepping[0] )
			break;

		mx_msleep(10);
	}

	/* Now check to see if we got the rest of the message. */

	message_length = (int) strlen( stepping );

	for ( i = 1; i < message_length; i++ ) {
		mx_status = mxi_databox_getchar( databox,
						&c, DATABOX_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c != stepping[i] ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Character '%c' sent by the Databox '%s' does not match the character '%c' "
"in the message '%s' that is expected to be returned after a move starts.",
				c, databox->record->name,
				stepping[i], stepping );
		}

		mx_msleep(10);
	}

	databox->moving_motor = databox_motor->axis_name;

	databox->last_start_action = MX_DATABOX_FREE_MOVE_ACTION;

	MX_DEBUG( 2,("%s: assigned %d to databox->last_start_action",
			fname, databox->last_start_action));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_get_position()";

	MX_DATABOX_MOTOR *databox_motor;
	MX_DATABOX *databox;
	char command[80];
	char response[80];
	double position;
	int num_tokens;
	mx_status_type mx_status;

	mx_status = mxd_databox_motor_get_pointers( motor,
					&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If a move is currently in progress for one of the motors attached
	 * to the Databox, then it is not possible to ask for a motor
	 * position at this time, so just return the old value.
	 */

	if ( databox->moving_motor != '\0' )
		return MX_SUCCESSFUL_RESULT;

	if ( databox->command_mode != MX_DATABOX_CALIBRATE_MODE ) {
		mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_CALIBRATE_MODE, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	sprintf( command, "A%c\r", databox_motor->axis_name );

	mx_status = mxi_databox_command( databox, command,
			response, sizeof response, DATABOX_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_tokens = sscanf( response, "%*s %*s %lg", &position );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to '%s' command.  "
		"Response seen = '%s'", command, response );
	}

	motor->raw_position.analog = position;

	mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_set_position()";

	MX_DATABOX_MOTOR *databox_motor;
	MX_DATABOX *databox;
	MX_RECORD *other_motor_record;
	double position;
	char command[100];
	char response[100];
	mx_status_type mx_status;

	mx_status = mxd_databox_motor_get_pointers( motor,
					&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( databox->moving_motor != '\0' ) {
		mx_status = mxi_databox_get_record_from_motor_name(
				databox, databox->moving_motor,
				&other_motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_TRY_AGAIN, fname,
	"Cannot change the position setpoint for motor '%s' at this time "
	"since motor '%s' of Databox '%s' is currently in motion.  "
	"Try again after the move completes.",
			motor->record->name,
			other_motor_record->name,
			databox->record->name );
	}

	if ( databox->command_mode != MX_DATABOX_CALIBRATE_MODE ) {
		mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_CALIBRATE_MODE, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	position = motor->raw_set_position.analog;

	sprintf( command, "A%c%g\r", databox_motor->axis_name, position );

	mx_status = mxi_databox_command( databox, command,
					response, sizeof response,
					DATABOX_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_databox_set_command_mode( databox,
					MX_DATABOX_MONITOR_MODE, TRUE );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_motor_soft_abort( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_immediate_abort()";

	MX_DATABOX_MOTOR *databox_motor;
	MX_DATABOX *databox;
	mx_status_type mx_status;

	mx_status = mxd_databox_motor_get_pointers( motor,
				&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a ctrl-C character to stop the motion.  Unfortunately, this
	 * causes the motor to lose its angle calibration.
	 */

	mx_status = mxi_databox_putchar( databox, '\003', DATABOX_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	databox->moving_motor = '\0';

	/* Put us back into Monitor mode by calling the
	 * resynchronize function.
	 */

	mx_status = mxd_databox_motor_resynchronize( motor->record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_motor_positive_limit_hit( MX_MOTOR *motor )
{
	motor->positive_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_negative_limit_hit( MX_MOTOR *motor )
{
	motor->negative_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_find_home_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Home searches are not supported for motor '%s'",
		motor->record->name );
}

MX_EXPORT mx_status_type
mxd_databox_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_constant_velocity_move()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Constant velocity moves are not supported for motor '%s'",
		motor->record->name );
}

/* The following are just stubs to allow Databox quick scans to work. */

MX_EXPORT mx_status_type
mxd_databox_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_get_parameter()";

	MX_DATABOX_MOTOR *databox_motor;
	MX_DATABOX *databox;
	mx_status_type mx_status;

	mx_status = mxd_databox_motor_get_pointers( motor,
				&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		motor->raw_speed = 1.0;
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		motor->acceleration_time = 0.0;
		break;

	case MXLV_MTR_ACCELERATION_DISTANCE:
		motor->acceleration_distance = 0.0;
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_databox_motor_set_parameter()";

	MX_DATABOX_MOTOR *databox_motor;
	MX_DATABOX *databox;
	mx_status_type mx_status;

	mx_status = mxd_databox_motor_get_pointers( motor,
				&databox_motor, &databox, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* Ignore any speed changes. */

		break;
	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

