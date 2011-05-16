/*
 * Name:    d_disabled_motor.c 
 *
 * Purpose: MX motor driver for a disabled motor.  It is simpler than a
 *          'soft_motor' record in that 'soft_motor' records have simulated
 *          speeds and accelerations, while 'disabled_motor' records arrive
 *          at their destination immediately.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2010-2011 Illinois Institute of Technology
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
#include "d_disabled_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_disabled_motor_record_function_list = {
	NULL,
	mxd_disabled_motor_create_record_structures,
	mxd_disabled_motor_finish_record_initialization,
	NULL,
	mxd_disabled_motor_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_disabled_motor_motor_function_list = {
	mxd_disabled_motor_motor_is_busy,
	mxd_disabled_motor_move_absolute,
	mxd_disabled_motor_get_position,
	mxd_disabled_motor_set_position,
	mxd_disabled_motor_disabled_abort,
	mxd_disabled_motor_immediate_abort,
	mxd_disabled_motor_positive_limit_hit,
	mxd_disabled_motor_negative_limit_hit,
	mxd_disabled_motor_find_home_position,
	mxd_disabled_motor_constant_velocity_move,
	mxd_disabled_motor_get_parameter,
	mxd_disabled_motor_set_parameter
};

/* Disabled motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_disabled_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS
};

long mxd_disabled_motor_num_record_fields
		= sizeof( mxd_disabled_motor_record_field_defaults )
			/ sizeof( mxd_disabled_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_disabled_motor_rfield_def_ptr
			= &mxd_disabled_motor_record_field_defaults[0];

/* === */

MX_EXPORT mx_status_type
mxd_disabled_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_disabled_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_DISABLED_MOTOR *disabled_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	disabled_motor = (MX_DISABLED_MOTOR *)
					malloc( sizeof(MX_DISABLED_MOTOR) );

	if ( disabled_motor == (MX_DISABLED_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DISABLED_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = disabled_motor;
	record->class_specific_function_list
				= &mxd_disabled_motor_motor_function_list;

	motor->record = record;

	/* A disabled motor is treated as a stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_disabled_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for disabled motor record '%s' is NULL.",
			record->name );
	}

	/* Set some values for motor speed and acceleration. */

	motor->speed = 1.0;
	motor->base_speed = 0.0;
	motor->maximum_speed = 10.0;

	motor->raw_speed = mx_divide_safely( motor->speed, motor->scale );

	motor->raw_base_speed = mx_divide_safely( motor->base_speed,
							motor->scale );

	motor->raw_maximum_speed = mx_divide_safely( motor->maximum_speed,
							motor->scale );

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	motor->raw_acceleration_parameters[0] = 0.5;	/* units/sec**2 */

	motor->raw_acceleration_parameters[1] = 0.0;
	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] =
		"mxd_disabled_motor_print_motor_structure()";

	MX_MOTOR *motor;
	double position, move_deadband;
	mx_status_type status;

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
	fprintf(file, "  Motor type     = DISABLED_MOTOR.\n\n");

	fprintf(file, "  name           = %s\n", record->name);

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS )
		return status;

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
mxd_disabled_motor_motor_is_busy( MX_MOTOR *motor )
{
	motor->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_move_absolute( MX_MOTOR *motor )
{
	motor->raw_position.stepper = motor->raw_destination.stepper;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_get_position( MX_MOTOR *motor )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_set_position( MX_MOTOR *motor )
{
	motor->raw_position.stepper = motor->raw_set_position.stepper;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_disabled_abort( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_immediate_abort( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_positive_limit_hit( MX_MOTOR *motor )
{
	motor->positive_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_negative_limit_hit( MX_MOTOR *motor )
{
	motor->negative_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_find_home_position( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_constant_velocity_move( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_disabled_motor_get_parameter( MX_MOTOR *motor )
{
	return mx_motor_default_get_parameter_handler( motor );
}

MX_EXPORT mx_status_type
mxd_disabled_motor_set_parameter( MX_MOTOR *motor )
{
	return mx_motor_default_set_parameter_handler( motor );
}

