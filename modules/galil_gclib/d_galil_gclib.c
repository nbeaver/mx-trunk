/*
 * Name:    d_galil_gclib.c
 *
 * Purpose: MX driver for Galil-controlled motors via the gclib library.
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

#define MXD_GALIL_GCLIB_MOTOR_DEBUG				TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_motor.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_galil_gclib_record_function_list = {
	NULL,
	mxd_galil_gclib_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_galil_gclib_open,
	NULL,
	NULL,
	mxd_galil_gclib_resynchronize,
	mxd_galil_gclib_special_processing_setup
};

MX_MOTOR_FUNCTION_LIST mxd_galil_gclib_motor_function_list = {
	NULL,
	mxd_galil_gclib_move_absolute,
	mxd_galil_gclib_get_position,
	mxd_galil_gclib_set_position,
	mxd_galil_gclib_soft_abort,
	mxd_galil_gclib_immediate_abort,
	NULL,
	NULL,
	mxd_galil_gclib_raw_home_command,
	NULL,
	mxd_galil_gclib_get_parameter,
	mxd_galil_gclib_set_parameter,
	NULL,
	mxd_galil_gclib_get_status
};

/* GALIL_GCLIB_MOTOR motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_galil_gclib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_GALIL_GCLIB_MOTOR_STANDARD_FIELDS
};

long mxd_galil_gclib_num_record_fields
		= sizeof( mxd_galil_gclib_record_field_defaults )
			/ sizeof( mxd_galil_gclib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_rfield_def_ptr
			= &mxd_galil_gclib_record_field_defaults[0];

/*---*/

static mx_status_type mxd_galil_gclib_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );
/*---*/

static mx_status_type
mxd_galil_gclib_get_pointers( MX_MOTOR *motor,
			MX_GALIL_GCLIB_MOTOR **galil_gclib_motor,
			MX_GALIL_GCLIB **galil_gclib,
			const char *calling_fname )
{
	static const char fname[] = "mxd_galil_gclib_get_pointers()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor_ptr;
	MX_RECORD *galil_gclib_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	galil_gclib_motor_ptr = (MX_GALIL_GCLIB_MOTOR *)
				motor->record->record_type_struct;

	if ( galil_gclib_motor_ptr == (MX_GALIL_GCLIB_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_GALIL_GCLIB_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( galil_gclib_motor != (MX_GALIL_GCLIB_MOTOR **) NULL ) {
		*galil_gclib_motor = galil_gclib_motor_ptr;
	}

	if ( galil_gclib != (MX_GALIL_GCLIB **) NULL ) {
		galil_gclib_record = galil_gclib_motor_ptr->galil_gclib_record;

		if ( galil_gclib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The galil_gclib_record pointer for record '%s' "
			"is NULL.", motor->record->name );
		}

		*galil_gclib = (MX_GALIL_GCLIB *)
				galil_gclib_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_galil_gclib_create_record_structures()";

	MX_MOTOR *motor;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	galil_gclib_motor = (MX_GALIL_GCLIB_MOTOR *)
			malloc( sizeof(MX_GALIL_GCLIB_MOTOR) );

	if ( galil_gclib_motor == (MX_GALIL_GCLIB_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_GALIL_GCLIB_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = galil_gclib_motor;
	record->class_specific_function_list =
				&mxd_galil_gclib_motor_function_list;

	motor->record = record;
	galil_gclib_motor->record = record;

	/* A Galil gclib motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* The Galil motor reports acceleration in units/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_galil_gclib_open()";

	MX_MOTOR *motor = NULL;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_galil_gclib_resynchronize()";

	MX_MOTOR *motor = NULL;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case 0:
			record_field->process_function
					= mxd_galil_gclib_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_galil_gclib_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxd_galil_gclib_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MOTOR *motor;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor;
	MX_RECORD *galil_gclib_record;
	MX_GALIL_GCLIB *galil_gclib;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	motor = (MX_MOTOR *) record->record_class_struct;
	galil_gclib_motor = (MX_GALIL_GCLIB_MOTOR *) record->record_type_struct;
	galil_gclib_record = galil_gclib_motor->galil_gclib_record;
	galil_gclib = (MX_GALIL_GCLIB *) galil_gclib_record->record_type_struct;

	motor = motor;
	galil_gclib = galil_gclib;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case 0:
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
		case 0:
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

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_galil_gclib_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_move_absolute()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_get_position()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_set_position()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_soft_abort()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_immediate_abort()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_raw_home_command()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_get_parameter()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GALIL_GCLIB_MOTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		/* Acceleration parameter 0 = acceleration (units/sec**2)
		 * Acceleration parameter 1 = minimum jerk time (sec)
		 * Acceleration parameter 2 = maximum jerk time (sec)
		 */

		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_set_parameter()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GALIL_GCLIB_MOTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		/* Acceleration parameter 0 = acceleration (units/sec**2)
		 * Acceleration parameter 1 = minimum jerk time (sec)
		 * Acceleration parameter 2 = maximum jerk time (sec)
		 */

		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
		} else {
		}
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_get_status()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_get_pointers( motor, &galil_gclib_motor,
							&galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	return MX_SUCCESSFUL_RESULT;
}

