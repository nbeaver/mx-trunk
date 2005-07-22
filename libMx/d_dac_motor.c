/*
 * Name:    d_dac_motor.c 
 *
 * Purpose: MX motor driver to control an MX analog output device as if
 *          it were a motor.  The most common use of this is to control
 *          piezoelectric motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003 Illinois Institute of Technology
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
#include "mx_analog_output.h"
#include "d_dac_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_dac_motor_record_function_list = {
	mxd_dac_motor_initialize_type,
	mxd_dac_motor_create_record_structures,
	mxd_dac_motor_finish_record_initialization,
	mxd_dac_motor_delete_record,
	mxd_dac_motor_print_motor_structure,
	mxd_dac_motor_read_parms_from_hardware,
	mxd_dac_motor_write_parms_to_hardware,
	mxd_dac_motor_open,
	mxd_dac_motor_close
};

MX_MOTOR_FUNCTION_LIST mxd_dac_motor_motor_function_list = {
	mxd_dac_motor_motor_is_busy,
	mxd_dac_motor_move_absolute,
	mxd_dac_motor_get_position,
	mxd_dac_motor_set_position,
	mxd_dac_motor_soft_abort,
	mxd_dac_motor_immediate_abort,
	mxd_dac_motor_positive_limit_hit,
	mxd_dac_motor_negative_limit_hit,
	mxd_dac_motor_find_home_position
};

/* DAC motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_dac_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_DAC_MOTOR_STANDARD_FIELDS
};

long mxd_dac_motor_num_record_fields
		= sizeof( mxd_dac_motor_record_field_defaults )
			/ sizeof( mxd_dac_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_dac_motor_rfield_def_ptr
			= &mxd_dac_motor_record_field_defaults[0];

/* === */

MX_EXPORT mx_status_type
mxd_dac_motor_initialize_type( long type )
{
		/* Nothing needed here. */

		return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_dac_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_DAC_MOTOR *dac_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	dac_motor = (MX_DAC_MOTOR *) malloc( sizeof(MX_DAC_MOTOR) );

	if ( dac_motor == (MX_DAC_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_DAC_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = dac_motor;
	record->class_specific_function_list
				= &mxd_dac_motor_motor_function_list;

	motor->record = record;

	/* A DAC motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	return status;
}

MX_EXPORT mx_status_type
mxd_dac_motor_delete_record( MX_RECORD *record )
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
mxd_dac_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_dac_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_DAC_MOTOR *dac_motor;
	MX_RECORD *dac_record;
	MX_ANALOG_OUTPUT *dac;
	double position, move_deadband;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	dac_motor = (MX_DAC_MOTOR *) (record->record_type_struct);

	if ( dac_motor == (MX_DAC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DAC_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	dac_record = dac_motor->dac_record;

	if ( dac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"DAC record pointer for record '%s' is NULL.",
			record->name );
	}

	dac = (MX_ANALOG_OUTPUT *) (dac_record->record_class_struct);

	if ( dac == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_ANALOG_OUTPUT pointer for record '%s' is NULL.",
			dac_record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type      = DAC_MOTOR.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  dac             = %s\n", dac_record->name);

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position        = %g %s  (%g).\n",
		motor->position, motor->units,
		motor->raw_position.analog );
	fprintf(file, "  scale           = %g %s per %s.\n",
		motor->scale, motor->units, dac->units);
	fprintf(file, "  offset          = %g %s.\n",
		motor->offset, motor->units);
        fprintf(file, "  backlash        = %g %s  (%g %s).\n",
                motor->backlash_correction, motor->units,
                motor->raw_backlash_correction.analog, dac->units);
        fprintf(file, "  negative limit  = %g %s  (%g %s).\n",
                motor->negative_limit, motor->units,
                motor->raw_negative_limit.analog, dac->units);
        fprintf(file, "  positive limit  = %g %s  (%g %s).\n",
                motor->positive_limit, motor->units,
                motor->raw_positive_limit.analog, dac->units);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband   = %g %s  (%g %s).\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog, dac->units);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_read_parms_from_hardware( MX_RECORD *record )
{
	/* All saving of parameters is handled by the MX_ANALOG_OUTPUT record
	 * this motor is derived from, so we need not do anything here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_write_parms_to_hardware( MX_RECORD *record )
{
	/* All setting of parameters is handled by the MX_ANALOG_OUTPUT record
	 * this motor is derived from, so we need not do anything here.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_open( MX_RECORD *record )
{
	/* Nothing to do. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_close( MX_RECORD *record )
{
	/* Nothing to do. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_motor_is_busy( MX_MOTOR *motor )
{
	/* A DAC motor is never busy. */

	motor->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_dac_motor_move_absolute()";

	MX_DAC_MOTOR *dac_motor;
	MX_RECORD *dac_record;
	mx_status_type status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	dac_motor = (MX_DAC_MOTOR *) (motor->record->record_type_struct);

	if ( dac_motor == (MX_DAC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DAC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	dac_record = dac_motor->dac_record;

	if ( dac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"DAC record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	status = mx_analog_output_write( dac_record,
					motor->raw_destination.analog );

	return status;
}

MX_EXPORT mx_status_type
mxd_dac_motor_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_dac_motor_get_position()";

	MX_DAC_MOTOR *dac_motor;
	MX_RECORD *dac_record;
	double present_value;
	mx_status_type status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	dac_motor = (MX_DAC_MOTOR *) (motor->record->record_type_struct);

	if ( dac_motor == (MX_DAC_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DAC_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	dac_record = dac_motor->dac_record;

	if ( dac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"DAC record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	status = mx_analog_output_read( dac_record, &present_value );

	if ( status.code != MXE_SUCCESS )
		return status;

	motor->raw_position.analog = present_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_dac_motor_set_position()";

	double negative_limit, positive_limit;
	double position, old_position, position_delta;
	mx_status_type status;

	/* The only plausible interpretation I can think of
	 * for this function here is to change the motor
	 * offset value.  We must check whether or not doing
	 * such a thing would cause the new value of the
	 * motor position to fall outside the software limits.
	 */

	position = motor->raw_set_position.analog;

	negative_limit = motor->raw_negative_limit.analog;
	positive_limit = motor->raw_positive_limit.analog;

	if ( position < negative_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Requested new set point %g would exceed the negative limit of %g",
			position, negative_limit );
	}
	if ( position < positive_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"Requested new set point %g would exceed the positive limit of %g",
			position, positive_limit );
	}

	status = mx_motor_get_position( motor->record, &old_position );

	if ( status.code != MXE_SUCCESS )
		return status;

	position_delta = position - old_position;

	motor->offset += position_delta;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_soft_abort( MX_MOTOR *motor )
{
	/* This routine does nothing. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_immediate_abort( MX_MOTOR *motor )
{
	/* This routine does nothing. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_positive_limit_hit( MX_MOTOR *motor )
{
	double present_position;
	mx_status_type status;

	status = mx_motor_get_position( motor->record, &present_position );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( present_position > motor->raw_positive_limit.analog ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_negative_limit_hit( MX_MOTOR *motor )
{
	double present_position;
	mx_status_type status;

	status = mx_motor_get_position( motor->record, &present_position );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( present_position < motor->raw_negative_limit.analog ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dac_motor_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_dac_motor_find_home_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"'find home position' is not valid for a DAC motor." );
}

