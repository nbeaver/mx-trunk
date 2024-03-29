/*
 * Name:    d_dtu_stage.c
 *
 * Purpose: MX driver for a tilt angle pseudomotor used at BioCAT by
 *          a general user group from dtu.dk.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_DTU_STAGE_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_constants.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "mx_motor.h"
#include "d_dtu_stage.h"

MX_RECORD_FUNCTION_LIST mxd_dtu_stage_record_function_list = {
	NULL,
	mxd_dtu_stage_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_dtu_stage_open
};

MX_MOTOR_FUNCTION_LIST mxd_dtu_stage_motor_function_list = {
	NULL,
	mxd_dtu_stage_move_absolute,
	mxd_dtu_stage_get_position,
	NULL,
	mxd_dtu_stage_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler,
	NULL,
	mxd_dtu_stage_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_dtu_stage_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_DTU_STAGE_STANDARD_FIELDS
};

long mxd_dtu_stage_num_record_fields =
		sizeof( mxd_dtu_stage_record_field_defaults )
		/ sizeof( mxd_dtu_stage_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_dtu_stage_rfield_def_ptr
			= &mxd_dtu_stage_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_dtu_stage_get_pointers( MX_MOTOR *motor,
			MX_DTU_STAGE **dtu_stage,
			const char *calling_fname )
{
	static const char fname[] = "mxd_dtu_stage_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( dtu_stage == (MX_DTU_STAGE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_DTU_STAGE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_MOTOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	*dtu_stage = (MX_DTU_STAGE *) motor->record->record_type_struct;

	if ( *dtu_stage == (MX_DTU_STAGE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DTU_STAGE pointer for motor "
			"record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_dtu_stage_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_dtu_stage_create_record_structures()";

	MX_MOTOR *motor = NULL;
	MX_DTU_STAGE *dtu_stage = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	dtu_stage = (MX_DTU_STAGE *) malloc( sizeof(MX_DTU_STAGE) );

	if ( dtu_stage == (MX_DTU_STAGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_DTU_STAGE structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = dtu_stage;
	record->class_specific_function_list
				= &mxd_dtu_stage_motor_function_list;

	motor->record = record;
	dtu_stage->record = record;

	motor->motor_flags = MXF_MTR_IS_PSEUDOMOTOR;

	/* This pseudomotor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dtu_stage_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_dtu_stage_open()";

	MX_MOTOR *motor = NULL;
	MX_DTU_STAGE *dtu_stage = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_dtu_stage_get_pointers( motor, &dtu_stage, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_strcasecmp( "alpha", dtu_stage->pseudomotor_name ) == 0 ) {
		dtu_stage->pseudomotor_type = MXT_DTU_STAGE_ALPHA;
	} else
	if ( mx_strcasecmp( "phi", dtu_stage->pseudomotor_name ) == 0 ) {
		dtu_stage->pseudomotor_type = MXT_DTU_STAGE_PHI;
	} else {
		dtu_stage->pseudomotor_type = 0;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The pseudomotor type '%s' for motor '%s' is illegal.  "
		"The legal pseudomotor types are 'alpha' and 'phi'.",
			dtu_stage->pseudomotor_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dtu_stage_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dtu_stage_move_absolute()";

	MX_DTU_STAGE *dtu_stage = NULL;
	MX_RECORD *raw_motor_record = NULL;
	mx_bool_type motors_are_idle;
	double delta_l, alpha, phi, delta_x, delta_y;
	double alpha_destination, phi_destination;
	double pseudomotor_destination;
	mx_status_type mx_status;

	mx_status = mxd_dtu_stage_get_pointers( motor, &dtu_stage, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that none of the motors are moving. */

	mx_status = mx_motor_is_idle( motor->record, &motors_are_idle );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: motor '%s', motors_are_idle = %d",
		fname, motor->record->name, motors_are_idle ));
#endif

	if ( motors_are_idle == FALSE ) {
		return mx_error( MXE_NOT_READY, fname,
		"One or more of the  motors used by tilt angle '%s' are "
		"either moving or in an error state.", motor->record->name );
	}

	mx_status = mx_get_double_variable( dtu_stage->delta_l_record,
								&delta_l );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: delta_l = %f", fname, delta_l));
#endif

	mx_status = mx_motor_get_position( dtu_stage->alpha_motor_record,
								&alpha );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( dtu_stage->phi_motor_record, &phi );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: motor type %lu, alpha = %f, phi = %f",
		fname, dtu_stage->pseudomotor_type, alpha, phi ));
#endif

	/*----*/

	pseudomotor_destination = motor->raw_destination.analog;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: pseudomotor_destination = %f",
		fname, pseudomotor_destination));
#endif

	switch( dtu_stage->pseudomotor_type ) {
	case MXT_DTU_STAGE_ALPHA:
		raw_motor_record = dtu_stage->alpha_motor_record;
		alpha_destination = pseudomotor_destination;
		phi_destination = phi;
		break;
	case MXT_DTU_STAGE_PHI:
		raw_motor_record = dtu_stage->phi_motor_record;
		alpha_destination = alpha;
		phi_destination = pseudomotor_destination;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The pseudomotor type '%s' for motor '%s' is illegal.  "
		"The legal pseudomotor types are 'alpha' and 'phi'.",
			dtu_stage->pseudomotor_name, motor->record->name );
		break;
	}

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: alpha_destination = %f, phi_destination = %f",
		fname, alpha_destination, phi_destination));
#endif

	/*----*/

	delta_x = sin( MX_RADIANS_PER_DEGREE * alpha_destination )
		* delta_l * cos( MX_RADIANS_PER_DEGREE * phi_destination );

	delta_y = delta_l - cos( MX_RADIANS_PER_DEGREE * alpha_destination )
								* delta_l;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: delta_x = %f, delta_y = %f",
		fname, delta_x, delta_y));
#endif

	/*----*/

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,
	("%s: raw_motor_record = '%s', pseudomotor_destination = %f",
		fname, raw_motor_record->name, pseudomotor_destination));
#endif

	mx_status = mx_motor_move_absolute( raw_motor_record,
					pseudomotor_destination, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute( dtu_stage->x_motor_record,
								delta_x, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_motor_soft_abort( motor->record );

		return mx_status;
	}

	mx_status = mx_motor_move_absolute( dtu_stage->y_motor_record,
								delta_y, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_motor_soft_abort( motor->record );
		(void) mx_motor_soft_abort( dtu_stage->x_motor_record );

		return mx_status;
	}

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s complete for motor '%s'",
			fname, motor->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dtu_stage_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dtu_stage_get_position()";

	MX_DTU_STAGE *dtu_stage = NULL;
	MX_RECORD *raw_motor_record = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dtu_stage_get_pointers( motor, &dtu_stage, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( dtu_stage->pseudomotor_type ) {
	case MXT_DTU_STAGE_ALPHA:
		raw_motor_record = dtu_stage->alpha_motor_record;
		break;
	case MXT_DTU_STAGE_PHI:
		raw_motor_record = dtu_stage->phi_motor_record;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The pseudomotor type '%s' for motor '%s' is illegal.  "
		"The legal pseudomotor types are 'alpha' and 'phi'.",
			dtu_stage->pseudomotor_name, motor->record->name );
		break;
	}

	mx_status = mx_motor_get_position( raw_motor_record,
					&(motor->raw_position.analog) );

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: raw motor '%s' position = %f",
		fname, raw_motor_record->name, motor->raw_position.analog));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dtu_stage_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dtu_stage_soft_abort()";

	MX_DTU_STAGE *dtu_stage = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dtu_stage_get_pointers( motor, &dtu_stage, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s'", fname, motor->record->name ));
#endif
	/* We must abort ALL of the motors, even if some of the aborts fail! */

	(void) mx_motor_soft_abort( dtu_stage->alpha_motor_record );
	(void) mx_motor_soft_abort( dtu_stage->x_motor_record );
	(void) mx_motor_soft_abort( dtu_stage->y_motor_record );
	(void) mx_motor_soft_abort( dtu_stage->phi_motor_record );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dtu_stage_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_dtu_stage_get_status()";

	MX_DTU_STAGE *dtu_stage = NULL;
	unsigned long x_status, y_status, alpha_status, phi_status;
	mx_status_type mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s invoked for pseudomotor '%s'",
		fname, motor->record->name ));
#endif

	mx_status = mxd_dtu_stage_get_pointers( motor, &dtu_stage, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( dtu_stage->alpha_motor_record,
							&alpha_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: alpha motor '%s' status = %#lx",
		fname, dtu_stage->alpha_motor_record->name, alpha_status));
#endif

	mx_status = mx_motor_get_status( dtu_stage->x_motor_record, &x_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: x motor '%s' status = %#lx",
		fname, dtu_stage->x_motor_record->name, x_status));
#endif

	mx_status = mx_motor_get_status( dtu_stage->y_motor_record, &y_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: y motor '%s' status = %#lx",
		fname, dtu_stage->y_motor_record->name, y_status));
#endif

	mx_status = mx_motor_get_status( dtu_stage->phi_motor_record,
							&phi_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: phi motor '%s' status = %#lx",
		fname, dtu_stage->phi_motor_record->name, phi_status));
#endif

	if ( (x_status & MXSF_MTR_IS_BUSY)
		|| (y_status & MXSF_MTR_IS_BUSY)
		|| (alpha_status & MXSF_MTR_IS_BUSY)
		|| (phi_status & MXSF_MTR_IS_BUSY) )
	{
		motor->status = MXSF_MTR_IS_BUSY;
	} else {
		motor->status = 0;
	}

	if ( (x_status & MXSF_MTR_ERROR_BITMASK)
		|| (y_status & MXSF_MTR_ERROR_BITMASK)
		|| (alpha_status & MXSF_MTR_ERROR_BITMASK)
		|| (phi_status & MXSF_MTR_ERROR_BITMASK) )
	{
		motor->status |= MXSF_MTR_ERROR;
	}

#if MXD_DTU_STAGE_DEBUG
	MX_DEBUG(-2,("%s: pseudomotor '%s' status = %#lx",
		fname, motor->record->name, motor->status ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

