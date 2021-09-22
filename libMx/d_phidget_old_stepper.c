/*
 * Name:    d_phidget_old_stepper.c 
 *
 * Purpose: MX motor driver to control the old non-HID Phidget stepper motor
 *          controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2008, 2010, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PHIDGET_OLD_STEPPER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_clock.h"
#include "mx_motor.h"
#include "mx_usb.h"
#include "i_phidget_old_stepper.h"
#include "d_phidget_old_stepper.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_phidget_old_stepper_record_function_list = {
	NULL,
	mxd_phidget_old_stepper_create_record_structures,
	mxd_phidget_old_stepper_finish_record_initialization,
	NULL,
	mxd_phidget_old_stepper_print_structure,
	mxd_phidget_old_stepper_open,
	NULL,
	NULL,
	mxd_phidget_old_stepper_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_phidget_old_stepper_motor_function_list = {
	NULL,
	mxd_phidget_old_stepper_move_absolute,
	NULL,
	NULL,
	mxd_phidget_old_stepper_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_phidget_old_stepper_get_parameter,
	mx_motor_default_set_parameter_handler,
	NULL,
	NULL,
	mxd_phidget_old_stepper_get_extended_status
};

/* Pontech PHIDGET_OLD_STEPPER motor controller data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_phidget_old_stepper_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PHIDGET_OLD_STEPPER_STANDARD_FIELDS
};

long mxd_phidget_old_stepper_num_record_fields
		= sizeof( mxd_phidget_old_stepper_rf_defaults )
			/ sizeof( mxd_phidget_old_stepper_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_phidget_old_stepper_rfield_def_ptr
			= &mxd_phidget_old_stepper_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_phidget_old_stepper_get_pointers( MX_MOTOR *motor,
			MX_PHIDGET_OLD_STEPPER **phidget_old_stepper,
	MX_PHIDGET_OLD_STEPPER_CONTROLLER **phidget_old_stepper_controller,
			const char *calling_fname )
{
	static const char fname[] = "mxd_phidget_old_stepper_get_pointers()";

	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper_ptr;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	phidget_old_stepper_ptr = (MX_PHIDGET_OLD_STEPPER *)
					motor->record->record_type_struct;

	if ( phidget_old_stepper_ptr == (MX_PHIDGET_OLD_STEPPER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PHIDGET_OLD_STEPPER pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( phidget_old_stepper != (MX_PHIDGET_OLD_STEPPER **) NULL ) {
		*phidget_old_stepper = phidget_old_stepper_ptr;
	}

	if ( phidget_old_stepper_controller !=
			(MX_PHIDGET_OLD_STEPPER_CONTROLLER **) NULL )
	{
		if ( phidget_old_stepper_ptr->controller_record ==
				(MX_RECORD *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The controller_record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		*phidget_old_stepper_controller =
			(MX_PHIDGET_OLD_STEPPER_CONTROLLER *)
		phidget_old_stepper_ptr->controller_record->record_type_struct;

		if ( (*phidget_old_stepper_controller) ==
			(MX_PHIDGET_OLD_STEPPER_CONTROLLER *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PHIDGET_OLD_STEPPER_CONTROLLER pointer for record '%s' "
		"used by record '%s' is NULL.",
			    phidget_old_stepper_ptr->controller_record->name,
			    motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_phidget_old_stepper_create_record_structures()";

	MX_MOTOR *motor;
	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	phidget_old_stepper = (MX_PHIDGET_OLD_STEPPER *)
				malloc( sizeof(MX_PHIDGET_OLD_STEPPER) );

	if ( phidget_old_stepper == (MX_PHIDGET_OLD_STEPPER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PHIDGET_OLD_STEPPER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = phidget_old_stepper;
	record->class_specific_function_list
				= &mxd_phidget_old_stepper_motor_function_list;

	motor->record = record;
	phidget_old_stepper->record = record;

	/* A PHIDGET_OLD_STEPPER motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* This driver stores the acceleration time. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	motor->raw_speed = 0.0;
	motor->raw_base_speed = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_finish_record_initialization( MX_RECORD *record )
{
	return mx_motor_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_phidget_old_stepper_print_structure()";

	MX_MOTOR *motor;
	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper = NULL;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_phidget_old_stepper_get_pointers( motor,
					&phidget_old_stepper, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = PHIDGET_OLD_STEPPER.\n\n");
	fprintf(file, "  name               = %s\n", record->name);

	fprintf(file, "  board number       = %ld\n",
				phidget_old_stepper->motor_number);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position           = %g %s  (%ld steps)\n",
		motor->position, motor->units,
		motor->raw_position.stepper );
	fprintf(file, "  scale              = %g %s per step.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
		motor->offset, motor->units);

	fprintf(file, "  backlash           = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper);

	fprintf(file, "  negative limit     = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit     = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband      = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_phidget_old_stepper_open()";

	MX_MOTOR *motor;
	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper = NULL;
	double raw_acceleration_parameters[ MX_MOTOR_NUM_ACCELERATION_PARAMS ];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_phidget_old_stepper_get_pointers( motor,
					&phidget_old_stepper, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_phidget_old_stepper_resynchronize( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the speed, base speed, and acceleration. */

	motor->raw_speed = fabs( phidget_old_stepper->default_speed );
	motor->speed = motor->raw_speed * fabs( motor->scale );

	motor->raw_base_speed = fabs( phidget_old_stepper->default_base_speed );
	motor->base_speed = motor->raw_base_speed * fabs( motor->scale );

	raw_acceleration_parameters[0] =
			phidget_old_stepper->default_acceleration;

	raw_acceleration_parameters[1] = 0.0;
	raw_acceleration_parameters[2] = 0.0;
	raw_acceleration_parameters[3] = 0.0;

	mx_status = mx_motor_set_raw_acceleration_parameters( record,
						raw_acceleration_parameters );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_phidget_old_stepper_resynchronize()";

	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	phidget_old_stepper = (MX_PHIDGET_OLD_STEPPER *)
					record->record_type_struct;

	if ( phidget_old_stepper == (MX_PHIDGET_OLD_STEPPER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PHIDGET_OLD_STEPPER pointer for motor '%s' is NULL.",
			record->name );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_phidget_old_stepper_move_absolute()";

	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper = NULL;
	MX_PHIDGET_OLD_STEPPER_CONTROLLER
				*phidget_old_stepper_controller = NULL;
	long destination;
	unsigned long speed, acceleration;
	char buffer[100];
	int num_bytes_written;
	mx_status_type mx_status;

	mx_status = mxd_phidget_old_stepper_get_pointers( motor,
					&phidget_old_stepper,
					&phidget_old_stepper_controller,
					fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	destination  = motor->raw_destination.stepper;
	speed        = mx_round( motor->raw_speed );
	acceleration = mx_round( motor->raw_acceleration_parameters[0] );

	buffer[0] = (unsigned char) phidget_old_stepper->motor_number;
	buffer[1] = 0;
	buffer[2] = (unsigned char) ( acceleration & 0xff );
	buffer[3] = (unsigned char) ( (acceleration >> 8) & 0xff );

	buffer[8] = (unsigned char) ( destination & 0xff );
	buffer[9] = (unsigned char) ( (destination >> 8) & 0xff );
	buffer[10] = (unsigned char) ( (destination >> 16) & 0xff );
	buffer[11] = (unsigned char) ( (destination >> 24) & 0xff );
	buffer[12] = (unsigned char) ( speed & 0xff );
	buffer[13] = (unsigned char) ( (speed >> 8) & 0xff );

#if PHIDGET_OLD_STEPPER_DEBUG

	{
		int i;

		fprintf( stderr, "%s:\n write  ", fname );

		for ( i = 0; i < 16; i++ ) {
			fprintf( stderr, " %02x", buffer[i] );
		}

		fprintf( stderr, "\n" );
	}

#endif /* PHIDGET_OLD_STEPPER_DEBUG */

	mx_status = mx_usb_bulk_write(
			phidget_old_stepper_controller->usb_device,
			MX_PHIDGET_OLD_STEPPER_WRITE_ENDPOINT,
			buffer, 16, &num_bytes_written, 1.0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_phidget_old_stepper_soft_abort()";

	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper = NULL;
	mx_status_type mx_status;

	mx_status = mxd_phidget_old_stepper_get_pointers( motor,
					&phidget_old_stepper, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current position of the motor. */

	mx_status = mxd_phidget_old_stepper_get_extended_status( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Command the motor to move to the current position. */

	motor->raw_destination.stepper = motor->raw_position.stepper;

	mx_status = mxd_phidget_old_stepper_move_absolute( motor );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_phidget_old_stepper_get_parameter()";

	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper;
	mx_status_type mx_status;

	phidget_old_stepper = NULL;

	mx_status = mxd_phidget_old_stepper_get_pointers( motor,
					&phidget_old_stepper, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_ACCELERATION_TYPE:
		motor->acceleration_type = MXF_MTR_ACCEL_TIME;
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_phidget_old_stepper_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_phidget_old_stepper_get_extended_status()";

	MX_PHIDGET_OLD_STEPPER *phidget_old_stepper = NULL;
	MX_PHIDGET_OLD_STEPPER_CONTROLLER
			*phidget_old_stepper_controller = NULL;

	long first_motor_position, second_motor_position;
	char buffer[100];
	int num_bytes_read, base;
	mx_status_type mx_status;

	mx_status = mxd_phidget_old_stepper_get_pointers( motor,
					&phidget_old_stepper,
					&phidget_old_stepper_controller,
					fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( phidget_old_stepper->motor_number ) {
	case 0:
		base = 0;
		break;
	case 1:
		base = 4;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal motor number %ld specified for motor '%s'",
			phidget_old_stepper->motor_number,
			motor->record->name );
	}

	/* Read from the controller twice in a row.  If the reported position
	 * on both reads is the same, we assume that the motor is not moving.
	 */

	/* Do the first read. */

	mx_status = mx_usb_bulk_read(
			phidget_old_stepper_controller->usb_device,
			MX_PHIDGET_OLD_STEPPER_READ_ENDPOINT,
			buffer, 16, &num_bytes_read, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if PHIDGET_OLD_STEPPER_DEBUG

	{
		int i;

		fprintf( stderr, "%s:\n read #1  ", fname );

		for ( i = 0; i < 16; i++ ) {
			fprintf( stderr, " %02x", buffer[i] );
		}

		fprintf( stderr, "\n" );
	}

#endif /* PHIDGET_OLD_STEPPER_DEBUG */

	first_motor_position = (long)( ((unsigned int) buffer[base+0])
				+ ( ((unsigned int) buffer[base+1]) << 8 )
				+ ( ((unsigned int) buffer[base+2]) << 16 )
				+ ( ((unsigned int) buffer[base+3]) << 24 ) );

	/* Wait a little while. */

	mx_msleep(10);

	/* Do the second read. */

	mx_status = mx_usb_bulk_read(
			phidget_old_stepper_controller->usb_device,
			MX_PHIDGET_OLD_STEPPER_READ_ENDPOINT,
			buffer, 16, &num_bytes_read, 1.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if PHIDGET_OLD_STEPPER_DEBUG

	{
		int i;

		fprintf( stderr, " read #2  " );

		for ( i = 0; i < 16; i++ ) {
			fprintf( stderr, " %02x", buffer[i] );
		}

		fprintf( stderr, "\n" );
	}

#endif /* PHIDGET_OLD_STEPPER_DEBUG */

	second_motor_position = (long)( ((unsigned int) buffer[base+0])
				+ ( ((unsigned int) buffer[base+1]) << 8 )
				+ ( ((unsigned int) buffer[base+2]) << 16 )
				+ ( ((unsigned int) buffer[base+3]) << 24 ) );

	/* See if the motor is moving. */

	if ( first_motor_position == second_motor_position ) {
		motor->status = 0;
	} else {
		motor->status = MXSF_MTR_IS_BUSY;
	}

	/* Save the second motor position. */

	motor->raw_position.stepper = second_motor_position;

	return MX_SUCCESSFUL_RESULT;
}

