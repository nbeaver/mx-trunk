/*
 * Name:    d_elapsed_time.c 
 *
 * Purpose: MX motor driver that can be used to scan using the wall clock
 *          elapsed time as the independent variable.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2007-2008, 2010-2011, 2013, 2015, 2021
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
#include <float.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_scan.h"
#include "mx_clock.h"
#include "d_elapsed_time.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_elapsed_time_record_function_list = {
	NULL,
	mxd_elapsed_time_create_record_structures,
	mxd_elapsed_time_finish_record_initialization,
	NULL,
	mxd_elapsed_time_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_elapsed_time_motor_function_list = {
	NULL,
	mxd_elapsed_time_move_absolute,
	mxd_elapsed_time_get_position,
	mxd_elapsed_time_set_position,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler
};

/* Elapsed time motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_elapsed_time_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_ELAPSED_TIME_MOTOR_STANDARD_FIELDS
};

long mxd_elapsed_time_num_record_fields
		= sizeof( mxd_elapsed_time_record_field_defaults )
			/ sizeof( mxd_elapsed_time_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_elapsed_time_rfield_def_ptr
			= &mxd_elapsed_time_record_field_defaults[0];

MX_EXPORT mx_status_type
mxd_elapsed_time_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_elapsed_time_create_record_structures()";

	MX_MOTOR *motor;
	MX_ELAPSED_TIME_MOTOR *elapsed_time;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	elapsed_time = (MX_ELAPSED_TIME_MOTOR *)
				malloc( sizeof(MX_ELAPSED_TIME_MOTOR) );

	if ( elapsed_time == (MX_ELAPSED_TIME_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ELAPSED_TIME_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = elapsed_time;
	record->class_specific_function_list
				= &mxd_elapsed_time_motor_function_list;

	motor->record = record;

	/* An elapsed time motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_elapsed_time_finish_record_initialization( MX_RECORD *record )
{
	MX_MOTOR *motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

#if 0
	/* WML (July 4, 2002): We no longer treat elapsed time as a
	 *  pseudomotor for two reasons:
	 *  1.  There is no underlying real motor.
	 *  2.  We now _do_ use elapsed_time motors in quick scans
	 *      as the motor record for 'mcs_time_mce' records.
	 *      It only serves as a placeholder, but it is useful
	 *      for reasons of clarity.
	 */

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;
	motor->motor_flags |= MXF_MTR_CANNOT_QUICK_SCAN;
#endif

	motor->raw_speed = 1.0;
	motor->raw_base_speed = 0.0;
	motor->raw_maximum_speed = DBL_MAX;

	motor->speed = motor->raw_speed * motor->scale;
	motor->base_speed = motor->raw_base_speed * motor->scale;
	motor->maximum_speed = motor->raw_maximum_speed * motor->scale;

	motor->acceleration_type = MXF_MTR_ACCEL_NONE;

	motor->raw_acceleration_parameters[0] = 0.0;
	motor->raw_acceleration_parameters[1] = 0.0;
	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_elapsed_time_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_elapsed_time_print_motor_structure()";

	MX_MOTOR *motor;
	MX_ELAPSED_TIME_MOTOR *elapsed_time;
	double position;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name);
	}

	elapsed_time = (MX_ELAPSED_TIME_MOTOR *) (record->record_type_struct);

	if ( elapsed_time == (MX_ELAPSED_TIME_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ELAPSED_TIME_MOTOR pointer for record '%s' is NULL.",
			record->name);
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type     = ELAPSED_TIME_MOTOR.\n\n");

	fprintf(file, "  name           = %s\n", record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  position       = %g %s (%g sec)\n",
		motor->position, motor->units,
		motor->raw_position.analog );

	fprintf(file, "  scale          = %g %s per second.\n",
		motor->scale, motor->units );

	fprintf(file, "  offset         = %g %s.\n",
		motor->offset, motor->units );

	fprintf(file, "  backlash       = %g %s  (%g sec)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );

	fprintf(file, "  negative limit = %g %s  (%g sec)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit = %g %s  (%g sec)\n\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	return MX_SUCCESSFUL_RESULT;
}

static double
mxd_elapsed_time_get_current_time( void )
{
	MX_CLOCK_TICK current_clock_tick;
	double current_time;

	current_clock_tick = mx_current_clock_tick();

	current_time = mx_convert_clock_ticks_to_seconds( current_clock_tick );

	return current_time;
}

MX_EXPORT mx_status_type
mxd_elapsed_time_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_elapsed_time_move_absolute()";

	MX_ELAPSED_TIME_MOTOR *elapsed_time_motor;
	double current_time, requested_time, time_since_reset;
	int interrupt;
	mx_status_type mx_status;

	elapsed_time_motor = (MX_ELAPSED_TIME_MOTOR *)
			motor->record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	current_time = mxd_elapsed_time_get_current_time();

	/* Use of a requested position that is less than or equal to zero
	 * is interpreted as request to reset the 'time_of_last_reset' field.
	 *
	 * Otherwise, we wait until the requested time has passed, unless
	 * the user interrupts us.
	 */

	requested_time = motor->raw_destination.analog;

	if ( requested_time <= 0.0 ) {
		elapsed_time_motor->time_of_last_reset = current_time;

		time_since_reset = 0.0;
	} else {
		time_since_reset = current_time
				- elapsed_time_motor->time_of_last_reset;

		if ( requested_time < time_since_reset ) {
			mx_warning(
"%s: Attempted to wait until elapsed time %g when that time "
"had already passed.  Current time since reset = %g",
				fname, requested_time, time_since_reset );
		}

		while ( requested_time > time_since_reset ) {

			current_time = mxd_elapsed_time_get_current_time();

			time_since_reset = current_time
				- elapsed_time_motor->time_of_last_reset;

			interrupt = mx_user_requested_interrupt_or_pause();

			if ( interrupt == MXF_USER_INT_ABORT ) {

				mx_status = mx_error(
					(MXE_INTERRUPTED | MXE_QUIET) ,fname,
			"Wait for time %g was interrupted due to user request.",
					requested_time );

				break;      /* Exit the while() loop. */
			}
			if ( interrupt == MXF_USER_INT_PAUSE ) {

				mx_status = mx_error(MXE_PAUSE_REQUESTED, fname,
					"Pause requested by user." );

				break;      /* Exit the while() loop. */
			}

			mx_msleep(10);
		}
	}
	motor->raw_position.analog = time_since_reset;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_elapsed_time_get_position( MX_MOTOR *motor )
{
	MX_ELAPSED_TIME_MOTOR *elapsed_time_motor;
	double current_time;

	elapsed_time_motor = (MX_ELAPSED_TIME_MOTOR *)
			(motor->record->record_type_struct);

	current_time = mxd_elapsed_time_get_current_time();

	current_time -= elapsed_time_motor->time_of_last_reset;

	motor->raw_position.analog = current_time;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_elapsed_time_set_position( MX_MOTOR *motor )
{
	MX_ELAPSED_TIME_MOTOR *elapsed_time_motor;
	double set_position;

	elapsed_time_motor = (MX_ELAPSED_TIME_MOTOR *)
			(motor->record->record_type_struct);

	set_position = motor->raw_set_position.analog;

	elapsed_time_motor->time_of_last_reset += set_position;

	motor->raw_position.analog = set_position;

	return MX_SUCCESSFUL_RESULT;
}

