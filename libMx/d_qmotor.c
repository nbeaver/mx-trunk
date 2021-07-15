/*
 * Name:    d_qmotor.c 
 *
 * Purpose: MX pseudomotor driver for the momentum transfer parameter q,
 *          which is defined as
 *
 *                 4 * pi * sin(theta)      2 * pi
 *          q  =  --------------------  =  --------
 *                       lambda               d
 *
 *          where lambda is the wavelength of the incident X-ray, theta
 *          is the angle of the analyzer arm and d is the effective 
 *          crystal d-spacing that is currently being probed.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006-2007, 2010, 2013, 2021
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_constants.h"
#include "mx_motor.h"
#include "d_qmotor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_q_motor_record_function_list = {
	NULL,
	mxd_q_motor_create_record_structures,
	mxd_q_motor_finish_record_initialization,
	NULL,
	mxd_q_motor_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_q_motor_motor_function_list = {
	mxd_q_motor_motor_is_busy,
	mxd_q_motor_move_absolute,
	mxd_q_motor_get_position,
	mxd_q_motor_set_position,
	mxd_q_motor_soft_abort,
	mxd_q_motor_immediate_abort,
	mxd_q_motor_positive_limit_hit,
	mxd_q_motor_negative_limit_hit,
	mxd_q_motor_raw_home_command,
	mxd_q_motor_constant_velocity_move,
	mxd_q_motor_get_parameter,
	mxd_q_motor_set_parameter
};

/* Q motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_q_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_Q_MOTOR_STANDARD_FIELDS
};

long mxd_q_motor_num_record_fields
		= sizeof( mxd_q_motor_record_field_defaults )
			/ sizeof( mxd_q_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_q_motor_rfield_def_ptr
			= &mxd_q_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_q_motor_get_pointers( MX_MOTOR *motor,
			MX_Q_MOTOR **q_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_q_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( q_motor == (MX_Q_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_Q_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*q_motor = (MX_Q_MOTOR *) (motor->record->record_type_struct);

	if ( *q_motor == (MX_Q_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_Q_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_q_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_q_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_Q_MOTOR *q_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	q_motor = (MX_Q_MOTOR *) malloc( sizeof(MX_Q_MOTOR) );

	if ( q_motor == (MX_Q_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_Q_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = q_motor;
	record->class_specific_function_list
				= &mxd_q_motor_motor_function_list;

	motor->record = record;

	/* The q motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_q_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_q_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_Q_MOTOR *q_motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	q_motor = (MX_Q_MOTOR *) record->record_type_struct;

	if ( q_motor == (MX_Q_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_Q_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->real_motor_record = q_motor->theta_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_q_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_q_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_RECORD *theta_motor_record;
	MX_MOTOR *dependent_motor;
	MX_Q_MOTOR *q_motor;
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

	q_motor = (MX_Q_MOTOR *) (record->record_type_struct);

	if ( q_motor == (MX_Q_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_Q_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	theta_motor_record = q_motor->theta_motor_record;

	if ( theta_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Dependent motor record pointer for record '%s' is NULL.",
			record->name );
	}

        dependent_motor = (MX_MOTOR *)
                                (theta_motor_record->record_class_struct);

        if ( dependent_motor == (MX_MOTOR *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                        "MX_MOTOR pointer for record '%s' is NULL.",
                        theta_motor_record->name );
        }

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type              = Q_MOTOR.\n\n");

	fprintf(file, "  name                    = %s\n", record->name);
	fprintf(file, "  theta motor record      = %s\n",
					theta_motor_record->name);
	fprintf(file, "  wavelength motor record = %s\n",
					q_motor->wavelength_motor_record->name);
	fprintf(file, "  angle scale             = %g\n",
					q_motor->angle_scale);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

        fprintf(file, "  position                = %.*g %s  (%.*g)\n",
		record->precision,
                motor->position, motor->units,
		record->precision,
                motor->raw_position.analog );
	fprintf(file, "  scale                   = %.*g %s per unscaled "
							"q unit.\n",
		record->precision,
		motor->scale, motor->units);
	fprintf(file, "  offset                  = %.*g %s.\n",
		record->precision,
		motor->offset, motor->units);
        fprintf(file, "  backlash                = %.*g %s  (%.*g).\n",
		record->precision,
                motor->backlash_correction, motor->units,
		record->precision,
                motor->raw_backlash_correction.analog);
        fprintf(file, "  negative limit          = %.*g %s  (%.*g).\n",
		record->precision,
                motor->negative_limit, motor->units,
		record->precision,
                motor->raw_negative_limit.analog);
        fprintf(file, "  positive limit          = %.*g %s  (%.*g).\n",
		record->precision,
		motor->positive_limit, motor->units,
		record->precision,
		motor->raw_positive_limit.analog);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband           = %.*g %s  (%.*g).\n\n",
		record->precision,
		move_deadband, motor->units,
		record->precision,
		motor->raw_move_deadband.analog);

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_q_motor_convert_theta_to_q( MX_MOTOR *motor,
			MX_Q_MOTOR *q_motor,
			double theta,
			double *q )
{
	double lambda, theta_in_radians, sin_theta;
	mx_status_type mx_status;

	/* The unscaled q value has to be in units that are the reciprocal
	 * of the units used by the wavelength motor.
	 */

	/* Get the wavelength of the incident radiation. */

	mx_status = mx_motor_get_position( q_motor->wavelength_motor_record,
					&lambda );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	theta_in_radians = theta * q_motor->angle_scale;

	sin_theta = sin( theta_in_radians );

	*q = mx_divide_safely( 4.0 * MX_PI * sin_theta, lambda );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_q_motor_convert_q_to_theta( MX_MOTOR *motor,
			MX_Q_MOTOR *q_motor,
			const char *operation,
			double q,
			double *theta )
{
	static const char fname[] = "mxd_q_motor_convert_q_to_theta()";

	MX_MOTOR *lambda_motor;
	double lambda, maximum_q;
	double angle_in_radians, sin_theta;
	mx_status_type mx_status;

	/* The unscaled q value has to be in units that are the reciprocal
	 * of the units used by the wavelength motor.
	 */

	/* Get the wavelength of the incident radiation. */

	mx_status = mx_motor_get_position( q_motor->wavelength_motor_record,
					&lambda );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	lambda_motor = (MX_MOTOR *)
		q_motor->wavelength_motor_record->record_class_struct;

	if ( lambda_motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for wavelength record '%s' is NULL.",
			q_motor->wavelength_motor_record->name );
	}

	/* Compute the maximum allowed value of q. */

	maximum_q = mx_divide_safely( 4.0 * MX_PI, lambda );

	/* Compute the analyzer theta angle. */

	sin_theta = q * lambda / ( 4.0 * MX_PI );

	if ( sin_theta > 1.0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Attempted to %s an unreachable q value of %g inverse %s for motor '%s'. The "
"maximum allowed q for a wavelength of %g %s is %g inverse %s.",
			operation,
			q, lambda_motor->units,
			motor->record->name,
			lambda, lambda_motor->units,
			maximum_q, lambda_motor->units );
	} else if ( sin_theta < -1.0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Attempted to %s an unreachable q value of %g inverse %s for motor '%s'. The "
"minimum allowed q for a wavelength of %g %s is %g inverse %s.",
			operation,
			q, lambda_motor->units,
			motor->record->name,
			lambda, lambda_motor->units,
			-maximum_q, lambda_motor->units );
	} else {
		angle_in_radians = asin( sin_theta );
	}

	*theta = mx_divide_safely( angle_in_radians, q_motor->angle_scale );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_q_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_motor_is_busy()";

	MX_Q_MOTOR *q_motor;
	mx_bool_type busy;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_is_busy( q_motor->theta_motor_record, &busy );

	motor->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_q_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_move_absolute()";

	MX_Q_MOTOR *q_motor;
	double q, theta;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	q = motor->raw_destination.analog;

	mx_status = mxd_q_motor_convert_q_to_theta( motor, q_motor, "move to",
						q, &theta );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute( q_motor->theta_motor_record,
						theta, 0 );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_q_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_get_position()";

	MX_Q_MOTOR *q_motor;
	double lambda, q, theta;
	mx_status_type mx_status;

	q_motor = NULL;
	q = 0.0;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the wavelength of the incident radiation. */

	mx_status = mx_motor_get_position( q_motor->wavelength_motor_record,
					&lambda );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the analyzer theta position. */

	mx_status = mx_motor_get_position( q_motor->theta_motor_record,
					&theta );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the q value. */

	mx_status = mxd_q_motor_convert_theta_to_q( motor, q_motor, theta, &q );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = q;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_q_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_set_position()";

	MX_Q_MOTOR *q_motor;
	double theta;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_q_motor_convert_q_to_theta( motor, q_motor, "set",
						motor->raw_set_position.analog,
						&theta );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_set_position( q_motor->theta_motor_record,
						theta );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_q_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_soft_abort()";

	MX_Q_MOTOR *q_motor;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( q_motor->theta_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_q_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_immediate_abort()";

	MX_Q_MOTOR *q_motor;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort( q_motor->theta_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_q_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_positive_limit_hit()";

	MX_Q_MOTOR *q_motor;
	mx_bool_type angle_limit_hit;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_positive_limit_hit( q_motor->theta_motor_record,
						&angle_limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( angle_limit_hit ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_q_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_negative_limit_hit()";

	MX_Q_MOTOR *q_motor;
	mx_bool_type angle_limit_hit;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_negative_limit_hit( q_motor->theta_motor_record,
						&angle_limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( angle_limit_hit ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_q_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_raw_home_command()";

	MX_Q_MOTOR *q_motor;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_raw_home_command( q_motor->theta_motor_record,
						motor->raw_home_command );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_q_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_constant_velocity_move()";

	MX_Q_MOTOR *q_motor;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_constant_velocity_move(
				q_motor->theta_motor_record,
				motor->constant_velocity_move );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_q_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_get_parameter()";

	MX_Q_MOTOR *q_motor;
	double theta, q, acceleration_time;
	double theta_start, theta_end;
	double real_theta_start, real_theta_end;
	double q_start, q_end;
	double real_q_start, real_q_end;
	mx_status_type mx_status;

	q_motor = NULL;
	q = real_q_start = real_q_end = 0.0;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
	case MXLV_MTR_ACCELERATION_DISTANCE:
		return mx_error( MXE_UNSUPPORTED, fname,
"Q pseudomotor '%s' cannot report the value of parameter '%s' (%ld).",
			motor->record->name,
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type );
		break;

	case MXLV_MTR_ACCELERATION_TYPE:
		motor->acceleration_type = MXF_MTR_ACCEL_NONE;
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time(
					q_motor->theta_motor_record,
					&acceleration_time );

		motor->acceleration_time = acceleration_time;
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		q_start = motor->raw_compute_extended_scan_range[0];
		q_end = motor->raw_compute_extended_scan_range[1];

		mx_status = mxd_q_motor_convert_q_to_theta(
					motor, q_motor,
					"compute theta for",
					q_start, &theta_start );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_q_motor_convert_q_to_theta(
					motor, q_motor,
					"compute theta for",
					q_end, &theta_end );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_compute_extended_scan_range(
					q_motor->theta_motor_record,
					theta_start, theta_end,
					&real_theta_start, &real_theta_end );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_q_motor_convert_theta_to_q(
					motor, q_motor,
					real_theta_start, &real_q_start);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxd_q_motor_convert_theta_to_q(
					motor, q_motor,
					real_theta_end, &real_q_end );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_compute_extended_scan_range[2] = real_q_start;
		motor->raw_compute_extended_scan_range[3] = real_q_end;
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		theta = motor->compute_pseudomotor_position[0];

		mx_status = mxd_q_motor_convert_theta_to_q(
				motor, q_motor, theta, &q );

		motor->compute_pseudomotor_position[1] = q;
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		q = motor->compute_real_position[0];

		mx_status = mxd_q_motor_convert_q_to_theta(
					motor, q_motor,
					"compute theta for",
					q, &theta );

		motor->compute_real_position[1] = theta;

		break;

	default:
		mx_status = mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_q_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_q_motor_set_parameter()";

	MX_Q_MOTOR *q_motor;
	double real_position1, real_position2, time_for_move;
	mx_status_type mx_status;

	q_motor = NULL;

	mx_status = mxd_q_motor_get_pointers( motor, &q_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Q pseudomotor '%s' cannot set the value of parameter '%s' (%ld).",
			motor->record->name,
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type );

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		mx_status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
			motor->record, motor->raw_speed_choice_parameters[0],
			&real_position1, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
			motor->record, motor->raw_speed_choice_parameters[1],
			&real_position2, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		time_for_move = motor->raw_speed_choice_parameters[2];

		mx_status = mx_motor_set_speed_between_positions(
					q_motor->theta_motor_record,
					real_position1, real_position2,
					time_for_move );
		break;

	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed( q_motor->theta_motor_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		mx_status = 
			mx_motor_restore_speed( q_motor->theta_motor_record );
		break;

	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

