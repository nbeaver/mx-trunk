/*
 * Name:    d_tangent_arm.c 
 *
 * Purpose: MX motor driver to move a tangent arm or sine arm.
 *
 * Author:  William Lavender
 *
 * IMPORTANT: The moving motor position and the arm length must both be
 *    specified using the _same_ user units, while the angle offset must
 *    be specified in radians.  Thus, if the moving motor position is
 *    specified in micrometers, then the arm length must be specified
 *    in micrometers as well.
 *
 *    If you want to specify the angle offset in degrees rather than in
 *    radians, the simplest way is to create a 'translation_mtr' record
 *    containing only the raw angle offset motor.  Then, set a scale factor
 *    in the translation motor record that converts from radians to degrees.
 *
 * A tangent arm consists of two arms that are connected at a pivot point
 * as follows:
 *
 *                                        *
 *                                  B  *
 *                                  *
 *                               *  |
 *                            *     |
 *                         *        |
 *                      *           |  m
 *                   *              |
 *                *                 |
 *             *  \  theta          |
 *          *-----------------------+------
 *          ^           a           A
 *          |
 *          |
 *     pivot point
 *
 * A linear motor is attached to the fixed arm at point A and then moves
 * a push rod that pushes the moving arm at point B.  In this geometry,
 * the position of the moving motor, m, is related to the angle theta by
 * the relationship
 *
 *   tan( theta ) = m / a
 *
 * where a is the distance of the motor on the arm it is attached to
 * from the pivot point.  The important consideration here is that the
 * linear motion is perpendicular to the fixed arm.
 *
 * A sine arm is similar except that the linear motion is now perpendicular
 * to the moving arm:
 *
 *                                        *
 *                                     *
 *                                  *
 *                            B  *
 *                            *
 *                         *   \
 *                      *       \
 *                   *           \ m
 *                *               \
 *             *  \  theta         \
 *          *-----------------------+------
 *          ^           a           A
 *          |
 *          |
 *     pivot point
 *
 * This changes the equation to the form
 *
 *   sin( theta ) = m / a
 *
 * hence the name "sine arm".
 *
 * Note: The 'tangent_arm' and 'sine_arm' drivers share the same code and
 *       distinguished in the driver code by their different driver types,
 *       namely, MXT_MTR_TANGENT_ARM and MXT_MTR_SINE_ARM.
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2006 Illinois Institute of Technology
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
#include "mx_variable.h"
#include "d_tangent_arm.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_tangent_arm_record_function_list = {
	mxd_tangent_arm_initialize_type,
	mxd_tangent_arm_create_record_structures,
	mxd_tangent_arm_finish_record_initialization,
	mxd_tangent_arm_delete_record,
	mxd_tangent_arm_print_motor_structure,
	mxd_tangent_arm_read_parms_from_hardware,
	mxd_tangent_arm_write_parms_to_hardware,
	mxd_tangent_arm_open,
	mxd_tangent_arm_close
};

MX_MOTOR_FUNCTION_LIST mxd_tangent_arm_motor_function_list = {
	mxd_tangent_arm_motor_is_busy,
	mxd_tangent_arm_move_absolute,
	mxd_tangent_arm_get_position,
	mxd_tangent_arm_set_position,
	mxd_tangent_arm_soft_abort,
	mxd_tangent_arm_immediate_abort,
	mxd_tangent_arm_positive_limit_hit,
	mxd_tangent_arm_negative_limit_hit,
	mxd_tangent_arm_find_home_position,
	NULL,
	mxd_tangent_arm_get_parameter,
	mxd_tangent_arm_set_parameter
};

/* Tangent arm data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_tangent_arm_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_TANGENT_ARM_STANDARD_FIELDS
};

long mxd_tangent_arm_num_record_fields
		= sizeof( mxd_tangent_arm_record_field_defaults )
			/ sizeof( mxd_tangent_arm_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_tangent_arm_rfield_def_ptr
			= &mxd_tangent_arm_record_field_defaults[0];

static mx_status_type
mxd_tangent_arm_convert_angle_to_moving_motor_position( MX_MOTOR *motor,
					MX_TANGENT_ARM *tangent_arm,
					double angle,
					double *moving_motor_position );
static mx_status_type
mxd_tangent_arm_convert_moving_motor_position_to_angle( MX_MOTOR *motor,
					MX_TANGENT_ARM *tangent_arm,
					double moving_motor_position,
					double *angle );

static mx_status_type
mxd_tangent_arm_get_angle_offset_value( MX_TANGENT_ARM *tangent_arm,
					double *angle_offset_value );

static mx_status_type
mxd_tangent_arm_set_angle_offset_value( MX_TANGENT_ARM *tangent_arm,
					double angle_offset_value );

/* === */

/* A private function for the use of the driver. */

static mx_status_type
mxd_tangent_arm_get_pointers( MX_MOTOR *motor,
			MX_TANGENT_ARM **tangent_arm,
			MX_RECORD **moving_motor_record,
			const char *calling_fname )
{
	MX_TANGENT_ARM *tangent_arm_ptr;

	static const char fname[] = "mxd_tangent_arm_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	tangent_arm_ptr = (MX_TANGENT_ARM *) motor->record->record_type_struct;

	if ( tangent_arm_ptr == (MX_TANGENT_ARM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TANGENT_ARM pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( tangent_arm != (MX_TANGENT_ARM **) NULL ) {
		*tangent_arm = tangent_arm_ptr;
	}

	if ( moving_motor_record != (MX_RECORD **) NULL ) {
		*moving_motor_record = (*tangent_arm)->moving_motor_record;

		if ( *moving_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Moving motor record pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_tangent_arm_initialize_type( long type )
{
		return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_tangent_arm_create_record_structures()";

	MX_MOTOR *motor;
	MX_TANGENT_ARM *tangent_arm;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	tangent_arm = (MX_TANGENT_ARM *) malloc( sizeof(MX_TANGENT_ARM) );

	if ( tangent_arm == (MX_TANGENT_ARM *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TANGENT_ARM structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = tangent_arm;
	record->class_specific_function_list
				= &mxd_tangent_arm_motor_function_list;

	motor->record = record;
	tangent_arm->motor = motor;

	/* A tangent arm is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_finish_record_initialization( MX_RECORD *record )
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
mxd_tangent_arm_delete_record( MX_RECORD *record )
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
mxd_tangent_arm_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_tangent_arm_print_motor_structure()";

	MX_MOTOR *motor;
	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	MX_MOTOR *moving_motor;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	moving_motor = (MX_MOTOR *) moving_motor_record->record_class_struct;

	if ( moving_motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for moving_motor_record '%s' "
			"used by record '%s' is NULL.",
			moving_motor_record->name, record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	if ( record->mx_type == MXT_MTR_TANGENT_ARM ) {
		fprintf(file,
		      "  Motor type      = TANGENT_ARM.\n\n");
	} else {
		fprintf(file,
		      "  Motor type      = SINE_ARM.\n\n");
	}

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  moving motor    = %s\n", moving_motor_record->name);
	fprintf(file, "  angle offset    = %s\n",
				tangent_arm->angle_offset_record->name);
	fprintf(file, "  arm length      = %g %s\n", tangent_arm->arm_length,
							moving_motor->units );

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position        = %g %s\n",
		position, motor->units);
	fprintf(file, "  scale           = %g %s per radian.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset          = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file, "  backlash        = %g %s.\n",
		motor->backlash_correction, motor->units);
	fprintf(file, "  negative limit  = %g %s.\n",
		motor->negative_limit, motor->units);
	fprintf(file, "  positive limit  = %g %s.\n",
		motor->positive_limit, motor->units);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband   = %g %s\n\n",
		move_deadband, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_motor_is_busy()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_is_busy( moving_motor_record, &busy );

	motor->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_move_absolute()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	double destination_angle, moving_motor_destination;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check the limits on this request. */

	destination_angle = motor->raw_destination.analog;

	if ( (destination_angle > motor->raw_positive_limit.analog)
	  || (destination_angle < motor->raw_negative_limit.analog) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested position %g is outside the allowed range of %g to %g",
			destination_angle, motor->raw_negative_limit.analog,
			motor->raw_positive_limit.analog );
	}

	/* Compute the destination for the moving motor. */

	mx_status = mxd_tangent_arm_convert_angle_to_moving_motor_position(
			motor, tangent_arm, destination_angle,
			&moving_motor_destination );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now perform the move. */

	mx_status = mx_motor_move_absolute( moving_motor_record,
				moving_motor_destination, MXF_MTR_NOWAIT );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_get_position()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	double moving_motor_position, angle;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current moving motor position. */

	mx_status = mx_motor_get_position( moving_motor_record,
						&moving_motor_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_tangent_arm_convert_moving_motor_position_to_angle(
			motor, tangent_arm, moving_motor_position, &angle );

	motor->raw_position.analog = angle;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_set_position()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	double new_angle, old_angle, delta;
	double angle_offset, new_angle_offset;
	double moving_motor_angle, moving_motor_ratio;
	double new_moving_motor_position;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check the limits on this request. */

	new_angle = motor->raw_set_position.analog;

	if ( (new_angle > motor->raw_positive_limit.analog)
	  || (new_angle < motor->raw_negative_limit.analog) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested position %g is outside the allowed range of %g to %g",
			new_angle, motor->raw_negative_limit.analog,
			motor->raw_positive_limit.analog );
	}

	/* Get the current value of the angle offset. */

	mx_status = mxd_tangent_arm_get_angle_offset_value(
				tangent_arm, &angle_offset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do we need to change the moving motor position
	 * or the angle offset? 
	 */

	if ( tangent_arm->tangent_arm_flags & MXF_TAN_CHANGE_ANGLE_OFFSET ) {

		/* We are going to change the angle offset. */
		
		/* The first step is to get the old position of the pseudmotor
		 * so that we can compute the difference between the old
		 * and the new position.
		 */

		mx_status = mx_motor_get_position( motor->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		old_angle = motor->raw_position.analog;

		/* Compute the angle change required to redefine
		 * the pseudomotor position.
		 */

		delta = new_angle - old_angle;

		/* Update the angle offset with the new value. */

		new_angle_offset = angle_offset + delta;

		mx_status = mxd_tangent_arm_set_angle_offset_value(
					tangent_arm, new_angle_offset );
	} else {

		/* We are going to change the moving motor position. */

		/* First, compute the angle corresponding to the 
		 * moving motor position.
		 */

		moving_motor_angle = new_angle - angle_offset;

		/* Convert the angle to a ratio of the linear translation to
		 * the arm length.
		 */

		if ( motor->record->mx_type == MXT_MTR_TANGENT_ARM ) {
			moving_motor_ratio = tan( moving_motor_angle );
		} else {
			moving_motor_ratio = sin( moving_motor_angle );
		}

		/* Compute the new moving motor position. */

		new_moving_motor_position =
			moving_motor_ratio * tangent_arm->arm_length;

		/* Now perform the move. */

		mx_status = mx_motor_set_position( moving_motor_record,
						new_moving_motor_position );
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_soft_abort()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( moving_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_immediate_abort()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort( moving_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_positive_limit_hit()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->scale >= 0.0 ) {
		mx_status = mx_motor_positive_limit_hit(
				moving_motor_record, &limit_hit);
	} else {
		mx_status = mx_motor_negative_limit_hit(
				moving_motor_record, &limit_hit);
	}

	motor->positive_limit_hit = limit_hit;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_negative_limit_hit()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->scale >= 0.0 ) {
		mx_status = mx_motor_negative_limit_hit(
				moving_motor_record, &limit_hit);
	} else {
		mx_status = mx_motor_positive_limit_hit(
				moving_motor_record, &limit_hit);
	}

	motor->negative_limit_hit = limit_hit;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_find_home_position()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	long direction;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->scale < 0.0 ) {
		direction = - ( motor->home_search );
	} else {
		direction = motor->home_search;
	}

	mx_status = mx_motor_find_home_position( moving_motor_record,
							direction );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_constant_velocity_move()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	long direction;
	mx_status_type mx_status;

	mx_status = mxd_tangent_arm_get_pointers( motor,
				&tangent_arm, &moving_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->scale < 0.0 ) {
		direction = - ( motor->constant_velocity_move );
	} else {
		direction = motor->constant_velocity_move;
	}

	mx_status = mx_motor_constant_velocity_move( moving_motor_record,
							direction );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_get_parameter()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	double acceleration_time;
	double moving_motor_position, angle;
	double angle_start, angle_end;
	double moving_motor_start, moving_motor_end;
	double real_angle_start, real_angle_end;
	double real_moving_motor_start, real_moving_motor_end;
	double moving_motor_speed, angular_speed;
	double scaled_speed, scaled_position, cos_scaled_position;
	mx_status_type status;

	status = mxd_tangent_arm_get_pointers( motor, &tangent_arm,
					&moving_motor_record, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_ACCELERATION_TYPE:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
	case MXLV_MTR_ACCELERATION_DISTANCE:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Pseudomotor '%s' cannot report a value for parameter '%s' (%ld).",
			motor->record->name,
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type );

	case MXLV_MTR_SPEED:
		/* Compute the instantaneous angular speed from the
		 * speed of the moving motor at its current position.
		 */

		status = mx_motor_get_speed( moving_motor_record,
						&moving_motor_speed );

		if ( status.code != MXE_SUCCESS )
			return status;

		scaled_speed = mx_divide_safely( moving_motor_speed,
						tangent_arm->arm_length );

		status = mx_motor_get_position( moving_motor_record,
						&moving_motor_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		scaled_position = mx_divide_safely( moving_motor_position,
						tangent_arm->arm_length );

		cos_scaled_position = cos( scaled_position );

		if ( motor->record->mx_type == MXT_MTR_TANGENT_ARM ) {
			angular_speed =
				mx_divide_safely( scaled_speed,
				cos_scaled_position * cos_scaled_position );
		} else {
			angular_speed = scaled_speed * cos_scaled_position;
		}

		motor->raw_speed = angular_speed;
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		status = mx_motor_get_acceleration_time( moving_motor_record,
							&acceleration_time );
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		angle_start = motor->raw_compute_extended_scan_range[0];
		angle_end = motor->raw_compute_extended_scan_range[1];

		status =
		    mxd_tangent_arm_convert_angle_to_moving_motor_position(
				motor, tangent_arm,
				angle_start, &moving_motor_start );

		if ( status.code != MXE_SUCCESS )
			return status;

		status =
		    mxd_tangent_arm_convert_angle_to_moving_motor_position(
				motor, tangent_arm,
				angle_end, &moving_motor_end );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_compute_extended_scan_range(
						moving_motor_record,
						moving_motor_start,
						moving_motor_end,
						&real_moving_motor_start,
						&real_moving_motor_end );

		if ( status.code != MXE_SUCCESS )
			return status;

		status =
		    mxd_tangent_arm_convert_moving_motor_position_to_angle(
				motor, tangent_arm,
				real_moving_motor_start, &real_angle_start );

		if ( status.code != MXE_SUCCESS )
			return status;

		status =
		    mxd_tangent_arm_convert_moving_motor_position_to_angle(
				motor, tangent_arm,
				real_moving_motor_end, &real_angle_end );

		if ( status.code != MXE_SUCCESS )
			return status;

		motor->raw_compute_extended_scan_range[2] = real_angle_start;
		motor->raw_compute_extended_scan_range[3] = real_angle_end;
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		moving_motor_position = motor->compute_pseudomotor_position[0];

		status =
		    mxd_tangent_arm_convert_moving_motor_position_to_angle(
			motor, tangent_arm, moving_motor_position, &angle );

		motor->compute_pseudomotor_position[1] = angle;
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		angle = motor->compute_real_position[0];

		status =
		    mxd_tangent_arm_convert_angle_to_moving_motor_position(
			motor, tangent_arm, angle, &moving_motor_position );

		motor->compute_real_position[1] = moving_motor_position;
		break;

	default:
		status = mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return status;
}

MX_EXPORT mx_status_type
mxd_tangent_arm_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_tangent_arm_set_parameter()";

	MX_TANGENT_ARM *tangent_arm;
	MX_RECORD *moving_motor_record;
	double real_position1, real_position2, time_for_move;
	mx_status_type status;

	status = mxd_tangent_arm_get_pointers( motor, &tangent_arm,
					&moving_motor_record, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Pseudomotor '%s' cannot set the value of parameter '%s' (%ld).",
			motor->record->name,
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type );

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
			motor->record, motor->raw_speed_choice_parameters[0],
			&real_position1, FALSE );

		if ( status.code != MXE_SUCCESS )
			return status;

		status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
			motor->record, motor->raw_speed_choice_parameters[1],
			&real_position2, FALSE );

		if ( status.code != MXE_SUCCESS )
			return status;

		time_for_move = motor->raw_speed_choice_parameters[2];

		status = mx_motor_set_speed_between_positions(
					moving_motor_record,
					real_position1, real_position2,
					time_for_move );
		break;

	case MXLV_MTR_SAVE_SPEED:
		status = mx_motor_save_speed( moving_motor_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		status = mx_motor_restore_speed( moving_motor_record );
		break;

	default:
		status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return status;
}

/*======== Private functions for the use of the driver only. ========*/

static mx_status_type
mxd_tangent_arm_convert_angle_to_moving_motor_position( MX_MOTOR *motor,
					MX_TANGENT_ARM *tangent_arm,
					double angle,
					double *moving_motor_position )
{
	static const char fname[] =
		"mxd_tangent_arm_convert_angle_to_moving_motor_position()";

	double moving_motor_angle, moving_motor_ratio, angle_offset;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s: angle = %g", fname, angle));
	MX_DEBUG( 2,("%s: tangent_arm->arm_length = %g",
					fname, tangent_arm->arm_length));

	/* Get the angle offset value. */

	mx_status = mxd_tangent_arm_get_angle_offset_value(
				tangent_arm, &angle_offset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: angle_offset = %g", fname, angle_offset));

	/* Compute the angle corresponding to the moving motor position. */

	moving_motor_angle = angle - angle_offset;

	MX_DEBUG( 2,("%s: moving_motor_angle = %g",
			fname, moving_motor_angle));

	/* Convert the angle to a ratio of the linear translation to
	 * the arm length.
	 */

	if ( motor->record->mx_type == MXT_MTR_TANGENT_ARM ) {
		moving_motor_ratio = tan( moving_motor_angle );
	} else {
		moving_motor_ratio = sin( moving_motor_angle );
	}

	MX_DEBUG( 2,("%s: moving_motor_ratio = %g", fname, moving_motor_ratio));

	/* Compute the final moving motor destination. */

	*moving_motor_position = moving_motor_ratio * tangent_arm->arm_length;

	MX_DEBUG( 2,("%s: *moving_motor_position = %g",
					fname, *moving_motor_position));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_tangent_arm_convert_moving_motor_position_to_angle( MX_MOTOR *motor,
					MX_TANGENT_ARM *tangent_arm,
					double moving_motor_position,
					double *angle )
{
	static const char fname[] = 
		"mxd_tangent_arm_convert_moving_motor_position_to_angle()";

	double moving_motor_angle;
	double moving_motor_ratio, angle_offset;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s: moving motor position = %g",
					fname, moving_motor_position));
	MX_DEBUG( 2,("%s: tangent_arm->arm_length = %g",
					fname, tangent_arm->arm_length));

	/* Compute the ratio of the moving motor position to the arm length.
	 * This will be used as the input to the atan() or asin() function.
	 */

	moving_motor_ratio = mx_divide_safely( moving_motor_position,
					tangent_arm->arm_length );

	MX_DEBUG( 2,("%s: moving_motor_ratio = %g", fname, moving_motor_ratio));

	/* Compute the angle in radians. */

	if ( motor->record->mx_type == MXT_MTR_TANGENT_ARM ) {
		moving_motor_angle = atan( moving_motor_ratio );
	} else {
		moving_motor_angle = asin( moving_motor_ratio );
	}

	MX_DEBUG( 2,("%s: moving_motor_angle = %g",
			fname, moving_motor_angle));

	/* Get the angle offset value. */

	mx_status = mxd_tangent_arm_get_angle_offset_value(
					tangent_arm, &angle_offset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: angle_offset = %g", fname, angle_offset));

	/* Compute the pseudomotor angle. */

	*angle = moving_motor_angle + angle_offset;

	MX_DEBUG( 2,("%s: *angle = %g", fname, *angle));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_tangent_arm_get_angle_offset_value(
		MX_TANGENT_ARM *tangent_arm,
		double *angle_offset_value )
{
	static const char fname[] = "mxd_tangent_arm_get_angle_offset()";

	MX_RECORD *angle_offset_record;
	mx_status_type mx_status;

	if ( tangent_arm == (MX_TANGENT_ARM *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL pointer passed." );
	}

	angle_offset_record = tangent_arm->angle_offset_record;

	if ( angle_offset_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"angle_offset_record pointer for '%s' record '%s' is NULL.",
			mx_get_driver_name( angle_offset_record ),
			tangent_arm->motor->record->name );
	}

	/* Get the angle offset value. */

	switch ( angle_offset_record->mx_superclass ) {
	case MXR_VARIABLE:
		mx_status = mx_get_double_variable( angle_offset_record,
							angle_offset_value );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXR_DEVICE:
		switch( angle_offset_record->mx_class ) {
		case MXC_MOTOR:
			mx_status = mx_motor_get_position(
				angle_offset_record, angle_offset_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
		"The device class %ld for record '%s' is not supported "
		"for the angle offset of a '%s' in record '%s'.",
				angle_offset_record->mx_class,
				angle_offset_record->name,
				mx_get_driver_name( angle_offset_record ),
				tangent_arm->motor->record->name );
		}
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"The record superclass %ld for record '%s' is not supported "
		"for the angle offset of a '%s' in record '%s'.",
			angle_offset_record->mx_superclass,
			angle_offset_record->name,
			mx_get_driver_name( angle_offset_record ),
			tangent_arm->motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_tangent_arm_set_angle_offset_value(
		MX_TANGENT_ARM *tangent_arm,
		double angle_offset_value )
{
	static const char fname[] = "mxd_tangent_arm_set_angle_offset()";

	MX_RECORD *angle_offset_record;
	mx_status_type mx_status;

	if ( tangent_arm == (MX_TANGENT_ARM *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL pointer passed." );
	}

	angle_offset_record = tangent_arm->angle_offset_record;

	if ( angle_offset_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"angle_offset_record pointer for '%s' record '%s' is NULL.",
			mx_get_driver_name( angle_offset_record ),
			tangent_arm->motor->record->name );
	}

	/* Set the angle offset value. */

	switch ( angle_offset_record->mx_superclass ) {
	case MXR_VARIABLE:
		mx_status = mx_set_double_variable( angle_offset_record,
							angle_offset_value );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXR_DEVICE:
		switch( angle_offset_record->mx_class ) {
		case MXC_MOTOR:
			mx_status = mx_motor_set_position(
				angle_offset_record, angle_offset_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
		"The device class %ld for record '%s' is not supported "
		"for the angle offset of a '%s' in record '%s'.",
				angle_offset_record->mx_class,
				angle_offset_record->name,
				mx_get_driver_name( angle_offset_record ),
				tangent_arm->motor->record->name );
		}
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"The record superclass %ld for record '%s' is not supported "
		"for the angle offset of a '%s' in record '%s'.",
			angle_offset_record->mx_superclass,
			angle_offset_record->name,
			mx_get_driver_name( angle_offset_record ),
			tangent_arm->motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

