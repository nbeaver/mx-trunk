/*
 * Name:    d_epics_motor.c 
 *
 * Purpose: MX motor driver for the EPICS BCDA developed motor record.
 *
 *          As of MX 1.0.2, Brian Tieman's portable channel access
 *          motor record is also supported.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_motor.h"
#include "d_epics_motor.h"

#include "mx_clock.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_epics_motor_record_function_list = {
	NULL,
	mxd_epics_motor_create_record_structures,
	mxd_epics_motor_finish_record_initialization,
	NULL,
	mxd_epics_motor_print_structure
};

MX_MOTOR_FUNCTION_LIST mxd_epics_motor_motor_function_list = {
	NULL,
	mxd_epics_motor_move_absolute,
	mxd_epics_motor_get_position,
	mxd_epics_motor_set_position,
	mxd_epics_motor_soft_abort,
	mxd_epics_motor_immediate_abort,
	NULL,
	NULL,
	mxd_epics_motor_find_home_position,
	mxd_epics_motor_constant_velocity_move,
	mxd_epics_motor_get_parameter,
	mxd_epics_motor_set_parameter,
	mxd_epics_motor_simultaneous_start,
	NULL,
	mxd_epics_motor_get_extended_status
};

/* EPICS motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_epics_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_EPICS_MOTOR_STANDARD_FIELDS
};

long mxd_epics_motor_num_record_fields
		= sizeof( mxd_epics_motor_record_field_defaults )
			/ sizeof( mxd_epics_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_motor_rfield_def_ptr
			= &mxd_epics_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_epics_motor_get_pointers( MX_MOTOR *motor,
			MX_EPICS_MOTOR **epics_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( epics_motor == (MX_EPICS_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*epics_motor = (MX_EPICS_MOTOR *) motor->record->record_type_struct;

	if ( *epics_motor == (MX_EPICS_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_epics_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_epics_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_EPICS_MOTOR *epics_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	epics_motor = (MX_EPICS_MOTOR *) malloc( sizeof(MX_EPICS_MOTOR) );

	if ( epics_motor == (MX_EPICS_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = epics_motor;
	record->class_specific_function_list
				= &mxd_epics_motor_motor_function_list;

	motor->record = record;

	/* An EPICS motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_epics_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_EPICS_MOTOR *epics_motor = NULL;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	/* Initialize MX EPICS data structures. */

	mx_epics_pvname_init( &(epics_motor->accl_pv),
				"%s.ACCL", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->dcof_pv),
				"%s.DCOF", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->dir_pv),
				"%s.DIR", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->dmov_pv),
				"%s.DMOV", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->hls_pv),
				"%s.HLS", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->icof_pv),
				"%s.ICOF", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->jogf_pv),
				"%s.JOGF", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->jogr_pv),
				"%s.JOGR", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->lls_pv),
				"%s.LLS", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->lvio_pv),
				"%s.LVIO", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->miss_pv),
				"%s.MISS", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->msta_pv),
				"%s.MSTA", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->pcof_pv),
				"%s.PCOF", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->rbv_pv),
				"%s.RBV", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->set_pv),
				"%s.SET", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->spmg_pv),
				"%s.SPMG", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->stop_pv),
				"%s.STOP", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->val_pv),
				"%s.VAL", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->vbas_pv),
				"%s.VBAS", epics_motor->epics_record_name );
	mx_epics_pvname_init( &(epics_motor->velo_pv),
				"%s.VELO", epics_motor->epics_record_name );

	epics_motor->driver_type          = MXT_EPICS_MOTOR_UNKNOWN;
	epics_motor->epics_record_version = -1.0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_motor_print_structure()";

	MX_MOTOR *motor;
	MX_EPICS_MOTOR *epics_motor = NULL;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type      = EPICS_MOTOR.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  EPICS record    = %s\n",
					epics_motor->epics_record_name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position        = %g %s  (%g).\n",
		motor->position, motor->units, 
		motor->raw_position.analog );
	fprintf(file, "  scale           = %g %s per EPICS EGU.\n",
		motor->scale, motor->units );
	fprintf(file, "  offset          = %g %s.\n",
		motor->offset, motor->units );
	fprintf(file, "  backlash        = %g %s  (%g).\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	fprintf(file, "  negative limit  = %g %s  (%g).\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );
	fprintf(file, "  positive limit  = %g %s  (%g).\n",
	    motor->positive_limit, motor->units,
	    motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband   = %g %s  (%g).\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_motor_move_absolute()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	double new_destination;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Begin the move. */

	new_destination = motor->raw_destination.analog;

	mx_status = mx_caput_nowait( &(epics_motor->val_pv),
				MX_CA_DOUBLE, 1, &new_destination );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Tieman EPICS motor record does not immediately show the
	 * motor as moving right after a move is commanded.  If we put
	 * in an intentional delay here, we can ensure that MX does not
	 * prematurely think that the move is complete.
	 */

	if ( epics_motor->driver_type == MXT_EPICS_MOTOR_TIEMAN ) {
		mx_status = mx_epics_flush_io();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(1000);	/* Rather a long time, unfortunately. */
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_motor_get_position()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	double position;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_motor->rbv_pv),
				MX_CA_DOUBLE, 1, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_motor_set_position( MX_MOTOR *motor )
{
	/* Setting the motor position is a three step process:
	 * 
	 * 1.  .SET <-- 1
	 * 2.  .VAL <-- new value
	 * 3.  .SET <-- 0
	 */

	static const char fname[] = "mxd_epics_motor_set_position()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	double user_set_position;
	short set_flag;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	user_set_position = motor->raw_set_position.analog;

	/*** Change the motor record from 'Use' to 'Set' mode. ***/

	set_flag = 1;

	mx_status = mx_caput( &(epics_motor->set_pv),
				MX_CA_SHORT, 1, &set_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Change the user position in the EPICS database. ***/

	mx_status = mx_caput( &(epics_motor->val_pv),
				MX_CA_DOUBLE, 1, &user_set_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Put the motor record back into 'Use' mode. ***/

	set_flag = 0;

	mx_status = mx_caput( &(epics_motor->set_pv),
				MX_CA_SHORT, 1, &set_flag );

	return mx_status;
}

/* mxd_epics_motor_soft_abort() stops the motor in a way that
 * allows the motor to immediately be moved again without taking
 * any special effort.
 */

MX_EXPORT mx_status_type
mxd_epics_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_motor_soft_abort()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	short stop_field, spmg_field;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( epics_motor->driver_type != MXT_EPICS_MOTOR_TIEMAN ) {

		/* The normal BCDA motor record uses the .STOP PV. */

		/* Stop the motor. */

		stop_field = 1;		/* Stop */

		mx_status = mx_caput( &(epics_motor->stop_pv),
				MX_CA_SHORT, 1, &stop_field );

		/* The .STOP field is automatically reset to 0 by the
		 * IOC database, so the motor is immediately ready to
		 * move again.
		 */

	} else {
		/* The Tieman motor record does not support the .STOP PV,
		 * so we must use the .SPMG PV instead.
		 */

		spmg_field = 0;		/* Stop */

		mx_status = mx_caput( &(epics_motor->spmg_pv),
				MX_CA_SHORT, 1, &spmg_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(500);

		/* Now reenable motor moves by setting .SPMG back to 'Go'. */

		spmg_field = 3;		/* Go */

		mx_status = mx_caput( &(epics_motor->spmg_pv),
				MX_CA_SHORT, 1, &spmg_field );

	}

	return mx_status;
}

/* mxd_epics_motor_immediate_abort() stops the motor in a way that
 * requires manual intervention (for example, via an MEDM screen)
 * to reset the .SPMG field back to "Go".
 */

MX_EXPORT mx_status_type
mxd_epics_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_motor_immediate_abort()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	short spmg_field;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the motor. */

	spmg_field = 0;		/* Stop */

	mx_status = mx_caput( &(epics_motor->spmg_pv),
				MX_CA_SHORT, 1, &spmg_field );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_motor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_motor_find_home_position()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	MX_EPICS_PV *pv;
	short home_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the home command without waiting for it to finish. */

	home_value = 1;

	if ( motor->home_search >= 0 ) {
		pv = &(epics_motor->homf_pv);
	} else {
		pv = &(epics_motor->homr_pv);
	}

	mx_status = mx_caput_nowait( pv, MX_CA_SHORT, 1, &home_value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_motor_constant_velocity_move()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	MX_EPICS_PV *pv;
	short move_command;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the move command without waiting for it to finish. */

	move_command = 1;

	if ( motor->constant_velocity_move >= 0 ) {
		pv = &(epics_motor->jogf_pv);
	} else {
		pv = &(epics_motor->jogr_pv);
	}

	mx_status = mx_caput_nowait( pv, MX_CA_SHORT, 1, &move_command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_motor_get_parameter()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	float float_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_caget( &(epics_motor->velo_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_speed = float_value;
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_caget( &(epics_motor->vbas_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_base_speed = float_value;
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		mx_status = mx_caget( &(epics_motor->accl_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_acceleration_parameters[0] = float_value;
		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		mx_status = mx_caget( &(epics_motor->pcof_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->proportional_gain = float_value;
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		mx_status = mx_caget( &(epics_motor->icof_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->integral_gain = float_value;
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		mx_status = mx_caget( &(epics_motor->dcof_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->derivative_gain = float_value;
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		motor->velocity_feedforward_gain = 0.0;
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		motor->acceleration_feedforward_gain = 0.0;
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_epics_motor_set_parameter()";

	MX_EPICS_MOTOR *epics_motor = NULL;
	float float_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		float_value = motor->raw_speed;

		mx_status = mx_caput( &(epics_motor->velo_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_MTR_BASE_SPEED:
		float_value = motor->raw_base_speed;

		mx_status = mx_caput( &(epics_motor->vbas_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		float_value = motor->raw_acceleration_parameters[0];

		mx_status = mx_caput( &(epics_motor->accl_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;

	case MXLV_MTR_AXIS_ENABLE:
		return mx_error( MXE_UNSUPPORTED, fname,
		    "Enabling or disabling EPICS motor '%s' is not supported.",
			motor->record->name);
		break;

	case MXLV_MTR_CLOSED_LOOP:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Enabling or disabling closed loop mode is not "
			"supported for EPICS motor '%s'.", motor->record->name);
		break;
	
	case MXLV_MTR_FAULT_RESET:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Fault reset is not supported for EPICS motor '%s'.",
				motor->record->name );
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		float_value = motor->proportional_gain;

		mx_status = mx_caput( &(epics_motor->pcof_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		float_value = motor->integral_gain;

		mx_status = mx_caput( &(epics_motor->icof_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		float_value = motor->derivative_gain;

		mx_status = mx_caput( &(epics_motor->dcof_pv),
					MX_CA_FLOAT, 1, &float_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_MTR_VELOCITY_FEEDFORWARD_GAIN:
		motor->velocity_feedforward_gain = 0.0;
		break;

	case MXLV_MTR_ACCELERATION_FEEDFORWARD_GAIN:
		motor->acceleration_feedforward_gain = 0.0;
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_motor_simultaneous_start( long num_motor_records,
				MX_RECORD **motor_record_array,
				double *destination_array,
				unsigned long flags )
{
	const char fname[] = "mxd_epics_motor_simultaneous_start()";

	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_EPICS_MOTOR *epics_motor = NULL;
	int i;
	double raw_destination;
	mx_status_type mx_status;

	if ( num_motor_records <= 0 )
		return MX_SUCCESSFUL_RESULT;

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor_record->mx_type != MXT_MTR_EPICS ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Cannot perform a simultaneous start since motors "
			"'%s' and '%s' are not the same type of motors.",
				motor_record_array[0]->name,
				motor_record->name );
		}

		epics_motor = (MX_EPICS_MOTOR *)
					motor_record->record_type_struct;

		/* Compute the new position in raw units. */

		raw_destination =
			mx_divide_safely( destination_array[i] - motor->offset,
						motor->scale );

		/* Send the move command for this motor. */

		mx_status = mx_caput_nowait( &(epics_motor->val_pv),
					MX_CA_DOUBLE, 1, &raw_destination );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Force the commands to be sent. */

	mx_status = mx_epics_flush_io();

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_motor_get_extended_status( MX_MOTOR *motor )
{
	const char fname[] = "mxd_epics_motor_get_extended_status()";

	MX_EPICS_GROUP epics_group;
	MX_EPICS_MOTOR *epics_motor = NULL;
	char pvname[MXU_EPICS_PVNAME_LENGTH+1];
	char driver_name[60];
	float version_number;
	double raw_position;
	unsigned long epics_motor_status;
	short done_moving_status, miss_pv_value;
	short positive_limit_hit, negative_limit_hit;
	short user_direction, lvio_pv_value;
	mx_status_type mx_status;

	mx_status = mxd_epics_motor_get_pointers( motor, &epics_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	done_moving_status = 0;
	epics_motor_status = 0;
	positive_limit_hit = 0;
	negative_limit_hit = 0;

	/* If we have not yet found out the EPICS driver type and
	 * version number for this record, ask the EPICS IOC for this.
	 */

	if ( epics_motor->driver_type == MXT_EPICS_MOTOR_UNKNOWN ) {

		/* See what the driver type is. */

		sprintf( pvname, "%s.DTYP",
				epics_motor->epics_record_name );

		mx_status = mx_caget_by_name( pvname,
				MX_CA_STRING, 1, driver_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: record '%s' driver_name = '%s'",
			fname, motor->record->name, driver_name ));

		if ( strcmp( driver_name, "Aero U500" ) == 0 ) {
			epics_motor->driver_type = MXT_EPICS_MOTOR_TIEMAN;
		} else {
			epics_motor->driver_type = MXT_EPICS_MOTOR_BCDA;
		}

		/* If supported, ask for the version number. */

		if ( epics_motor->driver_type == MXT_EPICS_MOTOR_TIEMAN ) {
			epics_motor->epics_record_version = 0;
		} else {
			sprintf( pvname, "%s.VERS",
				epics_motor->epics_record_name );

			mx_status = mx_caget_by_name( pvname,
				MX_CA_FLOAT, 1, &version_number );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			epics_motor->epics_record_version = version_number;
		}
	}

	/* Encapsulate the calls in an EPICS synchronous group. */

	mx_status = mx_epics_start_group( &epics_group );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Request the motor position. */

	mx_status = mx_group_caget( &epics_group, &(epics_motor->rbv_pv),
					MX_CA_DOUBLE, 1, &raw_position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );
		return mx_status;
	}

	/* Check the 'done moving' flag. */

	done_moving_status = 0;

	mx_status = mx_group_caget( &epics_group,
			&(epics_motor->dmov_pv),
			MX_CA_SHORT, 1, &done_moving_status );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );
		return mx_status;
	}

	/* Request the EPICS motor status. */

	if ( epics_motor->epics_record_version >= 4.0 ) {
		mx_status = mx_group_caget( &epics_group,
				&(epics_motor->msta_pv),
				MX_CA_LONG, 1, &epics_motor_status );
	
		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );
			return mx_status;
		}
	}

	/* Request the retry limit status.
	 *
	 * The Tieman EPICS motor record does not support the 'MISS' PV.
	 */

	if ( epics_motor->driver_type == MXT_EPICS_MOTOR_TIEMAN ) {
		miss_pv_value = 0;
	} else {
		mx_status = mx_group_caget( &epics_group,
				&(epics_motor->miss_pv),
				MX_CA_SHORT, 1, &miss_pv_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );
			return mx_status;
		}
	}

	/* Versions of the motor record older than 4.5 do not distinguish
	 * between positive limit hit and negative limit hit in the
	 * motor status word, so we have to ask for them individually.
	 */

	if ( epics_motor->epics_record_version < 4.5 ) {

		/* Request the motor positive limit status. */

		mx_status = mx_group_caget(&epics_group, &(epics_motor->hls_pv),
					MX_CA_SHORT, 1, &positive_limit_hit );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );
			return mx_status;
		}

		/* Request the motor negative limit status. */

		mx_status = mx_group_caget(&epics_group, &(epics_motor->lls_pv),
					MX_CA_SHORT, 1, &negative_limit_hit );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );
			return mx_status;
		}
	} else {
		/* epics_record_version >= 4.5 */

		/* If we are using the MSTA field to get the limit status,
		 * we also need the user direction value to convert the 
		 * direction of the limit into user units.
		 */

		mx_status = mx_group_caget(&epics_group, &(epics_motor->dir_pv),
					MX_CA_SHORT, 1, &user_direction );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );
			return mx_status;
		}
	}

	/* We also check for soft limit violation. */

	mx_status = mx_group_caget( &epics_group, &(epics_motor->lvio_pv),
					MX_CA_SHORT, 1, &lvio_pv_value );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );
		return mx_status;
	}

	/* Send all the commands. */

	mx_status = mx_epics_end_group( &epics_group );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Analyze the results.  The bit definitions for the MSTA field
	 * stored here in the 'epics_motor_status' variable may be found
	 * in the EPICS motor record documentation that can be found at
	 * http://www.aps.anl.gov/upd/people/sluiter/epics/motor/index.html
	 */

	motor->raw_position.analog = raw_position;

	motor->status = 0;

	if ( miss_pv_value ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	if ( epics_motor->epics_record_version < 4.5 ) {
		if ( positive_limit_hit ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		}
		if ( negative_limit_hit ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		}
	} else {
		if ( user_direction == 0 ) {	/* DIR = Positive direction */
			if ( epics_motor_status & 0x4 ) {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			}
			if ( epics_motor_status & 0x2000 ) {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			}
		} else {			/* DIR = Negative direction. */
			if ( epics_motor_status & 0x4 ) {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			}
			if ( epics_motor_status & 0x2000 ) {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			}
		}

		if ( epics_motor_status & 0x1000 ) {
			/* Controller communication error. */

			motor->status |= MXSF_MTR_ERROR;
		}
	}

	/* The EPICS motor record LVIO PV does not tell you which
	 * soft limit that you exceeded, so you must report both
	 * of them as tripped.
	 */

	if ( lvio_pv_value ) {
		motor->status |= MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT;
		motor->status |= MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT;
	}

	if ( done_moving_status == 0 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	if ( epics_motor->epics_record_version >= 4.0 ) {
		if ( ( epics_motor_status & 0x2 ) == 0 ) {
			/* Motion is complete. */

			motor->status |= MXSF_MTR_IS_BUSY;
		}
		if ( epics_motor_status & 0x80 ) {
			/* At home position. */

			motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
		}
		if ( epics_motor_status & 0x800 ) {
			/* Closed loop position control is supported. */

			if ( ( epics_motor_status & 0x20 ) == 0 ) {
			    /* Closed loop position control is NOT enabled. */

			    motor->status |= MXSF_MTR_OPEN_LOOP;
			}
		}
		if ( epics_motor_status & 0x200 ) {
			/* Driver stopped polling. */

			motor->status |= MXSF_MTR_ERROR;
		}
	}

	if ( motor->status & MXSF_MTR_ERROR_BITMASK ) {
		motor->status |= MXSF_MTR_ERROR;
	}

#if 0
	MX_DEBUG(-2,("%s: motor raw position    = %g", fname,
						motor->raw_position.analog));

	MX_DEBUG(-2,("%s: done_moving_status    = %d", fname,
						done_moving_status ));

	if ( epics_motor->epics_record_version >= 4.0 ) {
		MX_DEBUG(-2,
		    ("%s: epics_motor_status    = %#lx", fname,
						epics_motor_status ));
	}

	MX_DEBUG(-2,("%s: miss_pv_value         = %hd", fname, miss_pv_value ));

	MX_DEBUG(-2,("%s: motor->status         = %#lx", fname, motor->status));

	MX_DEBUG(-2,("%s: IS_BUSY               = %#lx", fname,
			motor->status & MXSF_MTR_IS_BUSY ));

	MX_DEBUG(-2,("%s: POSITIVE_LIMIT_HIT    = %#lx", fname,
			motor->status & MXSF_MTR_POSITIVE_LIMIT_HIT ));

	MX_DEBUG(-2,("%s: NEGATIVE_LIMIT_HIT    = %#lx", fname,
			motor->status & MXSF_MTR_NEGATIVE_LIMIT_HIT ));

	MX_DEBUG(-2,("%s: HOME_SEARCH_SUCCEEDED = %#lx", fname,
			motor->status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ));

	MX_DEBUG(-2,("%s: FOLLOWING_ERROR       = %#lx", fname,
			motor->status & MXSF_MTR_FOLLOWING_ERROR ));

	MX_DEBUG(-2,("%s: DRIVE_FAULT           = %#lx", fname,
			motor->status & MXSF_MTR_DRIVE_FAULT ));

	MX_DEBUG(-2,("%s: AXIS_DISABLED         = %#lx", fname,
			motor->status & MXSF_MTR_AXIS_DISABLED ));

	MX_DEBUG(-2,("%s: OPEN_LOOP             = %#lx", fname,
			motor->status & MXSF_MTR_OPEN_LOOP ));

	MX_DEBUG(-2,("%s: ERROR                 = %#lx", fname,
			motor->status & MXSF_MTR_ERROR ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EPICS */

