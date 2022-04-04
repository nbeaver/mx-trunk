/*
 * Name:    d_hrt_motor.c 
 *
 * Purpose: MX motor driver that can be used to scan using the MX
 *          high resolution timer as the independent variable.
 *
 *          An hrt_motor is not treated as a pseudomotor for two reasons:
 *          1.  There is no underlying real motor.
 *          2.  hrt_motor records can be used in quick scans as the motor
 *              record for 'mcs_time_mce' records.
 *              However, you probably should not use it for this since the
 *              CPU speed is not guaranteed to be constant on all platforms.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
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
#include "mx_process.h"
#include "mx_hrt.h"
#include "d_hrt_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_hrt_motor_record_function_list = {
	NULL,
	mxd_hrt_motor_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_hrt_motor_special_processing_setup
};

MX_MOTOR_FUNCTION_LIST mxd_hrt_motor_motor_function_list = {
	NULL,
	mxd_hrt_motor_move_absolute,
	mxd_hrt_motor_get_position,
	mxd_hrt_motor_set_position,
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

MX_RECORD_FIELD_DEFAULTS mxd_hrt_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_HRT_MOTOR_STANDARD_FIELDS
};

long mxd_hrt_motor_num_record_fields
		= sizeof( mxd_hrt_motor_record_field_defaults )
			/ sizeof( mxd_hrt_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_hrt_motor_rfield_def_ptr
			= &mxd_hrt_motor_record_field_defaults[0];

/*---*/

static mx_status_type mxd_hrt_motor_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*---*/

MX_EXPORT mx_status_type
mxd_hrt_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_hrt_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_HRT_MOTOR *hrt_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	hrt_motor = (MX_HRT_MOTOR *) malloc( sizeof(MX_HRT_MOTOR) );

	if ( hrt_motor == (MX_HRT_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HRT_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = hrt_motor;
	record->class_specific_function_list
				= &mxd_hrt_motor_motor_function_list;

	motor->record = record;

	/* An elapsed time motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hrt_motor_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_HRT_MTR_CPU_SPEED:
			record_field->process_function
				= mxd_hrt_motor_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static double
mxd_hrt_motor_get_current_time( void )
{
	struct timespec current_hrt_time;
	double current_time;

	current_hrt_time = mx_high_resolution_time();

	current_time =
	    mx_convert_high_resolution_time_to_seconds( current_hrt_time );

	return current_time;
}

MX_EXPORT mx_status_type
mxd_hrt_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_hrt_motor_move_absolute()";

	MX_HRT_MOTOR *hrt_motor;
	double current_time, requested_time, time_since_reset;
	int interrupt;
	mx_status_type mx_status;

	hrt_motor = (MX_HRT_MOTOR *)
			motor->record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	current_time = mxd_hrt_motor_get_current_time();

	/* Use of a requested position that is less than or equal to zero
	 * is interpreted as request to reset the 'time_of_last_reset' field.
	 *
	 * Otherwise, we wait until the requested time has passed, unless
	 * the user interrupts us.
	 */

	requested_time = motor->raw_destination.analog;

	if ( requested_time <= 0.0 ) {
		hrt_motor->time_of_last_reset = current_time;

		time_since_reset = 0.0;
	} else {
		time_since_reset = current_time
				- hrt_motor->time_of_last_reset;

		if ( requested_time < time_since_reset ) {
			mx_warning(
"%s: Attempted to wait until elapsed time %g when that time "
"had already passed.  Current time since reset = %g",
				fname, requested_time, time_since_reset );
		}

		while ( requested_time > time_since_reset ) {

			current_time = mxd_hrt_motor_get_current_time();

			time_since_reset = current_time
				- hrt_motor->time_of_last_reset;

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
mxd_hrt_motor_get_position( MX_MOTOR *motor )
{
	MX_HRT_MOTOR *hrt_motor;
	double current_time;

	hrt_motor = (MX_HRT_MOTOR *)
			(motor->record->record_type_struct);

	current_time = mxd_hrt_motor_get_current_time();

	current_time -= hrt_motor->time_of_last_reset;

	motor->raw_position.analog = current_time;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hrt_motor_set_position( MX_MOTOR *motor )
{
	MX_HRT_MOTOR *hrt_motor;
	double set_position;

	hrt_motor = (MX_HRT_MOTOR *)
			(motor->record->record_type_struct);

	set_position = motor->raw_set_position.analog;

	hrt_motor->time_of_last_reset += set_position;

	motor->raw_position.analog = set_position;

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_hrt_motor_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxd_hrt_motor_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_HRT_MOTOR *hrt_motor;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	hrt_motor = (MX_HRT_MOTOR *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_HRT_MTR_CPU_SPEED:
			/* Normally one would not call the function
			 * mx_high_resolution_time_init() at any point
			 * other than startup time.  But, the hrt_motor
			 * driver is partially for debugging purposes,
			 * so we make an exception.
			 */
			mx_high_resolution_time_init();

			hrt_motor->cpu_speed =
				mx_get_hrt_counter_ticks_per_second();
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
		case MXLV_HRT_MTR_CPU_SPEED:
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return mx_status;
}

