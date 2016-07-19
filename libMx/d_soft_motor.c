/*
 * Name:    d_soft_motor.c 
 *
 * Purpose: MX motor driver for software emulated stepping motor.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2004, 2006-2007, 2009, 2011, 2013, 2015-2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define SOFT_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_dead_reckoning.h"
#include "d_soft_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_motor_record_function_list = {
	NULL,
	mxd_soft_motor_create_record_structures,
	mxd_soft_motor_finish_record_initialization,
	NULL,
	mxd_soft_motor_print_motor_structure,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_soft_motor_special_processing_setup
};

MX_MOTOR_FUNCTION_LIST mxd_soft_motor_motor_function_list = {
	NULL,
	mxd_soft_motor_move_absolute,
	mxd_soft_motor_get_position,
	mxd_soft_motor_set_position,
	mxd_soft_motor_immediate_abort,
	mxd_soft_motor_immediate_abort,
	NULL,
	NULL,
	mxd_soft_motor_raw_home_command,
	mxd_soft_motor_constant_velocity_move,
	mxd_soft_motor_get_parameter,
	mxd_soft_motor_set_parameter,
	mxd_soft_motor_simultaneous_start,
	mxd_soft_motor_get_status,
	NULL,
	NULL,
	mxd_soft_motor_setup_triggered_move,
	mxd_soft_motor_trigger_move
};

/* Soft motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_soft_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SOFT_MOTOR_STANDARD_FIELDS
};

long mxd_soft_motor_num_record_fields
		= sizeof( mxd_soft_motor_record_field_defaults )
			/ sizeof( mxd_soft_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_motor_rfield_def_ptr
			= &mxd_soft_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_soft_motor_get_pointers( MX_MOTOR *motor,
			MX_SOFT_MOTOR **soft_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( soft_motor == (MX_SOFT_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOFT_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*soft_motor = (MX_SOFT_MOTOR *) (motor->record->record_type_struct);

	if ( *soft_motor == (MX_SOFT_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

static mx_status_type mxd_soft_motor_process_function( void *record_ptr,
							void *record_field_ptr,
							int operation );

/* === */

MX_EXPORT mx_status_type
mxd_soft_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_soft_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_SOFT_MOTOR *soft_motor;
	mx_status_type mx_status;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) calloc( 1, sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	soft_motor = (MX_SOFT_MOTOR *) calloc( 1, sizeof(MX_SOFT_MOTOR) );

	if ( soft_motor == (MX_SOFT_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = soft_motor;
	record->class_specific_function_list
				= &mxd_soft_motor_motor_function_list;

	motor->record = record;

	/* A soft motor is treated as a stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

#if 0
	/* Only turn this on if you are using the soft_motor driver to 
	 * debug the handling of queued events.
	 */

	record->event_time_manager = (MX_EVENT_TIME_MANAGER *)
		malloc( sizeof( MX_EVENT_TIME_MANAGER ) );

	if ( record->event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
  "Ran out of memory trying to allocate an MX_EVENT_TIME_MANAGER structure." );
	}

	/* This function call has the effect of forcing the MX server to
	 * only send commands to the soft_motor record at the rate of
	 * once every 5 seconds or more.
	 */

	mx_status = mx_set_event_interval( record, 5.0 );
#else
	mx_status = MX_SUCCESSFUL_RESULT;
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_soft_motor_finish_record_initialization()";

	MX_MOTOR *motor = NULL;
	MX_SOFT_MOTOR *soft_motor = NULL;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_soft_motor_get_pointers( motor, &soft_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Setup the motor speeds and acceleration. */

	motor->raw_speed         = soft_motor->default_speed;
	motor->raw_base_speed    = soft_motor->default_base_speed;
	motor->raw_maximum_speed = 1000.0 * motor->raw_speed;

	motor->speed         = motor->raw_speed * motor->scale;
	motor->base_speed    = motor->raw_base_speed * motor->scale;
	motor->maximum_speed = motor->raw_maximum_speed * motor->scale;

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	/* Acceleration in steps/sec**2 */

	motor->raw_acceleration_parameters[0]
			= soft_motor->default_acceleration;

	motor->raw_acceleration_parameters[1] = 0.0;
	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	/* Initialize home position. */

	soft_motor->simulated_home_position = 0.0;

	soft_motor->home_search_succeeded = FALSE;

	soft_motor->simulated_negative_limit = -DBL_MAX;
	soft_motor->simulated_positive_limit = DBL_MAX;

	/* Initialize the dead reckoning structure. */

	mx_status = mx_dead_reckoning_initialize(
				&(soft_motor->dead_reckoning), motor );

#if SOFT_MOTOR_DEBUG
	MX_DEBUG(-2,("%s complete for motor '%s'", fname, record->name));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_motor_print_motor_structure()";

	MX_MOTOR *motor;
	double position, move_deadband;
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

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type     = SOFT_MOTOR.\n\n");

	fprintf(file, "  name           = %s\n", record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  position       = %.*g %s (%ld steps)\n",
		record->precision,
		motor->position, motor->units,
		motor->raw_position.stepper );

	fprintf(file, "  scale          = %.*g %s per step.\n",
		record->precision,
		motor->scale, motor->units );

	fprintf(file, "  offset         = %.*g %s.\n",
		record->precision,
		motor->offset, motor->units );

	fprintf(file, "  backlash       = %.*g %s (%ld steps)\n",
		record->precision,
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );

	fprintf(file, "  negative_limit = %.*g %s (%ld steps)\n",
		record->precision,
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive_limit = %.*g %s (%ld steps)\n",
		record->precision,
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband  = %.*g %s (%ld steps)\n\n",
		record->precision,
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_motor_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_SOFT_MOTOR_RAW_SIMULATED_HOME_POSITION:
		case MXLV_SOFT_MOTOR_SIMULATED_HOME_POSITION:
			record_field->process_function
					= mxd_soft_motor_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_soft_motor_move_absolute()";

	MX_SOFT_MOTOR *soft_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_motor_get_pointers( motor, &soft_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if SOFT_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: motor '%s', destination = %g, raw_destination = %ld",
		fname, motor->record->name,
		motor->destination,
		motor->raw_destination.stepper));
#endif

	mx_status = mx_dead_reckoning_start_motion(
					&(soft_motor->dead_reckoning),
					motor->position,
					motor->destination,
					NULL );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_soft_motor_get_position()";

	MX_SOFT_MOTOR *soft_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_motor_get_pointers( motor, &soft_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->busy ) {
		mx_status = mx_dead_reckoning_predict_motion(
					&(soft_motor->dead_reckoning),
					NULL, NULL );
	}

#if SOFT_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: motor '%s', position = %g, raw_position = %ld",
		fname, motor->record->name,
		motor->position,
		motor->raw_position.stepper));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_motor_set_position( MX_MOTOR *motor )
{
	motor->raw_position.stepper = motor->raw_set_position.stepper;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_soft_motor_immediate_abort()";

	MX_SOFT_MOTOR *soft_motor;
	mx_status_type mx_status;

	soft_motor = NULL;

	mx_status = mxd_soft_motor_get_pointers( motor, &soft_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if SOFT_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: motor '%s', immediate abort invoked.",
		fname, motor->record->name));
#endif

	if ( motor->busy ) {
		mx_status = mx_dead_reckoning_abort_motion(
					&(soft_motor->dead_reckoning) );
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_soft_motor_raw_home_command()";

	MX_SOFT_MOTOR *soft_motor = NULL;
	double destination;
	mx_status_type mx_status;

	mx_status = mxd_soft_motor_get_pointers( motor, &soft_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->limit_switch_as_home_switch > 0 ) {
		destination = soft_motor->simulated_positive_limit;
	} else
	if ( motor->limit_switch_as_home_switch == 0 ) {
		destination = soft_motor->simulated_home_position;
	} else {
		destination = soft_motor->simulated_negative_limit;
	}

	mx_status = mx_motor_move_absolute( motor->record, destination, 0 );

	motor->home_search_in_progress = TRUE;

	soft_motor->home_search_succeeded = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_motor_constant_velocity_move( MX_MOTOR *motor )
{
	double destination;
	mx_status_type mx_status;

	if ( motor->constant_velocity_move >= 0 ) {
		destination = motor->positive_limit;
	} else {
		destination = motor->negative_limit;
	}

	mx_status = mx_motor_move_absolute( motor->record, destination, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_motor_get_parameter( MX_MOTOR *motor )
{
	return mx_motor_default_get_parameter_handler( motor );
}

MX_EXPORT mx_status_type
mxd_soft_motor_set_parameter( MX_MOTOR *motor )
{
	return mx_motor_default_set_parameter_handler( motor );
}

MX_EXPORT mx_status_type
mxd_soft_motor_simultaneous_start( long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				unsigned long flags )
{
	static const char fname[] = "mxd_soft_motor_simultaneous_start()";

	MX_MOTOR *motor;
	MX_SOFT_MOTOR *soft_motor;
	MX_CLOCK_TICK start_time;
	int i;
	mx_status_type mx_status;

	if ( num_motor_records <= 0 )
		return MX_SUCCESSFUL_RESULT;

	/* Check to see if all of the motors are soft motors. */

	for ( i = 0; i < num_motor_records; i++ ) {
		if ( motor_record_array[i]->mx_type != MXT_MTR_SOFTWARE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same kind of motor.",
				motor_record_array[0]->name,
				motor_record_array[i]->name );
		}
	}

	/* Use the current_time as the start time. */

	start_time = mx_current_clock_tick();

	/* Start the dead reckoning timers for all of the motors. */

	for ( i = 0; i < num_motor_records; i++ ) {

		motor = (MX_MOTOR *) motor_record_array[i]->record_class_struct;

		if ( motor == (MX_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for record '%s' is NULL.",
				motor_record_array[i]->name );
		}

		soft_motor = (MX_SOFT_MOTOR *)
				motor_record_array[i]->record_type_struct;

		if ( soft_motor == (MX_SOFT_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SOFT_MOTOR pointer for record '%s' is NULL.",
				motor_record_array[i]->name );
		}

		mx_status = mx_dead_reckoning_start_motion(
						&(soft_motor->dead_reckoning),
						motor->position,
						position_array[i],
						&start_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_soft_motor_get_status()";

	MX_SOFT_MOTOR *soft_motor;
	mx_bool_type at_home_switch;
	mx_status_type mx_status;

	soft_motor = NULL;

	mx_status = mxd_soft_motor_get_pointers( motor, &soft_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	if ( motor->busy ) {
		mx_status = mx_dead_reckoning_predict_motion(
					&(soft_motor->dead_reckoning),
					NULL, NULL );
	}

	if ( motor->busy ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	if ( motor->raw_position.stepper ==
		soft_motor->raw_simulated_home_position )
	{
		at_home_switch = TRUE;
	} else {
		at_home_switch = FALSE;
	}

	if ( motor->home_search_in_progress ) {
		if ( motor->busy == FALSE ) {
			motor->home_search_in_progress = FALSE;

			if ( at_home_switch ) {
				soft_motor->home_search_succeeded = TRUE;
			} else {
				soft_motor->home_search_succeeded = FALSE;
			}
		}
	}

	if ( at_home_switch ) {
		motor->status |= MXSF_MTR_AT_HOME_SWITCH;
	}

	if ( soft_motor->home_search_succeeded ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	if ( motor->position <= soft_motor->simulated_negative_limit ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}

	if ( motor->position >= soft_motor->simulated_positive_limit ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}

#if SOFT_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: motor '%s', busy = %d, status = %#lx",
		fname, motor->record->name, motor->busy, motor->status));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_motor_setup_triggered_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_soft_motor_setup_triggered_move()";

	/* This function _must_ exist for triggered moves to work,
	 * but the function does not need to actually do anything,
	 * since the destination value is already located in the
	 * motor->triggered_move_destination variable.
	 */

	MX_DEBUG(-2,("%s invoked for motor '%s', destination = %g",
		fname, motor->record->name, motor->triggered_move_destination));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_motor_trigger_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_soft_motor_trigger_move()";

	long mask;
	mx_status_type mx_status;

	MX_DEBUG(-2,
	("%s invoked for motor '%s', trigger_mode = %ld, destination = %g",
		fname, motor->record->name, motor->trigger_mode,
		motor->triggered_move_destination));

	mx_status = MX_SUCCESSFUL_RESULT;

	mask = ( MXF_MTR_INTERNAL_TRIGGER | MXF_MTR_EXTERNAL_TRIGGER );

	if( motor->trigger_mode == MXF_MTR_NO_TRIGGER ) {
		/* Ignore the trigger request. */
	} else
	if ( motor->trigger_mode & mask ) {
		/* This simulation driver treats both external and internal
		 * triggers the same, since there cannot be a _real_
		 * external trigger.
		 */

		mx_status = mx_motor_move_absolute( motor->record,
					motor->triggered_move_destination, 0 );
	} else {
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized trigger mode %#lx specified for motor '%s'.",
			motor->trigger_mode, motor->record->name );
	}

	return mx_status;
}

/*=== */

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxd_soft_motor_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxd_soft_motor_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MOTOR *motor;
	MX_SOFT_MOTOR *soft_motor;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	motor = (MX_MOTOR *) record->record_class_struct;
	soft_motor = (MX_SOFT_MOTOR *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_SOFT_MOTOR_SIMULATED_HOME_POSITION:
			soft_motor->simulated_home_position = motor->offset
		  + motor->scale * soft_motor->raw_simulated_home_position;
			break;

		case MXLV_SOFT_MOTOR_RAW_SIMULATED_HOME_POSITION:
			break;

		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_SOFT_MOTOR_SIMULATED_HOME_POSITION:
			soft_motor->raw_simulated_home_position =
			    mx_divide_safely(
			    soft_motor->simulated_home_position - motor->offset,
					motor->scale );
			break;

		case MXLV_SOFT_MOTOR_RAW_SIMULATED_HOME_POSITION:
			soft_motor->simulated_home_position = motor->offset
		  + motor->scale * soft_motor->raw_simulated_home_position;
			break;

		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

