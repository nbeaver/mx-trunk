/*
 * Name:    d_galil_gclib_motor.c
 *
 * Purpose: MX driver for Galil-controlled motors via the gclib library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020-2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_GALIL_GCLIB_MOTOR_DEBUG		TRUE
#define MXD_GALIL_GCLIB_MOTOR_DEBUG_PARAMETERS	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_inttypes.h"
#include "mx_process.h"
#include "mx_motor.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_galil_gclib_motor_record_function_list = {
	NULL,
	mxd_galil_gclib_motor_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_galil_gclib_motor_open,
	NULL,
	NULL,
	mxd_galil_gclib_motor_resynchronize,
	mxd_galil_gclib_motor_special_processing_setup
};

MX_MOTOR_FUNCTION_LIST mxd_galil_gclib_motor_motor_function_list = {
	NULL,
	mxd_galil_gclib_motor_move_absolute,
	mxd_galil_gclib_motor_get_position,
	mxd_galil_gclib_motor_set_position,
	mxd_galil_gclib_motor_soft_abort,
	mxd_galil_gclib_motor_immediate_abort,
	NULL,
	NULL,
	mxd_galil_gclib_motor_raw_home_command,
	NULL,
	mxd_galil_gclib_motor_get_parameter,
	mxd_galil_gclib_motor_set_parameter,
	NULL,
	NULL,
	mxd_galil_gclib_motor_get_extended_status
};

/* GALIL_GCLIB_MOTOR motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_galil_gclib_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_GALIL_GCLIB_MOTOR_STANDARD_FIELDS
};

long mxd_galil_gclib_motor_num_record_fields
		= sizeof( mxd_galil_gclib_motor_record_field_defaults )
			/ sizeof( mxd_galil_gclib_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_galil_gclib_motor_rfield_def_ptr
			= &mxd_galil_gclib_motor_record_field_defaults[0];

/*---*/

static mx_status_type mxd_galil_gclib_motor_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );
/*---*/

static mx_status_type
mxd_galil_gclib_motor_get_pointers( MX_MOTOR *motor,
			MX_GALIL_GCLIB_MOTOR **galil_gclib_motor,
			MX_GALIL_GCLIB **galil_gclib,
			const char *calling_fname )
{
	static const char fname[] = "mxd_galil_gclib_motor_get_pointers()";

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
mxd_galil_gclib_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_galil_gclib_motor_create_record_structures()";

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
				&mxd_galil_gclib_motor_motor_function_list;

	motor->record = record;
	galil_gclib_motor->record = record;

	/* A Galil gclib motor is treated as a stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The Galil motor reports acceleration in counts/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_galil_gclib_motor_open()";

	MX_MOTOR *motor = NULL;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	unsigned long new_motor_index;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( islower( galil_gclib_motor->motor_name ) ) {
		galil_gclib_motor->motor_name =
			toupper( galil_gclib_motor->motor_name );
	}

	switch( galil_gclib_motor->motor_name ) {
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
		galil_gclib_motor->motor_type = MXT_GALIL_GCLIB_MOTOR_AXIS;
		break;
	case 'S':
	case 'T':
		galil_gclib_motor->motor_type =
			MXT_GALIL_GCLIB_MOTOR_COORDINATED;
		break;
	case 'I':
		return mx_error( MXE_UNSUPPORTED, fname,
		"The I block for Galil motor record '%s' cannot be used "
		"as a motor.", record->name );
		break;
	default:
		galil_gclib_motor->motor_type = MXT_GALIL_GCLIB_MOTOR_ILLEGAL;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal motor name '%c' requested for Galil motor '%s'.  "
		"The only supported motor names are "
		"A, B, C, D, E, F, G, H, S, or T.",
			galil_gclib_motor->motor_name, record->name );
		break;
	}

	/* Add the motor to the Galil controller's motor_record_array. */

	new_motor_index = galil_gclib->current_num_motors;

	if ( new_motor_index >= galil_gclib->maximum_num_motors ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"This motor '%s' would be motor number %lu for "
		"Galil controller '%s', but this Galil controller has "
		"a maximum of %lu motor axes and coordinated motions.",
			record->name, new_motor_index,
			galil_gclib->record->name,
			galil_gclib->maximum_num_motors );
	}

	galil_gclib->motor_record_array[ new_motor_index ] = record;

	(galil_gclib->current_num_motors)++;

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_galil_gclib_motor_resynchronize()";

	MX_MOTOR *motor = NULL;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_GALIL_GCLIB_MOTOR_SHOW_QR:
		case MXLV_GALIL_GCLIB_MOTOR_STOP_CODE:
			record_field->process_function
					= mxd_galil_gclib_motor_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static void
mxd_galil_gclib_motor_show_qr( MX_GALIL_GCLIB *galil_gclib,
			MX_GALIL_GCLIB_MOTOR *galil_gclib_motor )
{
	static uint8_t old_qr[400] = {0};
	static uint8_t qr[400];
	static unsigned long counter = 0;
	char command[40], response[40];
	int i;
	mx_status_type mx_status;

	memset( qr, 0, sizeof(qr) );

	snprintf( command, sizeof(command), "QR%c",
			galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib, command,
						(char *) qr, sizeof(qr) );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	if ( counter == 0 ) {
		counter++;
		return;
	}

	fprintf( stderr, "Counter = %lu\n", counter );

	mx_status = mxi_galil_gclib_command( galil_gclib, "QZ",
						response, sizeof(response) );

	for ( i = 0; i < sizeof(qr); i++ ) {
		if ( old_qr[i] != qr[i] ) {
			fprintf( stderr,
		"  old[%d] = %#" PRIx8 " ==>  new[%d] = %#" PRIx8 "\n",
				i, old_qr[i], i, qr[i] );
		}
	}

	memcpy( old_qr, qr, sizeof(qr) );

	counter++;
	return;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_galil_gclib_motor_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxd_galil_gclib_motor_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MOTOR *motor;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor;
	MX_RECORD *galil_gclib_record;
	MX_GALIL_GCLIB *galil_gclib;
	char command[80], response[80];
	int num_items;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	motor = (MX_MOTOR *) record->record_class_struct;
	galil_gclib_motor = (MX_GALIL_GCLIB_MOTOR *) record->record_type_struct;
	galil_gclib_record = galil_gclib_motor->galil_gclib_record;
	galil_gclib = (MX_GALIL_GCLIB *) galil_gclib_record->record_type_struct;

	MXW_UNUSED( motor );
	MXW_UNUSED( galil_gclib );

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_GALIL_GCLIB_MOTOR_SHOW_QR:
			mxd_galil_gclib_motor_show_qr( galil_gclib,
						galil_gclib_motor );
			break;

		case MXLV_GALIL_GCLIB_MOTOR_STOP_CODE:
			snprintf( command, sizeof(command),
				"SC%c", galil_gclib_motor->motor_name );

			mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			num_items = sscanf( response, "%lu",
					&(galil_gclib_motor->stop_code) );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Could not find the stop code in the "
				"response '%s' to command '%s' for "
				"Galil motor '%s'.",
					response, command, record->name );
			}
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
mxd_galil_gclib_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_motor_move_absolute()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "PA%c=%ld",
			galil_gclib_motor->motor_name,
			motor->raw_destination.stepper );

	mx_status = mxi_galil_gclib_command( galil_gclib, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"BG%c", galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib, command, NULL, 0 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_motor_get_position()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"TP%c", galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib,
			command, response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld",
			&(motor->raw_position.stepper) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not find the position for motor '%s' "
		"in the response '%s' to command '%s'.",
			motor->record->name,
			response, command );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_motor_set_position()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"DP%c=%ld", galil_gclib_motor->motor_name,
		motor->raw_set_position.stepper );

	mx_status = mxi_galil_gclib_command( galil_gclib, command, NULL, 0 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_motor_soft_abort()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"ST%c", galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib, command, NULL, 0 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_motor_immediate_abort()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"AB%c", galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib, command, NULL, 0 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_motor_raw_home_command()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on homing mode. */

	snprintf( command, sizeof(command),
		"FE%c", galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the home search. */

	snprintf( command, sizeof(command),
		"BG%c", galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib, command, NULL, 0 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_motor_get_parameter()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[80];
	int num_items;
	unsigned long ulong_value;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GALIL_GCLIB_MOTOR_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		snprintf( command, sizeof(command),
			"MG _SP%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &(motor->raw_speed) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the motor speed in the response '%s' "
			"to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}
		break;

	case MXLV_MTR_CURRENT_SPEED:
		snprintf( command, sizeof(command),
			"TV%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
				&(motor->raw_current_speed) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the current_motor speed in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}
		break;

	case MXLV_MTR_ACCELERATION_TYPE:
		motor->acceleration_type = MXF_MTR_ACCEL_RATE;
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		/* Acceleration parameter 0 = acceleration (units/sec**2) */

		snprintf( command, sizeof(command),
			"MG _AC%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response,
			"%lg", &(motor->raw_acceleration_parameters[0]) );
 
		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the motor acceeleration in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}
		break;

	case MXLV_MTR_AXIS_ENABLE:
	case MXLV_MTR_CLOSED_LOOP:
		snprintf( command, sizeof(command),
			"MG MO%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lu", &ulong_value );
 
		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the motor servo state in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}

		if ( ulong_value ) {
			motor->axis_enable = FALSE;
			motor->closed_loop = FALSE;
		} else {
			motor->axis_enable = TRUE;
			motor->closed_loop = TRUE;
		}
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		snprintf( command, sizeof(command),
			"MG _KP%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response,
				"%lg", &(motor->proportional_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the proportional gain in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		snprintf( command, sizeof(command),
			"MG _KI%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response,
				"%lg", &(motor->integral_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the integral gain in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		snprintf( command, sizeof(command),
			"MG _KD%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response,
				"%lg", &(motor->derivative_gain) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the derivative gain in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		snprintf( command, sizeof(command),
			"MG _IL%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response,
				"%lg", &(motor->integral_limit) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the integral limit in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		snprintf( command, sizeof(command),
			"MG _FV%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the velocity feedforward gain in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}

		motor->velocity_feedforward_gain = (double) ulong_value;
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		snprintf( command, sizeof(command),
			"MG _FA%c", galil_gclib_motor->motor_name );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					command, response, sizeof(response) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Cannot find the acceleration feedforward gain in the "
			"response '%s' to command '%s' for motor '%s'.",
				response, command, motor->record->name );
		}

		motor->acceleration_feedforward_gain = (double) ulong_value;
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_galil_gclib_motor_set_parameter()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_GALIL_GCLIB_MOTOR_DEBUG_PARAMETERS
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		snprintf( command, sizeof(command),
			"SP%c=%lu", galil_gclib_motor->motor_name,
			mx_round(motor->raw_speed) );

		mx_status = mxi_galil_gclib_command( galil_gclib,
							command, NULL, 0 );
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		/* Acceleration parameter 0 = acceleration (units/sec**2)
		 */

		snprintf( command, sizeof(command),
			"AC%c=%lu", galil_gclib_motor->motor_name,
			mx_round( motor->raw_acceleration_parameters[0] ));

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command),
			"DC%c=%lu", galil_gclib_motor->motor_name,
			mx_round( motor->raw_acceleration_parameters[0] ));

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );
		break;

	case MXLV_MTR_AXIS_ENABLE:
	case MXLV_MTR_CLOSED_LOOP:
		if ( (motor->axis_enable) || (motor->closed_loop) ) {
			snprintf( command, sizeof(command),
				"SH%c", galil_gclib_motor->motor_name );
		} else {
			snprintf( command, sizeof(command),
				"MO%c", galil_gclib_motor->motor_name );
		}

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		snprintf( command, sizeof(command),
			"KP%c=%g", galil_gclib_motor->motor_name,
			motor->proportional_gain );

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		snprintf( command, sizeof(command),
			"KI%c=%g", galil_gclib_motor->motor_name,
			motor->integral_gain );

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		snprintf( command, sizeof(command),
			"KD%c=%g", galil_gclib_motor->motor_name,
			motor->derivative_gain );

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );
		break;

	case MXLV_MTR_INTEGRAL_LIMIT:
		snprintf( command, sizeof(command),
			"IL%c=%lu", galil_gclib_motor->motor_name,
			mx_round( motor->integral_limit ));

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		snprintf( command, sizeof(command),
			"FV%c=%lu", galil_gclib_motor->motor_name,
			mx_round( motor->velocity_feedforward_gain ));

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		snprintf( command, sizeof(command),
			"FA%c=%lu", galil_gclib_motor->motor_name,
			mx_round( motor->acceleration_feedforward_gain ));

		mx_status = mxi_galil_gclib_command( galil_gclib,
						command, NULL, 0 );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_simultaneous_start( long num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				unsigned long flags )
{
	static const char fname[] = "mxd_galil_gclib_motor_simultaneous_start()";

	MX_RECORD *galil_interface_record = NULL;
	MX_RECORD *motor_record = NULL;
	MX_MOTOR *motor = NULL;
	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char destination_command[80];
	char simultaneous_start_command[500];
	long i;
	double raw_position;
	mx_status_type mx_status = MX_SUCCESSFUL_RESULT;

	/* Set the requested destinations for all of the motors
	 * while building the Galil move command to be run at
	 * the end of this function.
	 */

	memset( simultaneous_start_command, 0,
		sizeof(simultaneous_start_command) );

	strlcpy( simultaneous_start_command, "BG",
			sizeof(simultaneous_start_command) );

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		/* FIXME: We should verify that all of these motors
		 * are Galil Gclib-controlled motors.  For now we
		 * just assume that this is the case.
		 */

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		galil_gclib_motor = (MX_GALIL_GCLIB_MOTOR *)
					motor_record->record_type_struct;

		if ( galil_interface_record == (MX_RECORD *) NULL ) {
			galil_interface_record =
				galil_gclib_motor->galil_gclib_record;

			galil_gclib = (MX_GALIL_GCLIB *)
				galil_interface_record->record_type_struct;
		}

		/* Verify that all of the Galil motors are controlled by
		 * the same Galil controller.
		 */

		if ( galil_interface_record
			!= galil_gclib_motor->galil_gclib_record )
		{
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different Galil controllers, "
		"namely '%s' and '%s'.",
				motor_record_array[0]->name,
				motor_record->name,
				galil_interface_record->name,
				galil_gclib_motor->galil_gclib_record->name );
		}

		/* Compute the new position in raw units. */

		raw_position = 
			mx_divide_safely( position_array[i] - motor->offset,
						motor->scale );

		/* Set the new destination for this motor. */

		snprintf( destination_command, sizeof(destination_command),
			"PR%c=%ld", galil_gclib_motor->motor_name,
				mx_round( raw_position ) );

		mx_status = mxi_galil_gclib_command( galil_gclib,
					destination_command, NULL, 0 );

		/* Add this motor to the simultaneous start command. */

		simultaneous_start_command[i+2] = galil_gclib_motor->motor_name;
	}

	/* We finish by sending the simultaneous start command. */

	mx_status = mxi_galil_gclib_command( galil_gclib,
			simultaneous_start_command, NULL, 0 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_galil_gclib_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_galil_gclib_motor_get_extended_status()";

	MX_GALIL_GCLIB_MOTOR *galil_gclib_motor = NULL;
	MX_GALIL_GCLIB *galil_gclib = NULL;
	char command[80], response[500];
	int num_items;
	unsigned long switch_status;
	mx_status_type mx_status;

	mx_status = mxd_galil_gclib_motor_get_pointers( motor,
				&galil_gclib_motor, &galil_gclib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	/* Ask for axis-specific I/O status using the _TSm command. */

	snprintf( command, sizeof(command),
		"MG _TS%c", galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib,
			command, response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lu", &switch_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not find the motor status in the response '%s' "
		"to command '%s' for the Galil motor '%s'.",
			response, command, motor->record->name );
	}

	if ( switch_status & 0x80 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}
	if ( switch_status & 0x40 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}
	if ( switch_status & 0x20 ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
		motor->status |= MXSF_MTR_OPEN_LOOP;
	}
	/* 0x10 is reserved. */
	if ( (switch_status & 0x8) == 0 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	}
	if ( (switch_status & 0x4) == 0 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	}
	if ( (switch_status & 0x2) == 0 ) {
		motor->status |= MXSF_MTR_AT_HOME_SWITCH;

		motor->latched_status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}
	/* 0x1 is "position latch".  FIXME: Do not know what that is just yet.*/

	/* Get the motor position. */

	snprintf( command, sizeof(command),
		"TP%c", galil_gclib_motor->motor_name );

	mx_status = mxi_galil_gclib_command( galil_gclib,
			command, response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &(motor->raw_position.stepper) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not find the motor position in the response '%s' "
		"to command '%s' for the Galil motor '%s'.",
			response, command, motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

