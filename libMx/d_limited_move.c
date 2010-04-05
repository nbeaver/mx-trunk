/*
 * Name:    d_limited_move.c 
 *
 * Purpose: MX pseudomotor driver that limits the magnitude of relative moves.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_LIMITED_MOVE_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "d_limited_move.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_limited_move_record_function_list = {
	NULL,
	mxd_limited_move_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	mxd_limited_move_print_motor_structure,
	NULL,
	NULL,
	mxd_limited_move_open,
	NULL,
	NULL,
	mxd_limited_move_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_limited_move_motor_function_list = {
	NULL,
	mxd_limited_move_move_absolute,
	mxd_limited_move_get_position,
	mxd_limited_move_set_position,
	mxd_limited_move_soft_abort,
	mxd_limited_move_immediate_abort,
	NULL,
	NULL,
	mxd_limited_move_find_home_position,
	mxd_limited_move_constant_velocity_move,
	mxd_limited_move_get_parameter,
	mxd_limited_move_set_parameter,
	NULL,
	mxd_limited_move_get_status,
	mxd_limited_move_get_extended_status
};

/* Soft motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_limited_move_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_LIMITED_MOVE_STANDARD_FIELDS
};

long mxd_limited_move_num_record_fields
		= sizeof( mxd_limited_move_record_field_defaults )
			/ sizeof( mxd_limited_move_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_limited_move_rfield_def_ptr
			= &mxd_limited_move_record_field_defaults[0];

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_limited_move_get_pointers( MX_MOTOR *motor,
			MX_LIMITED_MOVE **limited_move,
			const char *calling_fname )
{
	static const char fname[] = "mxd_limited_move_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( limited_move == (MX_LIMITED_MOVE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_LIMITED_MOVE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_MOTOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	*limited_move = (MX_LIMITED_MOVE *)
				motor->record->record_type_struct;

	if ( *limited_move == (MX_LIMITED_MOVE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_LIMITED_MOVE pointer for motor record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
	}

	if ( (*limited_move)->real_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX real_motor_record pointer for MX limited move motor '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_limited_move_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_limited_move_create_record_structures()";

	MX_MOTOR *motor;
	MX_LIMITED_MOVE *limited_move;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	limited_move = (MX_LIMITED_MOVE *)
				malloc( sizeof(MX_LIMITED_MOVE) );

	if ( limited_move == (MX_LIMITED_MOVE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_LIMITED_MOVE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = limited_move;
	record->class_specific_function_list
				= &mxd_limited_move_motor_function_list;

	motor->record = record;
	limited_move->record = record;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	/* A limited move motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_limited_move_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[]
		= "mxd_limited_move_print_motor_structure()";

	MX_MOTOR *motor;
	MX_LIMITED_MOVE *limited_move;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type        = LIMITED_MOVE.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  real motor record = %s\n",
				limited_move->real_motor_record->name );
	fprintf(file, "  relative negative limit = %g\n",
				limited_move->relative_negative_limit );
	fprintf(file, "  relative positive limit = %g\n",
				limited_move->relative_positive_limit );

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  position          = %g %s (%g)\n",
		motor->position, motor->units, motor->raw_position.analog );

	fprintf(file, "  scale             = %g %s per step.\n",
		motor->scale, motor->units );

	fprintf(file, "  offset            = %g %s.\n",
		motor->offset, motor->units );

	fprintf(file, "  backlash          = %g %s (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );

	fprintf(file, "  negative_limit    = %g %s (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive_limit    = %g %s (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband     = %g %s (%g)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to get the speed of motor '%s'",
			record->name );
	}

	fprintf(file, "  speed             = %g %s/sec\n\n",
		speed, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_limited_move_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_limited_move_open()";

	MX_MOTOR *motor;
	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->real_motor_record = limited_move->real_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_limited_move_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_limited_move_resynchronize()";

	MX_MOTOR *motor;
	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->busy = FALSE;

		return mx_status;
	}

	mx_status =
		mx_resynchronize_record( limited_move->real_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_move_absolute()";

	MX_LIMITED_MOVE *limited_move;
	double relative_distance;
	double scaled_negative_limit, scaled_positive_limit;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position(
			limited_move->real_motor_record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->scale >= 0.0 ) {
		scaled_negative_limit =
			motor->scale * limited_move->relative_negative_limit;
		scaled_positive_limit =
			motor->scale * limited_move->relative_positive_limit;
	} else {
		scaled_positive_limit =
			motor->scale * limited_move->relative_negative_limit;
		scaled_negative_limit =
			motor->scale * limited_move->relative_positive_limit;
	}

	relative_distance = motor->destination - motor->position;

	if ( relative_distance < scaled_negative_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested move from %g %s to %g %s is larger than the "
		"largest allowed relative move in the negative direction "
		"of %g %s for motor '%s'.  You must not attempt to move the "
		"motor by such a large distance.",
			motor->position, motor->units,
			motor->destination, motor->units,
			scaled_negative_limit, motor->units,
			motor->record->name );
	} else
	if ( relative_distance < scaled_positive_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested move from %g %s to %g %s is larger than the "
		"largest allowed relative move in the positive direction "
		"of %g %s for motor '%s'.  You must not attempt to move the "
		"motor by such a large distance.",
			motor->position, motor->units,
			motor->destination, motor->units,
			scaled_positive_limit, motor->units,
			motor->record->name );
	}

	mx_status = mx_motor_move_absolute( limited_move->real_motor_record,
					motor->raw_destination.analog, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_get_position()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( limited_move->real_motor_record,
						&(motor->raw_position.analog) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_set_position()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_set_position( limited_move->real_motor_record,
					motor->raw_set_position.analog );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_soft_abort()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( limited_move->real_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_immediate_abort()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status =
		mx_motor_immediate_abort( limited_move->real_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_find_home_position()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status =
		mx_motor_find_home_position( limited_move->real_motor_record,
						motor->home_search );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_limited_move_constant_velocity_move()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_constant_velocity_move(
					limited_move->real_motor_record,
					motor->constant_velocity_move );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_get_parameter()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status =
			mx_motor_get_speed( limited_move->real_motor_record,
						&(motor->raw_speed) );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_get_base_speed(
					limited_move->real_motor_record,
					&(motor->raw_base_speed) );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_get_maximum_speed(
					limited_move->real_motor_record,
					&(motor->raw_maximum_speed) );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		mx_status = mx_motor_get_synchronous_motion_mode(
					limited_move->real_motor_record,
					&(motor->synchronous_motion_mode) );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mx_motor_get_raw_acceleration_parameters(
					limited_move->real_motor_record,
					motor->raw_acceleration_parameters );
		break;

	case MXLV_MTR_ACCELERATION_DISTANCE:
		mx_status = mx_motor_get_acceleration_distance(
					limited_move->real_motor_record,
					&(motor->acceleration_distance) );
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time(
					limited_move->real_motor_record,
					&(motor->acceleration_time) );
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		mx_status = mx_motor_compute_extended_scan_range(
				limited_move->real_motor_record,
				motor->raw_compute_extended_scan_range[0],
				motor->raw_compute_extended_scan_range[1],
				&(motor->raw_compute_extended_scan_range[2]),
				&(motor->raw_compute_extended_scan_range[3]) );
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		mx_status =
		    mx_motor_compute_pseudomotor_position_from_real_position(
				limited_move->real_motor_record,
				motor->compute_pseudomotor_position[0],
				&(motor->compute_pseudomotor_position[1]),
				TRUE );
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		mx_status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
				limited_move->real_motor_record,
				motor->compute_real_position[0],
				&(motor->compute_real_position[1]),
				TRUE );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			motor->parameter_type );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_set_parameter()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch ( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status =
			mx_motor_set_speed( limited_move->real_motor_record,
						motor->raw_speed );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_set_base_speed(
					limited_move->real_motor_record,
					motor->raw_base_speed );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_set_maximum_speed(
					limited_move->real_motor_record,
					motor->raw_maximum_speed );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mx_motor_set_raw_acceleration_parameters(
					limited_move->real_motor_record,
					motor->raw_acceleration_parameters );
		break;

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		mx_status = mx_motor_set_speed_between_positions(
					limited_move->real_motor_record,
					motor->raw_speed_choice_parameters[0],
					motor->raw_speed_choice_parameters[1],
					motor->raw_speed_choice_parameters[2] );
		break;

	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed(
					limited_move->real_motor_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		mx_status = mx_motor_restore_speed(
					limited_move->real_motor_record );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		mx_status = mx_motor_set_synchronous_motion_mode(
					limited_move->real_motor_record,
					motor->synchronous_motion_mode );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %ld is not supported by this driver.",
			motor->parameter_type );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_get_status()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( limited_move->real_motor_record,
						&(motor->status) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_limited_move_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_limited_move_get_extended_status()";

	MX_LIMITED_MOVE *limited_move;
	mx_status_type mx_status;

	mx_status = mxd_limited_move_get_pointers( motor,
						&limited_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_extended_status(
			limited_move->real_motor_record,
			&(motor->raw_position.analog),
			&(motor->status) );

	return mx_status;
}

