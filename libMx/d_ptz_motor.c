/*
 * Name:    d_ptz_motor.c
 *
 * Purpose: MX motor driver for Pan/Tilt/Zoom controller motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2007, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PTZ_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_ptz.h"
#include "d_ptz_motor.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_ptz_motor_record_function_list = {
	NULL,
	mxd_ptz_motor_create_record_structures,
	mxd_ptz_motor_finish_record_initialization,
	NULL,
	mxd_ptz_motor_print_structure
};

MX_MOTOR_FUNCTION_LIST mxd_ptz_motor_motor_function_list = {
	NULL,
	mxd_ptz_motor_move_absolute,
	mxd_ptz_motor_get_position,
	NULL,
	mxd_ptz_motor_soft_abort,
	NULL,
	NULL,
	NULL,
	mxd_ptz_motor_raw_home_command,
	NULL,
	mxd_ptz_motor_get_parameter,
	mxd_ptz_motor_set_parameter,
	NULL,
	mxd_ptz_motor_get_status
};

/* PTZ motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ptz_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PTZ_MOTOR_STANDARD_FIELDS
};

long mxd_ptz_motor_num_record_fields
		= sizeof( mxd_ptz_motor_record_field_defaults )
			/ sizeof( mxd_ptz_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ptz_motor_rfield_def_ptr
			= &mxd_ptz_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_ptz_motor_get_pointers( MX_MOTOR *motor,
			MX_PTZ_MOTOR **ptz_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_ptz_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ptz_motor == (MX_PTZ_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PTZ_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*ptz_motor = (MX_PTZ_MOTOR *) motor->record->record_type_struct;

	if ( *ptz_motor == (MX_PTZ_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PTZ_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_ptz_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_ptz_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_PTZ_MOTOR *ptz_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	ptz_motor = (MX_PTZ_MOTOR *) malloc( sizeof(MX_PTZ_MOTOR) );

	if ( ptz_motor == (MX_PTZ_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_PTZ_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = ptz_motor;
	record->class_specific_function_list =
				&mxd_ptz_motor_motor_function_list;

	motor->record = record;
	ptz_motor->record = record;

	/* A PTZ motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The PTZ reports acceleration in steps/sec**2. */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ptz_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_ptz_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_PTZ_MOTOR *ptz_motor;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ptz_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_ptz_motor_print_structure()";

	MX_MOTOR *motor;
	MX_PTZ_MOTOR *ptz_motor;
	double position, move_deadband, speed;
	mx_status_type mx_status;

	ptz_motor = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type         = PTZ motor.\n\n");

	fprintf(file, "  name               = %s\n", record->name);
	fprintf(file, "  PTZ record name    = %s\n",
					ptz_motor->ptz_record->name);
	fprintf(file, "  PTZ motor type     = %ld\n",
					ptz_motor->ptz_motor_type);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
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
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit     = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit     = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband      = %g %s  (%ld steps)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	mx_status = mx_motor_get_speed( record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  speed              = %g %s/s  (%g steps/s)\n",
		speed, motor->units,
		motor->raw_speed );

	fprintf(file, "\n");

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_ptz_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ptz_motor_move_absolute()";

	MX_PTZ_MOTOR *ptz_motor;
	mx_status_type mx_status;

	ptz_motor = NULL;

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ptz_motor->ptz_motor_type ) {
	case MXT_PTZ_MOTOR_PAN:
		mx_status = mx_ptz_set_pan( ptz_motor->ptz_record,
					motor->raw_destination.stepper );
		break;
	case MXT_PTZ_MOTOR_TILT:
		mx_status = mx_ptz_set_tilt( ptz_motor->ptz_record,
					motor->raw_destination.stepper );
		break;
	case MXT_PTZ_MOTOR_ZOOM:
		mx_status = mx_ptz_set_zoom( ptz_motor->ptz_record,
					motor->raw_destination.stepper );
		break;
	case MXT_PTZ_MOTOR_FOCUS:
		mx_status = mx_ptz_set_focus( ptz_motor->ptz_record,
					motor->raw_destination.stepper );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported PTZ motor type %ld requested for PTZ motor '%s'.",
			ptz_motor->ptz_motor_type, motor->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ptz_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ptz_motor_get_position()";

	MX_PTZ_MOTOR *ptz_motor;
	unsigned long ulong_value;
	mx_status_type mx_status;

	ptz_motor = NULL;

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ptz_motor->ptz_motor_type ) {
	case MXT_PTZ_MOTOR_PAN:
		mx_status = mx_ptz_get_pan( ptz_motor->ptz_record,
					&(motor->raw_position.stepper) );
		break;
	case MXT_PTZ_MOTOR_TILT:
		mx_status = mx_ptz_get_tilt( ptz_motor->ptz_record,
					&(motor->raw_position.stepper) );
		break;
	case MXT_PTZ_MOTOR_ZOOM:
		mx_status = mx_ptz_get_zoom( ptz_motor->ptz_record,
							&ulong_value );

		motor->raw_position.stepper = (long) ulong_value;
		break;
	case MXT_PTZ_MOTOR_FOCUS:
		mx_status = mx_ptz_get_focus( ptz_motor->ptz_record,
							&ulong_value );

		motor->raw_position.stepper = (long) ulong_value;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported PTZ motor type %ld requested for PTZ motor '%s'.",
			ptz_motor->ptz_motor_type, motor->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ptz_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ptz_motor_soft_abort()";

	MX_PTZ_MOTOR *ptz_motor;
	mx_status_type mx_status;

	ptz_motor = NULL;

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ptz_motor->ptz_motor_type ) {
	case MXT_PTZ_MOTOR_PAN:
		mx_status = mx_ptz_pan_stop( ptz_motor->ptz_record );
		break;
	case MXT_PTZ_MOTOR_TILT:
		mx_status = mx_ptz_tilt_stop( ptz_motor->ptz_record );
		break;
	case MXT_PTZ_MOTOR_ZOOM:
		mx_status = mx_ptz_zoom_stop( ptz_motor->ptz_record );
		break;
	case MXT_PTZ_MOTOR_FOCUS:
		mx_status = mx_ptz_focus_stop( ptz_motor->ptz_record );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported PTZ motor type %ld requested for PTZ motor '%s'.",
			ptz_motor->ptz_motor_type, motor->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ptz_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ptz_motor_raw_home_command()";

	MX_PTZ_MOTOR *ptz_motor;
	mx_status_type mx_status;

	ptz_motor = NULL;

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_ptz_command( ptz_motor->ptz_record, MXF_PTZ_DRIVE_HOME );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ptz_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ptz_motor_get_parameter()";

	MX_PTZ_MOTOR *ptz_motor;
	unsigned long raw_speed;
	mx_status_type mx_status;

	ptz_motor = NULL;

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		switch( ptz_motor->ptz_motor_type ) {
		case MXT_PTZ_MOTOR_PAN:
			mx_status = mx_ptz_get_pan_speed(
					ptz_motor->ptz_record, &raw_speed );
			break;
		case MXT_PTZ_MOTOR_TILT:
			mx_status = mx_ptz_get_tilt_speed(
					ptz_motor->ptz_record, &raw_speed );
			break;
		case MXT_PTZ_MOTOR_ZOOM:
			mx_status = mx_ptz_get_zoom_speed(
					ptz_motor->ptz_record, &raw_speed );
			break;
		case MXT_PTZ_MOTOR_FOCUS:
			mx_status = mx_ptz_get_focus_speed(
					ptz_motor->ptz_record, &raw_speed );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported PTZ motor type %ld requested for PTZ motor '%s'.",
				ptz_motor->ptz_motor_type, motor->record->name);
		}

		motor->raw_speed = (double) raw_speed;
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ptz_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ptz_motor_set_parameter()";

	MX_PTZ_MOTOR *ptz_motor;
	unsigned long raw_speed;
	mx_status_type mx_status;

	ptz_motor = NULL;

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		raw_speed = mx_round( motor->raw_speed );

		switch( ptz_motor->ptz_motor_type ) {
		case MXT_PTZ_MOTOR_PAN:
			mx_status = mx_ptz_set_pan_speed(
					ptz_motor->ptz_record, raw_speed );
			break;
		case MXT_PTZ_MOTOR_TILT:
			mx_status = mx_ptz_set_tilt_speed(
					ptz_motor->ptz_record, raw_speed );
			break;
		case MXT_PTZ_MOTOR_ZOOM:
			mx_status = mx_ptz_set_zoom_speed(
					ptz_motor->ptz_record, raw_speed );
			break;
		case MXT_PTZ_MOTOR_FOCUS:
			mx_status = mx_ptz_set_focus_speed(
					ptz_motor->ptz_record, raw_speed );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported PTZ motor type %ld requested for PTZ motor '%s'.",
				ptz_motor->ptz_motor_type, motor->record->name);
		}
		break;
	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ptz_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ptz_motor_get_status()";

	MX_PTZ_MOTOR *ptz_motor;
	unsigned long ptz_status;
	mx_status_type mx_status;

	ptz_motor = NULL;

	mx_status = mxd_ptz_motor_get_pointers( motor, &ptz_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	mx_status = mx_ptz_get_status( ptz_motor->ptz_record, &ptz_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ptz_status & MXSF_PTZ_HOME_SEARCH_SUCCEEDED ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	switch( ptz_motor->ptz_motor_type ) {
	case MXT_PTZ_MOTOR_PAN:
		if ( ptz_status & MXSF_PTZ_PAN_IS_BUSY ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}
		if ( ptz_status & MXSF_PTZ_PAN_RIGHT_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		}
		if ( ptz_status & MXSF_PTZ_PAN_LEFT_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		}
		if ( ptz_status & MXSF_PTZ_PAN_FOLLOWING_ERROR ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		}
		if ( ptz_status & MXSF_PTZ_PAN_DRIVE_FAULT ) {
			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}
		break;
	case MXT_PTZ_MOTOR_TILT:
		if ( ptz_status & MXSF_PTZ_TILT_IS_BUSY ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}
		if ( ptz_status & MXSF_PTZ_TILT_TOP_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		}
		if ( ptz_status & MXSF_PTZ_TILT_BOTTOM_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		}
		if ( ptz_status & MXSF_PTZ_TILT_FOLLOWING_ERROR ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		}
		if ( ptz_status & MXSF_PTZ_TILT_DRIVE_FAULT ) {
			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}
		break;
	case MXT_PTZ_MOTOR_ZOOM:
		if ( ptz_status & MXSF_PTZ_ZOOM_IS_BUSY ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}
		if ( ptz_status & MXSF_PTZ_ZOOM_IN_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		}
		if ( ptz_status & MXSF_PTZ_ZOOM_OUT_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		}
		if ( ptz_status & MXSF_PTZ_ZOOM_FOLLOWING_ERROR ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		}
		if ( ptz_status & MXSF_PTZ_ZOOM_DRIVE_FAULT ) {
			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}
		break;
	case MXT_PTZ_MOTOR_FOCUS:
		if ( ptz_status & MXSF_PTZ_FOCUS_IS_BUSY ) {
			motor->status |= MXSF_MTR_IS_BUSY;
		}
		if ( ptz_status & MXSF_PTZ_FOCUS_NEAR_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		}
		if ( ptz_status & MXSF_PTZ_FOCUS_FAR_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		}
		if ( ptz_status & MXSF_PTZ_FOCUS_FOLLOWING_ERROR ) {
			motor->status |= MXSF_MTR_FOLLOWING_ERROR;
		}
		if ( ptz_status & MXSF_PTZ_FOCUS_DRIVE_FAULT ) {
			motor->status |= MXSF_MTR_DRIVE_FAULT;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported PTZ motor type %ld requested for PTZ motor '%s'.",
			ptz_motor->ptz_motor_type, motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

