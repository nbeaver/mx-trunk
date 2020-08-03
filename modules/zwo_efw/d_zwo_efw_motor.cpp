/*
 * Name:    d_zwo_efw_motor.c
 *
 * Purpose: MX driver for ZWO Electronic Filter Wheels treated as motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ZWO_EFW_MOTOR_DEBUG			TRUE
#define MXD_ZWO_EFW_MOTOR_DEBUG_PARAMETERS	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "EFW_filter.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "d_zwo_efw_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_zwo_efw_motor_record_function_list = {
	NULL,
	mxd_zwo_efw_motor_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_zwo_efw_motor_open,
};

MX_MOTOR_FUNCTION_LIST mxd_zwo_efw_motor_motor_function_list = {
	NULL,
	mxd_zwo_efw_motor_move_absolute,
	mxd_zwo_efw_motor_get_position,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_zwo_efw_motor_get_parameter,
	mxd_zwo_efw_motor_set_parameter,
	NULL,
	NULL,
	mxd_zwo_efw_motor_get_extended_status
};

/* ZWO_EFW_MOTOR motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_zwo_efw_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_ZWO_EFW_MOTOR_STANDARD_FIELDS
};

long mxd_zwo_efw_motor_num_record_fields
		= sizeof( mxd_zwo_efw_motor_record_field_defaults )
			/ sizeof( mxd_zwo_efw_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_zwo_efw_motor_rfield_def_ptr
			= &mxd_zwo_efw_motor_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_zwo_efw_motor_get_pointers( MX_MOTOR *motor,
			MX_ZWO_EFW_MOTOR **zwo_efw_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_zwo_efw_motor_get_pointers()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor_ptr;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	zwo_efw_motor_ptr = (MX_ZWO_EFW_MOTOR *)
				motor->record->record_type_struct;

	if ( zwo_efw_motor_ptr == (MX_ZWO_EFW_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ZWO_EFW_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( zwo_efw_motor != (MX_ZWO_EFW_MOTOR **) NULL ) {
		*zwo_efw_motor = zwo_efw_motor_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_zwo_efw_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_ZWO_EFW_MOTOR *zwo_efw_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	zwo_efw_motor = (MX_ZWO_EFW_MOTOR *) malloc( sizeof(MX_ZWO_EFW_MOTOR) );

	if ( zwo_efw_motor == (MX_ZWO_EFW_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ZWO_EFW_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = zwo_efw_motor;
	record->class_specific_function_list =
				&mxd_zwo_efw_motor_motor_function_list;

	motor->record = record;
	zwo_efw_motor->record = record;

	/* A ZWO Electronic Filter Wheel is treated as a stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* Set the (unused) acceleration type to the rate in counts/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_zwo_efw_motor_open()";

	MX_MOTOR *motor = NULL;
	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
						&zwo_efw_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: EFW SDK version = '%s'.", fname, EFWGetSDKVersion()));

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_zwo_efw_motor_move_absolute()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
						&zwo_efw_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_zwo_efw_motor_get_position()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
						&zwo_efw_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_zwo_efw_motor_get_parameter()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
						&zwo_efw_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ZWO_EFW_MOTOR_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_zwo_efw_motor_set_parameter()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
						&zwo_efw_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_ZWO_EFW_MOTOR_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_zwo_efw_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_zwo_efw_motor_get_extended_status()";

	MX_ZWO_EFW_MOTOR *zwo_efw_motor = NULL;
	mx_status_type mx_status;

	mx_status = mxd_zwo_efw_motor_get_pointers( motor,
						&zwo_efw_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	return MX_SUCCESSFUL_RESULT;
}

