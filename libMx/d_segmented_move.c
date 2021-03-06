/*
 * Name:    d_segmented_move.c 
 *
 * Purpose: MX motor driver for motors that break up long moves into
 *          short segments.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2006, 2010, 2013, 2020
 *    Illinois Institute of Technology
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
#include "d_segmented_move.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_segmented_move_record_function_list = {
	NULL,
	mxd_segmented_move_create_record_structures,
	mxd_segmented_move_finish_record_initialization,
	NULL,
	mxd_segmented_move_print_motor_structure,
	NULL,
	NULL,
	NULL,
	mxd_segmented_move_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_segmented_move_motor_function_list = {
	mxd_segmented_move_motor_is_busy,
	mxd_segmented_move_move_absolute,
	mxd_segmented_move_get_position,
	mxd_segmented_move_set_position,
	mxd_segmented_move_soft_abort,
	mxd_segmented_move_immediate_abort,
	mxd_segmented_move_positive_limit_hit,
	mxd_segmented_move_negative_limit_hit,
	mxd_segmented_move_raw_home_command,
	mxd_segmented_move_constant_velocity_move,
	mxd_segmented_move_get_parameter,
	mxd_segmented_move_set_parameter
};

/* Soft motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_segmented_move_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SEGMENTED_MOVE_STANDARD_FIELDS
};

long mxd_segmented_move_num_record_fields
		= sizeof( mxd_segmented_move_record_field_defaults )
			/ sizeof( mxd_segmented_move_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_segmented_move_rfield_def_ptr
			= &mxd_segmented_move_record_field_defaults[0];

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_segmented_move_get_pointers( MX_MOTOR *motor,
			MX_SEGMENTED_MOVE **segmented_move,
			const char *calling_fname )
{
	static const char fname[] = "mxd_segmented_move_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( segmented_move == (MX_SEGMENTED_MOVE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SEGMENTED_MOVE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_MOTOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	*segmented_move = (MX_SEGMENTED_MOVE *)
				motor->record->record_type_struct;

	if ( *segmented_move == (MX_SEGMENTED_MOVE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SEGMENTED_MOVE pointer for motor record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
	}

	if ( (*segmented_move)->real_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX real_motor_record pointer for MX segmented move motor '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_segmented_move_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_segmented_move_create_record_structures()";

	MX_MOTOR *motor;
	MX_SEGMENTED_MOVE *segmented_move;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	segmented_move = (MX_SEGMENTED_MOVE *)
				malloc( sizeof(MX_SEGMENTED_MOVE) );

	if ( segmented_move == (MX_SEGMENTED_MOVE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SEGMENTED_MOVE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = segmented_move;
	record->class_specific_function_list
				= &mxd_segmented_move_motor_function_list;

	motor->record = record;

	segmented_move->more_segments_needed = FALSE;
	segmented_move->final_destination = 0.0;

	/* A segmented move motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_segmented_move_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_segmented_move_finish_record_initialization()";

	MX_MOTOR *motor;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;
	motor->motor_flags |= MXF_MTR_PSEUDOMOTOR_RECURSION_IS_NOT_NECESSARY;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_segmented_move_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[]
		= "mxd_segmented_move_print_motor_structure()";

	MX_MOTOR *motor;
	MX_SEGMENTED_MOVE *segmented_move;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type        = SEGMENTED_MOVE.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  real motor record = %s\n",
				segmented_move->real_motor_record->name );
	fprintf(file, "  segment length    = %g %s\n",
					segmented_move->segment_length,
					motor->units );

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
mxd_segmented_move_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_segmented_move_resynchronize()";

	MX_MOTOR *motor;
	MX_SEGMENTED_MOVE *segmented_move;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->busy = FALSE;

		return mx_status;
	}

	mx_status =
		mx_resynchronize_record( segmented_move->real_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_motor_is_busy()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->busy = FALSE;

		return mx_status;
	}

	mx_status = mx_motor_is_busy( segmented_move->real_motor_record, &busy );

	if ( busy ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	MX_DEBUG( 2,("%s: busy = %d, more_segments_needed = %d",
		fname, (int)motor->busy, segmented_move->more_segments_needed));

	if ( ( motor->busy == FALSE )
	  && ( segmented_move->more_segments_needed == TRUE ) )
	{
		/* Need to start the next segment. */

		MX_DEBUG( 2,("%s: starting the next segment.", fname));

		motor->busy = TRUE;

		mx_status = mxd_segmented_move_move_absolute( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_move_absolute()";

	MX_SEGMENTED_MOVE *segmented_move;
	double destination, segment_destination;
	double current_position, relative_distance;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	destination = motor->raw_destination.analog;

	mx_status = mx_motor_get_position( segmented_move->real_motor_record,
						&current_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( segmented_move->more_segments_needed == FALSE ) {
		segmented_move->final_destination = destination;
	}

	relative_distance =
		segmented_move->final_destination - current_position;

	if ( fabs(relative_distance) > segmented_move->segment_length ) {
		segmented_move->more_segments_needed = TRUE;

		if ( relative_distance > 0.0 ) {
			segment_destination = current_position
					+ segmented_move->segment_length;
		} else {
			segment_destination = current_position
					- segmented_move->segment_length;
		}
	} else {
		segmented_move->more_segments_needed = FALSE;

		segment_destination = segmented_move->final_destination;
	}

	MX_DEBUG( 2,
	("%s: final_dest = %g, current_pos = %g, relative_dist = %g",
		fname, segmented_move->final_destination,
		current_position, relative_distance));

	MX_DEBUG( 2,("%s: more_segments_needed = %d, segment_destination = %g",
		fname, segmented_move->more_segments_needed,
		segment_destination));

	/* Start this move segment.
	 *
	 * We need to ignore backlash corrections here.  Otherwise, the
	 * motor would perform a backlash correction on each segment.
	 * If you want a backlash correction for the move as a whole,
	 * you should specify a backlash correction for this pseudomotor.
	 */

	mx_status = mx_motor_move_absolute( segmented_move->real_motor_record,
				segment_destination,
				MXF_MTR_NOWAIT | MXF_MTR_IGNORE_BACKLASH );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_get_position()";

	MX_SEGMENTED_MOVE *segmented_move;
	double position;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( segmented_move->real_motor_record,
							&position );

	motor->raw_position.analog = position;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_set_position()";

	MX_SEGMENTED_MOVE *segmented_move;
	double new_set_position;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_set_position = motor->raw_set_position.analog;

	mx_status = mx_motor_set_position( segmented_move->real_motor_record,
						new_set_position );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_soft_abort()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	segmented_move->more_segments_needed = FALSE;

	mx_status = mx_motor_soft_abort( segmented_move->real_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_immediate_abort()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	segmented_move->more_segments_needed = FALSE;

	mx_status =
		mx_motor_immediate_abort( segmented_move->real_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_positive_limit_hit()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status =
		mx_motor_positive_limit_hit( segmented_move->real_motor_record,
						&limit_hit );

	motor->positive_limit_hit = limit_hit;

	if ( limit_hit ) {
		segmented_move->more_segments_needed = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_negative_limit_hit()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status =
		mx_motor_negative_limit_hit( segmented_move->real_motor_record,
						&limit_hit );

	motor->negative_limit_hit = limit_hit;

	if ( limit_hit ) {
		segmented_move->more_segments_needed = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_raw_home_command()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_raw_home_command(
			segmented_move->real_motor_record,
			motor->raw_home_command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_segmented_move_constant_velocity_move()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_constant_velocity_move(
			segmented_move->real_motor_record,
			motor->constant_velocity_move );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_get_parameter()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

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
			mx_motor_get_speed( segmented_move->real_motor_record,
						&(motor->raw_speed) );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_get_base_speed(
					segmented_move->real_motor_record,
					&(motor->raw_base_speed) );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_get_maximum_speed(
					segmented_move->real_motor_record,
					&(motor->raw_maximum_speed) );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		mx_status = mx_motor_get_synchronous_motion_mode(
					segmented_move->real_motor_record,
					&(motor->synchronous_motion_mode) );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mx_motor_get_raw_acceleration_parameters(
					segmented_move->real_motor_record,
					motor->raw_acceleration_parameters );
		break;

	case MXLV_MTR_ACCELERATION_DISTANCE:
		mx_status = mx_motor_get_acceleration_distance(
					segmented_move->real_motor_record,
					&(motor->acceleration_distance) );
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time(
					segmented_move->real_motor_record,
					&(motor->acceleration_time) );
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		mx_status = mx_motor_compute_extended_scan_range(
				segmented_move->real_motor_record,
				motor->raw_compute_extended_scan_range[0],
				motor->raw_compute_extended_scan_range[1],
				&(motor->raw_compute_extended_scan_range[2]),
				&(motor->raw_compute_extended_scan_range[3]) );
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		mx_status =
		    mx_motor_compute_pseudomotor_position_from_real_position(
				segmented_move->real_motor_record,
				motor->compute_pseudomotor_position[0],
				&(motor->compute_pseudomotor_position[1]),
				TRUE );
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		mx_status =
		    mx_motor_compute_real_position_from_pseudomotor_position(
				segmented_move->real_motor_record,
				motor->compute_real_position[0],
				&(motor->compute_real_position[1]),
				TRUE );
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_segmented_move_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_segmented_move_set_parameter()";

	MX_SEGMENTED_MOVE *segmented_move;
	mx_status_type mx_status;

	mx_status = mxd_segmented_move_get_pointers( motor,
						&segmented_move, fname );

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
			mx_motor_set_speed( segmented_move->real_motor_record,
						motor->raw_speed );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_set_base_speed(
					segmented_move->real_motor_record,
					motor->raw_base_speed );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_set_maximum_speed(
					segmented_move->real_motor_record,
					motor->raw_maximum_speed );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mx_motor_set_raw_acceleration_parameters(
					segmented_move->real_motor_record,
					motor->raw_acceleration_parameters );
		break;

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		mx_status = mx_motor_set_speed_between_positions(
					segmented_move->real_motor_record,
					motor->raw_speed_choice_parameters[0],
					motor->raw_speed_choice_parameters[1],
					motor->raw_speed_choice_parameters[2] );
		break;

	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed(
					segmented_move->real_motor_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		mx_status = mx_motor_restore_speed(
					segmented_move->real_motor_record );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		mx_status = mx_motor_set_synchronous_motion_mode(
					segmented_move->real_motor_record,
					motor->synchronous_motion_mode );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

