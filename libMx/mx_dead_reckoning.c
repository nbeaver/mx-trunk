/*
 * Name:    mx_dead_reckoning.c
 *
 * Purpose: Functions for dead reckoning calculations of motor positions.
 *
 *          Currently this set of routines only works for motors that
 *          have a constant acceleration and deceleration in units/sec**2.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2015, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_DEAD_RECKONING_DEBUG		FALSE

#include <stdio.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_clock_tick.h"
#include "mx_dead_reckoning.h"

MX_EXPORT mx_status_type
mx_dead_reckoning_initialize( MX_DEAD_RECKONING *dead_reckoning,
				MX_MOTOR *motor )
{
	static const char fname[] = "mx_dead_reckoning_initialize()";

	MX_CLOCK_TICK current_tick;
	double current_position;

	if ( dead_reckoning == (MX_DEAD_RECKONING *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DEAD_RECKONING pointer passed was NULL." );
	}
	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed was NULL." );
	}

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s'.",
		fname, motor->record->name));
#endif

	dead_reckoning->motor = motor;

	/* Get the current time. */

	current_tick = mx_current_clock_tick();

	/* Indicate that no move is in progress by setting all the event
	 * times to the current time and all of the positions to the current
	 * position.
	 */

	current_position = dead_reckoning->motor->position;

	dead_reckoning->start_of_move_tick = current_tick;
	dead_reckoning->end_of_acceleration_tick = current_tick;
	dead_reckoning->start_of_deceleration_tick = current_tick;
	dead_reckoning->end_of_move_tick = current_tick;

	dead_reckoning->start_position = current_position;
	dead_reckoning->end_of_acceleration_position = current_position;
	dead_reckoning->start_of_deceleration_position = current_position;
	dead_reckoning->end_position = current_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dead_reckoning_start_motion( MX_DEAD_RECKONING *dead_reckoning,
				double start_position,
				double end_position,
				MX_CLOCK_TICK *starting_clock_tick )
{
	static const char fname[] = "mx_dead_reckoning_start_motion()";

	MX_MOTOR *motor;

	MX_CLOCK_TICK current_clock_tick;
	MX_CLOCK_TICK end_of_accel_relative_tick;
	MX_CLOCK_TICK start_of_decel_relative_tick;
	MX_CLOCK_TICK end_of_move_relative_tick;

	double end_of_accel_relative_time;
	double start_of_decel_relative_time;
	double end_of_move_relative_time;

	double base_speed, slew_speed, slew_time;
	double acceleration, acceleration_distance, acceleration_time;
	double move_distance, slew_distance;

	double middle_position, middle_speed, middle_speed_squared;
	double middle_relative_time;

	mx_status_type mx_status;

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s'.",
		fname, dead_reckoning->motor->record->name));
#endif

	if ( dead_reckoning == (MX_DEAD_RECKONING *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DEAD_RECKONING pointer passed was NULL." );
	}

	dead_reckoning->start_position = start_position;
	dead_reckoning->end_position = end_position;

	motor = dead_reckoning->motor;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MOTOR pointer for the MX_DEAD_RECKONING pointer passed was NULL." );
	}

	if ( motor->acceleration_type != MXF_MTR_ACCEL_RATE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The acceleration type %ld for motor '%s' is not supported.  "
		"Only acceleration type MXF_MTR_ACCEL_RATE is supported.",
			motor->acceleration_type, motor->record->name );
	}

	motor->busy = TRUE;

	if ( starting_clock_tick == (MX_CLOCK_TICK *) NULL ) {
		current_clock_tick = mx_current_clock_tick();
	} else {
		current_clock_tick = *starting_clock_tick;
	}

	move_distance = end_position - start_position;

	/* Make sure that the motor speeds are current. */

	mx_status = mx_motor_get_base_speed( motor->record, &base_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_speed( motor->record, &slew_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute acceleration parameters. */

	acceleration =
		fabs( motor->scale * motor->raw_acceleration_parameters[0] );

	mx_status = mx_motor_get_acceleration_distance( motor->record,
						&acceleration_distance );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( move_distance < 0.0 ) {
		acceleration_distance = - acceleration_distance;
	}

	mx_status = mx_motor_get_acceleration_time( motor->record,
						&acceleration_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s: start_position = %g, end_position = %g",
				fname, start_position, end_position));
	MX_DEBUG(-2,("%s: move_distance = %g", fname, move_distance));
	MX_DEBUG(-2,("%s: base_speed = %g, slew_speed = %g",
				fname, base_speed, slew_speed));
	MX_DEBUG(-2,("%s: acceleration = %g", fname, acceleration));
	MX_DEBUG(-2,("%s: acceleration_distance = %g",
				fname, acceleration_distance));
	MX_DEBUG(-2,("%s: acceleration_time = %g",
				fname, acceleration_time));
#endif

	/* See if the motor has a chance to reach full speed. */

	if ( fabs( move_distance ) >= fabs( 2.0 * acceleration_distance ) ) {

		/* The motor will reach full speed. */

		dead_reckoning->end_of_acceleration_position = 
				start_position + acceleration_distance;

		dead_reckoning->start_of_deceleration_position =
				end_position - acceleration_distance;

		slew_distance = end_position - start_position
					- 2.0 * acceleration_distance;

		slew_time = fabs( mx_divide_safely(slew_distance, slew_speed) );

		end_of_accel_relative_time = acceleration_time;
		start_of_decel_relative_time
					= slew_time + acceleration_time;
		end_of_move_relative_time = slew_time + 2.0 * acceleration_time;

#if MX_DEAD_RECKONING_DEBUG
		MX_DEBUG(-2,("%s: slew_distance = %g", fname, slew_distance));
		MX_DEBUG(-2,("%s: slew_time = %g", fname, slew_time));
#endif

	} else {

		/* The motor will _not_ reach full speed.
		 *
		 * Look for the velocity at which the acceleration
		 * and deceleration profiles intersect.  This will
		 * be at the middle of the move.
		 */

		middle_position = 0.5 * ( start_position + end_position );

		dead_reckoning->end_of_acceleration_position = middle_position;

		dead_reckoning->start_of_deceleration_position
							= middle_position;

		middle_speed_squared = base_speed * base_speed + 2.0
		  * fabs(acceleration * ( middle_position - start_position ));

		middle_speed = sqrt( middle_speed_squared );

		middle_relative_time = mx_divide_safely(
					middle_speed - base_speed,
						acceleration );

		end_of_accel_relative_time = middle_relative_time;
		start_of_decel_relative_time = middle_relative_time;
		end_of_move_relative_time = 2.0 * middle_relative_time;

#if MX_DEAD_RECKONING_DEBUG
		MX_DEBUG(-2,("%s: middle_position = %g",
				fname, middle_position));
		MX_DEBUG(-2,("%s: middle_speed_squared = %g",
				fname, middle_speed_squared));
		MX_DEBUG(-2,("%s: middle_speed = %g", fname, middle_speed));
		MX_DEBUG(-2,("%s: middle_relative_time = %g",
				fname, middle_relative_time));
#endif
	}

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s: ...->end_of_acceleration_position = %g",
		fname, dead_reckoning->end_of_acceleration_position));
	MX_DEBUG(-2,("%s: ...->start_of_deceleration_position = %g",
		fname, dead_reckoning->start_of_deceleration_position));

	MX_DEBUG(-2,("%s: end_of_accel_relative_time = %g",
			fname, end_of_accel_relative_time));
	MX_DEBUG(-2,("%s: start_of_decel_relative_time = %g",
			fname, start_of_decel_relative_time));
	MX_DEBUG(-2,("%s: end_of_move_relative_time = %g",
			fname, end_of_move_relative_time));
#endif

	/*===*/

	end_of_accel_relative_tick = mx_convert_seconds_to_clock_ticks(
					end_of_accel_relative_time );

	start_of_decel_relative_tick = mx_convert_seconds_to_clock_ticks(
					start_of_decel_relative_time );

	end_of_move_relative_tick = mx_convert_seconds_to_clock_ticks(
					end_of_move_relative_time );

	/*===*/

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s: current_clock_tick = (%lu,%lu)",
		fname, current_clock_tick.high_order,
		(unsigned long) current_clock_tick.low_order));

	MX_DEBUG(-2,("%s: end_of_accel_relative_tick = (%lu,%lu)",
		fname, end_of_accel_relative_tick.high_order,
		(unsigned long) end_of_accel_relative_tick.low_order));

	MX_DEBUG(-2,("%s: start_of_decel_relative_tick = (%lu,%lu)",
		fname, start_of_decel_relative_tick.high_order,
		(unsigned long) start_of_decel_relative_tick.low_order));

	MX_DEBUG(-2,("%s: end_of_move_relative_tick = (%lu,%lu)",
		fname, end_of_move_relative_tick.high_order,
		(unsigned long) end_of_move_relative_tick.low_order));
#endif

	/*===*/

	dead_reckoning->start_of_move_tick = current_clock_tick;

	dead_reckoning->end_of_acceleration_tick = mx_add_clock_ticks(
		current_clock_tick, end_of_accel_relative_tick );

	dead_reckoning->start_of_deceleration_tick = mx_add_clock_ticks(
		current_clock_tick, start_of_decel_relative_tick );

	dead_reckoning->end_of_move_tick = mx_add_clock_ticks(
		current_clock_tick, end_of_move_relative_tick );

	/*===*/

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s: ...->start_of_move_tick = (%lu,%lu)",
		fname, dead_reckoning->start_of_move_tick.high_order,
		(unsigned long) dead_reckoning->start_of_move_tick.low_order));

	MX_DEBUG(-2,("%s: ...->end_of_acceleration_tick = (%lu,%lu)",
		fname, dead_reckoning->end_of_acceleration_tick.high_order,
	(unsigned long) dead_reckoning->end_of_acceleration_tick.low_order));

	MX_DEBUG(-2, ("%s: ...->start_of_deceleration_tick = (%lu,%lu)",
		fname, dead_reckoning->start_of_deceleration_tick.high_order,
	(unsigned long) dead_reckoning->start_of_deceleration_tick.low_order));

	MX_DEBUG(-2,("%s: ...->end_of_move_tick = (%lu,%lu)",
		fname, dead_reckoning->end_of_move_tick.high_order,
		(unsigned long) dead_reckoning->end_of_move_tick.low_order));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dead_reckoning_predict_motion( MX_DEAD_RECKONING *dead_reckoning,
				double *present_position,
				double *present_velocity )
{
	static const char fname[] = "mx_dead_reckoning_predict_motion()";

	MX_MOTOR *motor;
	MX_CLOCK_TICK current_tick;

	MX_CLOCK_TICK start_of_move_tick;
	MX_CLOCK_TICK end_of_accel_tick;
	MX_CLOCK_TICK start_of_decel_tick;
	MX_CLOCK_TICK end_of_move_tick;

	MX_CLOCK_TICK relative_ticks;

	double raw_position, position, velocity;
	double base_speed, slew_speed, acceleration;
	double relative_time;
	char status_string[40];

	if ( dead_reckoning == (MX_DEAD_RECKONING *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DEAD_RECKONING pointer passed was NULL." );
	}

	start_of_move_tick  = dead_reckoning->start_of_move_tick;
	end_of_accel_tick   = dead_reckoning->end_of_acceleration_tick;
	start_of_decel_tick = dead_reckoning->start_of_deceleration_tick;
	end_of_move_tick    = dead_reckoning->end_of_move_tick;

	motor = dead_reckoning->motor;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_MOTOR pointer for the MX_DEAD_RECKONING pointer passed is NULL." );
	}

	base_speed = motor->base_speed;
	slew_speed = motor->speed;
	acceleration = 
		fabs( motor->scale * motor->raw_acceleration_parameters[0] );

	if ( dead_reckoning->end_position < dead_reckoning->start_position ) {
		base_speed = -base_speed;
		slew_speed = -slew_speed;
		acceleration = -acceleration;
	}

	motor->busy = TRUE;

	/* Get the current time. */

	current_tick = mx_current_clock_tick();

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s: current_tick = (%lu,%lu)",
			fname, current_tick.high_order,
			(unsigned long) current_tick.low_order));
#endif

	/* Figure out which section of the move we are currently in. */

	if ( mx_compare_clock_ticks(current_tick, end_of_move_tick) >= 0 ) {

		position = dead_reckoning->end_position;
		velocity = 0.0;

		strlcpy( status_string, "MOVE COMPLETE", sizeof(status_string));

		motor->busy = FALSE;

	} else
	if ( mx_compare_clock_ticks(current_tick, start_of_decel_tick) >= 0 ) {

		relative_ticks = mx_subtract_clock_ticks( end_of_move_tick,
								current_tick );

		relative_time = - mx_convert_clock_ticks_to_seconds(
							relative_ticks );

#if MX_DEAD_RECKONING_DEBUG
		MX_DEBUG(-2,
		("%s: relative_ticks = (%lu,%lu), relative_time = %g",
			fname, relative_ticks.high_order,
			(unsigned long) relative_ticks.low_order,
			relative_time ));
#endif

		position = dead_reckoning->end_position
			+ base_speed * relative_time
			- 0.5 * acceleration * relative_time * relative_time;

		velocity = base_speed - acceleration * relative_time;

		strlcpy( status_string, "DECELERATING", sizeof(status_string) );

	} else
	if ( mx_compare_clock_ticks(current_tick, end_of_accel_tick) >= 0 ) {

		relative_ticks = mx_subtract_clock_ticks( current_tick,
							end_of_accel_tick );

		relative_time = mx_convert_clock_ticks_to_seconds(
							relative_ticks );

#if MX_DEAD_RECKONING_DEBUG
		MX_DEBUG(-2,
		("%s: relative_ticks = (%lu,%lu), relative_time = %g",
			fname, relative_ticks.high_order,
			(unsigned long) relative_ticks.low_order,
			relative_time ));
#endif

		position = dead_reckoning->end_of_acceleration_position
				+ slew_speed * relative_time;

		velocity = slew_speed;

		strlcpy( status_string, "MOVING", sizeof(status_string) );

	} else
	if ( mx_compare_clock_ticks(current_tick, start_of_move_tick) >= 0 ) {

		relative_ticks = mx_subtract_clock_ticks( current_tick,
							start_of_move_tick );

		relative_time = mx_convert_clock_ticks_to_seconds(
							relative_ticks );

#if MX_DEAD_RECKONING_DEBUG
		MX_DEBUG(-2,
		("%s: relative_ticks = (%lu,%lu), relative_time = %g",
			fname, relative_ticks.high_order,
			(unsigned long) relative_ticks.low_order,
			relative_time ));
#endif

		position = dead_reckoning->start_position
			+ base_speed * relative_time
			+ 0.5 * acceleration * relative_time * relative_time;

		velocity = base_speed + acceleration * relative_time;

		strlcpy( status_string, "ACCELERATING", sizeof(status_string) );

	} else {

		position = dead_reckoning->start_position;
		velocity = 0.0;

		strlcpy( status_string, "BEFORE START", sizeof(status_string) );
	}

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s: %s, position = %g, velocity = %g",
			fname, status_string, position, velocity));
#endif

	raw_position = mx_divide_safely( position - motor->offset,
						motor->scale );

	switch( motor->subclass ) {
	case MXC_MTR_STEPPER:
		motor->raw_position.stepper = mx_round( raw_position );
		break;
	case MXC_MTR_ANALOG:
		motor->raw_position.analog = raw_position;
		break;
	}

	motor->position = position;

	if ( present_position != NULL ) {
		*present_position = position;
	}
	if ( present_velocity != NULL ) {
		*present_velocity = velocity;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_dead_reckoning_abort_motion( MX_DEAD_RECKONING *dead_reckoning )
{
#if MX_DEAD_RECKONING_DEBUG
	static const char fname[] = "mx_dead_reckoning_abort_motion()";
#endif

	MX_CLOCK_TICK current_tick;
	double current_position;
	mx_status_type mx_status;

#if MX_DEAD_RECKONING_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s'.",
		fname, dead_reckoning->motor->record->name));
#endif

	/* Refresh the current motor position. */

	mx_status = mx_dead_reckoning_predict_motion( dead_reckoning,
							NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	current_position = dead_reckoning->motor->position;

	/* Get the current time. */

	current_tick = mx_current_clock_tick();

	/* Abort the move by setting all the event times to the current time
	 * and all of the positions to the current position.
	 */

	dead_reckoning->start_of_move_tick = current_tick;
	dead_reckoning->end_of_acceleration_tick = current_tick;
	dead_reckoning->start_of_deceleration_tick = current_tick;
	dead_reckoning->end_of_move_tick = current_tick;

	dead_reckoning->start_position = current_position;
	dead_reckoning->end_of_acceleration_position = current_position;
	dead_reckoning->start_of_deceleration_position = current_position;
	dead_reckoning->end_position = current_position;

	return MX_SUCCESSFUL_RESULT;
}

