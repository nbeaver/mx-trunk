/*
 * Name:    d_theta_2theta.c 
 *
 * Purpose: MX motor driver to control a theta-2 theta pair of motors as a
 *          single pseudomotor with the 2 theta motor moving at twice the
 *          speed of the theta motor.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "d_theta_2theta.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_theta_2theta_motor_record_function_list = {
	mxd_theta_2theta_motor_initialize_type,
	mxd_theta_2theta_motor_create_record_structures,
	mxd_theta_2theta_motor_finish_record_initialization,
	mxd_theta_2theta_motor_delete_record,
	mxd_theta_2theta_motor_print_motor_structure,
	mxd_theta_2theta_motor_read_parms_from_hardware,
	mxd_theta_2theta_motor_write_parms_to_hardware,
	mxd_theta_2theta_motor_open,
	mxd_theta_2theta_motor_close
};

MX_MOTOR_FUNCTION_LIST mxd_theta_2theta_motor_motor_function_list = {
	mxd_theta_2theta_motor_motor_is_busy,
	mxd_theta_2theta_motor_move_absolute,
	mxd_theta_2theta_motor_get_position,
	mxd_theta_2theta_motor_set_position,
	mxd_theta_2theta_motor_soft_abort,
	mxd_theta_2theta_motor_immediate_abort,
	mxd_theta_2theta_motor_positive_limit_hit,
	mxd_theta_2theta_motor_negative_limit_hit,
	mxd_theta_2theta_motor_find_home_position,
	mxd_theta_2theta_motor_constant_velocity_move,
	mxd_theta_2theta_motor_get_parameter,
	mxd_theta_2theta_motor_set_parameter
};

/* Theta-2 theta motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_theta_2theta_rfield_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_THETA_2THETA_MOTOR_STANDARD_FIELDS
};

long mxd_theta_2theta_motor_num_record_fields
		= sizeof( mxd_theta_2theta_rfield_defaults )
			/ sizeof( mxd_theta_2theta_rfield_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_theta_2theta_motor_rfield_def_ptr
			= &mxd_theta_2theta_rfield_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_theta_2theta_motor_get_pointers( MX_MOTOR *motor,
			MX_THETA_2THETA_MOTOR **theta_2theta_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_theta_2theta_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( theta_2theta_motor == (MX_THETA_2THETA_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THETA_2THETA_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*theta_2theta_motor = (MX_THETA_2THETA_MOTOR *)
				(motor->record->record_type_struct);

	if ( *theta_2theta_motor == (MX_THETA_2THETA_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_THETA_2THETA_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_initialize_type( long type )
{
		return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_theta_2theta_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_THETA_2THETA_MOTOR *theta_2theta_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	theta_2theta_motor = (MX_THETA_2THETA_MOTOR *)
					malloc( sizeof(MX_THETA_2THETA_MOTOR) );

	if ( theta_2theta_motor == (MX_THETA_2THETA_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_THETA_2THETA_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = theta_2theta_motor;
	record->class_specific_function_list
				= &mxd_theta_2theta_motor_motor_function_list;

	motor->record = record;
	theta_2theta_motor->motor = motor;

	/* A theta-2 theta motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_finish_record_initialization( MX_RECORD *record )
{
	MX_MOTOR *motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_delete_record( MX_RECORD *record )
{
	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_theta_2theta_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	theta_2theta_motor = (MX_THETA_2THETA_MOTOR *)
					(record->record_type_struct);

	if ( theta_2theta_motor == (MX_THETA_2THETA_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_THETA_2THETA_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type      = THETA_2THETA_MOTOR.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  theta motor     = %s\n",
			theta_2theta_motor->theta_motor_record->name);
	fprintf(file, "  2 theta motor   = %s\n",
			theta_2theta_motor->two_theta_motor_record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position        = %g %s\n",
		position, motor->units);
	fprintf(file, "  scale           = %g %s per unscaled unit.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset          = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file, "  backlash        = %g %s.\n",
		motor->raw_backlash_correction.analog, motor->units);
	fprintf(file, "  negative limit  = %g %s.\n",
		motor->raw_negative_limit.analog, motor->units);
	fprintf(file, "  positive limit  = %g %s.\n",
		motor->raw_positive_limit.analog, motor->units);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband   = %g %s\n\n",
		move_deadband, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_motor_is_busy()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->busy = FALSE;

	mx_status = mx_motor_is_busy( theta_2theta_motor->theta_motor_record,
					&busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		motor->busy = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_motor_is_busy( theta_2theta_motor->two_theta_motor_record,
					&busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		motor->busy = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_move_absolute()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	double theta_speed, two_theta_speed;
	double theta_destination, two_theta_destination;
	mx_status_type mx_status;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the speed of the theta motor and set the speed of
	 * the 2theta motor based on that.
	 */

	mx_status = mx_motor_get_speed( theta_2theta_motor->theta_motor_record,
					&theta_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	two_theta_speed = 2.0 * theta_speed;

	mx_status = mx_motor_set_speed( theta_2theta_motor->two_theta_motor_record,
					two_theta_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the requested destination for the 2theta motor. */

	theta_destination = motor->raw_destination.analog;

	two_theta_destination = 2.0 * theta_destination;

	/* Now start the commanded move for the two motors. */

	mx_status = mx_motor_move_absolute( theta_2theta_motor->theta_motor_record,
						theta_destination,
						MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute(
				theta_2theta_motor->two_theta_motor_record,
						two_theta_destination,
						MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS ) {
		/* Abort the theta move which is already in progress. */

		(void) mx_motor_soft_abort(
				theta_2theta_motor->theta_motor_record );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_get_position()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	double theta_motor_position;
	mx_status_type mx_status;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The reported position depends only on the theta position. */

	mx_status = mx_motor_get_position( theta_2theta_motor->theta_motor_record,
						&theta_motor_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = theta_motor_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_set_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"'set position' is not valid for a theta-2 theta motor." );
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_soft_abort()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	mx_status_type mx_status, mx_status2;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( 
			theta_2theta_motor->theta_motor_record );

	/* Perform the soft abort for 2theta even if the abort for theta
	 * failed.
	 */

	mx_status2 = mx_motor_soft_abort(
			theta_2theta_motor->two_theta_motor_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status2;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_immediate_abort()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	mx_status_type mx_status, mx_status2;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort( 
			theta_2theta_motor->theta_motor_record );

	/* Perform the immediate abort for 2theta even if the abort for theta
	 * failed.
	 */

	mx_status2 = mx_motor_immediate_abort(
			theta_2theta_motor->two_theta_motor_record );

	if ( mx_status2.code != MXE_SUCCESS )
		return mx_status2;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_positive_limit_hit()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->positive_limit_hit = FALSE;

	mx_status = mx_motor_positive_limit_hit(
				theta_2theta_motor->theta_motor_record,
				&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_motor_positive_limit_hit(
				theta_2theta_motor->two_theta_motor_record,
				&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_negative_limit_hit()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->negative_limit_hit = FALSE;

	mx_status = mx_motor_negative_limit_hit(
				theta_2theta_motor->theta_motor_record,
				&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_motor_negative_limit_hit(
				theta_2theta_motor->two_theta_motor_record,
				&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_find_home_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
"A home search for a theta-2 theta motor is not allowed.  Motor name = '%s'",
		motor->record->name );
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_constant_velocity_move()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	double theta_speed, two_theta_speed;
	mx_status_type mx_status;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the speed of the theta motor and set the speed of
	 * the 2theta motor based on that.
	 */

	mx_status = mx_motor_get_speed( theta_2theta_motor->theta_motor_record,
					&theta_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	two_theta_speed = 2.0 * theta_speed;

	mx_status = mx_motor_set_speed( theta_2theta_motor->two_theta_motor_record,
					two_theta_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now start the commanded move for the two motors. */

	mx_status = mx_motor_constant_velocity_move(
			theta_2theta_motor->theta_motor_record,
			motor->constant_velocity_move );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_constant_velocity_move(
			theta_2theta_motor->two_theta_motor_record,
			motor->constant_velocity_move );

	if ( mx_status.code != MXE_SUCCESS ) {
		/* Abort the theta move which is already in progress. */

		(void) mx_motor_soft_abort(
				theta_2theta_motor->theta_motor_record );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_get_parameter()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	double theta_speed;
	mx_status_type mx_status;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		/* Report back the speed for theta. */

		mx_status = mx_motor_get_speed(
				theta_2theta_motor->theta_motor_record,
				&theta_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_speed = theta_speed;

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			motor->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_theta_2theta_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_theta_2theta_motor_set_parameter()";

	MX_THETA_2THETA_MOTOR *theta_2theta_motor;
	double theta_speed, two_theta_speed;
	mx_status_type mx_status;

	mx_status = mxd_theta_2theta_motor_get_pointers( motor,
						&theta_2theta_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		theta_speed = motor->raw_speed;

		two_theta_speed = 2.0 * theta_speed;

		mx_status = mx_motor_set_speed(
				theta_2theta_motor->theta_motor_record,
				theta_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_set_speed(
				theta_2theta_motor->two_theta_motor_record,
				two_theta_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			motor->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

