/*
 * Name:    d_biocat_mirror_positioner.c
 *
 * Purpose: MX driver that coordinates the moves of the BioCAT (APS 18-ID)
 *          mirror angle, downstream angle, collimator and table positions.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_BIOCAT_MIRROR_POSITIONER_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "d_biocat_mirror_positioner.h"

MX_RECORD_FUNCTION_LIST mxd_biocat_mirror_pos_record_function_list = {
	mxd_biocat_mirror_pos_initialize_driver,
	mxd_biocat_mirror_pos_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_biocat_mirror_pos_open
};

MX_MOTOR_FUNCTION_LIST mxd_biocat_mirror_pos_motor_function_list = {
	NULL,
	mxd_biocat_mirror_pos_move_absolute,
	mxd_biocat_mirror_pos_get_position,
	NULL,
	mxd_biocat_mirror_pos_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_biocat_mirror_pos_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_biocat_mirror_pos_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_BIOCAT_MIRROR_POSITIONER_STANDARD_FIELDS
};

long mxd_biocat_mirror_pos_num_record_fields =
		sizeof( mxd_biocat_mirror_pos_record_field_defaults )
		/ sizeof( mxd_biocat_mirror_pos_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_biocat_mirror_pos_rfield_def_ptr
			= &mxd_biocat_mirror_pos_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_biocat_mirror_pos_get_pointers( MX_MOTOR *motor,
			MX_BIOCAT_MIRROR_POSITIONER **biocat_mirror_pos,
			const char *calling_fname )
{
	static const char fname[] = "mxd_biocat_mirror_pos_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( biocat_mirror_pos == (MX_BIOCAT_MIRROR_POSITIONER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_BIOCAT_MIRROR_POSITIONER pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_MOTOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	*biocat_mirror_pos = (MX_BIOCAT_MIRROR_POSITIONER *)
				motor->record->record_type_struct;

	if ( *biocat_mirror_pos == (MX_BIOCAT_MIRROR_POSITIONER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BIOCAT_MIRROR_POSITIONER pointer for motor "
			"record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_biocat_mirror_pos_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] =
		"mxd_biocat_mirror_pos_initialize_driver()";

	static char field_name[][MXU_FIELD_NAME_LENGTH+1] = {
		"motor_record_array",
		"motor_scale_array",
	};

	static int num_motors = sizeof(field_name) / sizeof(field_name[0]);

	MX_RECORD_FIELD_DEFAULTS *field;
	int i;
	long referenced_field_index;
	long num_motors_varargs_cookie;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_motors",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
			referenced_field_index, 0, &num_motors_varargs_cookie);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < num_motors; i++ ) {
		mx_status = mx_find_record_field_defaults( driver,
						field_name[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_motors_varargs_cookie;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_biocat_mirror_pos_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_biocat_mirror_pos_create_record_structures()";

	MX_MOTOR *motor;
	MX_BIOCAT_MIRROR_POSITIONER *biocat_mirror_pos;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	biocat_mirror_pos = (MX_BIOCAT_MIRROR_POSITIONER *)
				malloc( sizeof(MX_BIOCAT_MIRROR_POSITIONER) );

	if ( biocat_mirror_pos == (MX_BIOCAT_MIRROR_POSITIONER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_BIOCAT_MIRROR_POSITIONER structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = biocat_mirror_pos;
	record->class_specific_function_list
				= &mxd_biocat_mirror_pos_motor_function_list;

	motor->record = record;
	biocat_mirror_pos->record = record;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	/* A BioCAT mirror positioner is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_biocat_mirror_pos_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_biocat_mirror_pos_open()";

	MX_MOTOR *motor;
	MX_BIOCAT_MIRROR_POSITIONER *biocat_mirror_pos;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_biocat_mirror_pos_get_pointers( motor,
						&biocat_mirror_pos, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_biocat_mirror_pos_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_biocat_mirror_pos_move_absolute()";

	MX_BIOCAT_MIRROR_POSITIONER *biocat_mirror_pos;
	MX_RECORD *child_motor_record;
	long i, num_motors;
	double raw_destination, raw_position, raw_delta;
	double child_position, child_destination, child_delta, child_scale;
	mx_status_type mx_status;

	mx_status = mxd_biocat_mirror_pos_get_pointers( motor,
						&biocat_mirror_pos, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_destination = motor->raw_destination.analog;

#if MXD_BIOCAT_MIRROR_POSITIONER_DEBUG
	MX_DEBUG(-2,("%s: moving '%s' to %f",
			fname, motor->record->name, raw_destination));
#endif
	/* Compute the change in the position of the pseudomotor. */

	mx_status = mx_motor_get_position( motor->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_position = motor->raw_position.analog;

	raw_delta = raw_destination - raw_position;

#if MXD_BIOCAT_MIRROR_POSITIONER_DEBUG
	MX_DEBUG(-2,
	("%s: raw_destination = %f, raw_position = %f, raw_delta = %f",
		fname, raw_destination, raw_position, raw_delta));
#endif

	/*---*/

	num_motors = biocat_mirror_pos->num_motors;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = biocat_mirror_pos->motor_record_array[i];

		if ( child_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Motor record %ld for pseudomotor '%s' is NULL.",
				i, motor->record->name );
		}

		child_scale = biocat_mirror_pos->motor_scale_array[i];

		mx_status = mx_motor_get_position( child_motor_record,
							&child_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_BIOCAT_MIRROR_POSITIONER_DEBUG
		MX_DEBUG(-2,
		("%s: child '%s' child_position = %f, child_scale = %f",
			fname, child_motor_record->name,
			child_position, child_scale));
#endif

		child_delta = child_scale * raw_delta;

		child_destination = child_position + child_delta;

#if MXD_BIOCAT_MIRROR_POSITIONER_DEBUG
		MX_DEBUG(-2,("%s: child_delta = %f, child_destination = %f",
			fname, child_delta, child_destination));
#endif

#if 0
		mx_status = mx_motor_move_absolute( child_motor_record,
						child_destination, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_biocat_mirror_pos_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_biocat_mirror_pos_get_position()";

	MX_BIOCAT_MIRROR_POSITIONER *biocat_mirror_pos;
	double raw_position;
	mx_status_type mx_status;

	mx_status = mxd_biocat_mirror_pos_get_pointers( motor,
						&biocat_mirror_pos, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We get the reported position from the first entry in 
	 * the motor array.
	 */

	mx_status = mx_motor_get_position(
				biocat_mirror_pos->motor_record_array[0],
				&raw_position );

#if MXD_BIOCAT_MIRROR_POSITIONER_DEBUG
	MX_DEBUG(-2,("%s: '%s' position = %f",
			fname, motor->record->name, raw_position));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_biocat_mirror_pos_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_biocat_mirror_pos_soft_abort()";

	MX_BIOCAT_MIRROR_POSITIONER *biocat_mirror_pos;
	MX_RECORD *motor_record;
	long i, num_motors;
	mx_status_type mx_status;

	mx_status = mxd_biocat_mirror_pos_get_pointers( motor,
						&biocat_mirror_pos, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BIOCAT_MIRROR_POSITIONER_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s'", fname, motor->record->name ));
#endif

	num_motors = biocat_mirror_pos->num_motors;

	for ( i = 0; i < num_motors; i++ ) {
		motor_record = biocat_mirror_pos->motor_record_array[i];

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Motor record %ld for pseudomotor '%s' is NULL.",
				i, motor->record->name );
		}

		mx_status = mx_motor_soft_abort( motor_record );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_biocat_mirror_pos_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_biocat_mirror_pos_get_status()";

	MX_BIOCAT_MIRROR_POSITIONER *biocat_mirror_pos;
	MX_RECORD *motor_record;
	long i, num_motors;
	unsigned long motor_status, pseudomotor_status;
	mx_status_type mx_status;

	mx_status = mxd_biocat_mirror_pos_get_pointers( motor,
						&biocat_mirror_pos, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pseudomotor_status = 0;

	num_motors = biocat_mirror_pos->num_motors;

	for ( i = 0; i < num_motors; i++ ) {
		motor_record = biocat_mirror_pos->motor_record_array[i];

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Motor record %ld for pseudomotor '%s' is NULL.",
				i, motor->record->name );
		}

		mx_status = mx_motor_get_status( motor_record, &motor_status );

		if ( motor_status & MXSF_MTR_IS_BUSY ) {
			pseudomotor_status |= MXSF_MTR_IS_BUSY;

			/* Break out of the while() loop. */
			break;
		}
	}

#if MXD_BIOCAT_MIRROR_POSITIONER_DEBUG
	MX_DEBUG(-2,("%s: '%s' status = %lx",
		fname, motor->record->name, pseudomotor_status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

