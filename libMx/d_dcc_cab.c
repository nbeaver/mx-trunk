/*
 * Name:    d_dcc_cab.c 
 *
 * Purpose: MX motor driver for DCC-controlled model railroad locomotives.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DCC_CAB_DEBUG		FALSE

#define DCC_CAB_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_encoder.h"
#include "mx_motor.h"
#include "i_dcc_base.h"
#include "d_dcc_cab.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_dcc_cab_record_function_list = {
	NULL,
	mxd_dcc_cab_create_record_structures,
	mxd_dcc_cab_finish_record_initialization,
	NULL,
	NULL,
	mxd_dcc_cab_open,
};

MX_MOTOR_FUNCTION_LIST mxd_dcc_cab_motor_function_list = {
	mxd_dcc_cab_is_busy,
	mxd_dcc_cab_move_absolute,
	mxd_dcc_cab_get_position,
	mxd_dcc_cab_set_position,
	mxd_dcc_cab_soft_abort,
	mxd_dcc_cab_immediate_abort,
	NULL,
	NULL,
	NULL,
	mxd_dcc_cab_constant_velocity_move,
	mxd_dcc_cab_get_parameter,
	mxd_dcc_cab_set_parameter
};

/* Pontech DCC_CAB motor controller data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_dcc_cab_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_DCC_CAB_STANDARD_FIELDS
};

long mxd_dcc_cab_num_record_fields
		= sizeof( mxd_dcc_cab_record_field_defaults )
			/ sizeof( mxd_dcc_cab_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_dcc_cab_rfield_def_ptr
			= &mxd_dcc_cab_record_field_defaults[0];

/* === */

static mx_status_type
mxd_dcc_cab_get_pointers( MX_MOTOR *motor,
			MX_DCC_CAB **dcc_cab,
			MX_DCC_BASE **dcc_base,
			const char *calling_fname )
{
	static const char fname[] = "mxd_dcc_cab_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( dcc_cab == (MX_DCC_CAB **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DCC_CAB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*dcc_cab = (MX_DCC_CAB *) motor->record->record_type_struct;

	if ( *dcc_cab == (MX_DCC_CAB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DCC_CAB pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_dcc_cab_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_dcc_cab_create_record_structures()";

	MX_MOTOR *motor;
	MX_DCC_CAB *dcc_cab;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_MOTOR structure." );
	}

	dcc_cab = (MX_DCC_CAB *) malloc( sizeof(MX_DCC_CAB) );

	if ( dcc_cab == (MX_DCC_CAB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_DCC_CAB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = dcc_cab;
	record->class_specific_function_list = &mxd_dcc_cab_motor_function_list;

	motor->record = record;
	dcc_cab->record = record;

	/* A DCC-controlled locomotive is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* This driver stores the acceleration time. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	motor->raw_speed = 0.0;
	motor->raw_base_speed = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dcc_cab_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_dcc_cab_finish_record_initialization()";

	MX_DCC_CAB *dcc_cab;
	mx_status_type mx_status;

	dcc_cab = (MX_DCC_CAB *) record->record_type_struct;

	if ( dcc_cab == (MX_DCC_CAB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"record_type_struct for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check for a valid cab number. */

	if ( (dcc_cab->cab_number < 0)
	  || (dcc_cab->cab_number > 255) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal DCC_CAB cab number %ld.  The allowed range is (0-255).",
			dcc_cab->cab_number );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dcc_cab_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_dcc_cab_open()";

	MX_MOTOR *motor = NULL;
	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	mx_status_type mx_status;

	dcc_cab = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if a position encoder is available. */

	if ( strlen( dcc_cab->encoder_record_name ) == 0 ) {
		dcc_cab->encoder_record = NULL;
	} else {
		dcc_cab->encoder_record = mx_get_record( record,
						dcc_cab->encoder_record_name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dcc_cab_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dcc_cab_is_busy()";

	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	unsigned long encoder_status;
	mx_status_type mx_status;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( dcc_cab->encoder_record == (MX_RECORD *) NULL ) {
		/* FIXME: This is not a great solution. */

		motor->busy = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_encoder_get_status( dcc_cab->encoder_record,
						&encoder_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( encoder_status & MXSF_ENC_MOVING ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dcc_cab_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dcc_cab_move_absolute()";

	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	mx_status = mxi_dcc_base_command( dcc_base, DCC_CAB_DEBUG );

	return mx_status;
#else
	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Not yet implemented." );
#endif
}

MX_EXPORT mx_status_type
mxd_dcc_cab_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dcc_cab_get_position()";

	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	double raw_position;
	mx_status_type mx_status;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( dcc_cab->encoder_record == NULL ) {
		raw_position = 0.0;
	} else {
		mx_status = mx_encoder_read( dcc_cab->encoder_record,
							&raw_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	motor->raw_position.analog = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dcc_cab_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dcc_cab_set_position()";

	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	double raw_position;
	mx_status_type mx_status;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( dcc_cab->encoder_record == NULL )
		return MX_SUCCESSFUL_RESULT;

	raw_position = motor->raw_set_position.analog;

	mx_status = mx_encoder_write( dcc_cab->encoder_record, raw_position );

	return mx_status;
}

static mx_status_type
mxd_dcc_cab_stop_locomotive( MX_MOTOR *motor, int type_of_stop )
{
	static const char fname[] = "mxd_dcc_cab_stop_locomotive()";

	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	long new_dcc_speed;
	unsigned long cab_lighting_direction;
	mx_status_type mx_status;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If 'type_of_stop' == 0, then we decelerate to a normal stop.
	 *    'type_of_stop' != 0 is treated as 'emergency_stop'.
	 */

	if ( type_of_stop == 0 ) {
		new_dcc_speed = 0;
	} else {
		new_dcc_speed = -1L;
	}

	/* We first look at the the speed the locomotive had before the
	 * emergency stop to determine the direction of cab lighting.
	 */

	if ( motor->raw_speed >= 0.0 ) {
		cab_lighting_direction = 1;
	} else {
		cab_lighting_direction = 0;
	}

	snprintf( dcc_base->command, sizeof(dcc_base->command),
		"< t %lu %lu -1 %lu >",
		dcc_cab->packet_register, dcc_cab->cab_number,
		cab_lighting_direction );

	mx_status = mxi_dcc_base_command( dcc_base, DCC_CAB_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dcc_cab_soft_abort( MX_MOTOR *motor )
{
	return mxd_dcc_cab_stop_locomotive( motor, 0 );
}

MX_EXPORT mx_status_type
mxd_dcc_cab_immediate_abort( MX_MOTOR *motor )
{
	return mxd_dcc_cab_stop_locomotive( motor, -1 );
}

MX_EXPORT mx_status_type
mxd_dcc_cab_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dcc_cab_constant_velocity_move()";

	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	unsigned long cab_speed, cab_direction;
	mx_status_type mx_status;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->raw_speed >= 0.0 ) {
		cab_direction = 1;
	} else {
		cab_direction = 0;
	}

	cab_speed = mx_round( fabs( motor->raw_speed ) );

	if ( cab_speed > 126 ) {
		cab_speed = 126;
	}

	snprintf( dcc_base->command, sizeof(dcc_base->command),
		"< t %lu %lu %lu %lu >",
		dcc_cab->packet_register, dcc_cab->cab_number,
		cab_speed, cab_direction );

	mx_status = mxi_dcc_base_command( dcc_base, DCC_CAB_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dcc_cab_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dcc_cab_get_parameter()";

	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* The speed parameter is used when a throttle 't' command
		 * is requested.
		 */
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dcc_cab_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dcc_cab_set_parameter()";

	MX_DCC_CAB *dcc_cab = NULL;
	MX_DCC_BASE *dcc_base = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dcc_cab_get_pointers(motor, &dcc_cab, &dcc_base, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* The speed parameter is used when a throttle 't' command
		 * is requested.
		 */
		break;
	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

