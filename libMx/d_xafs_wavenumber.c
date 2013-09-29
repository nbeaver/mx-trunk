/*
 * Name:    d_xafs_wavenumber.c 
 *
 * Purpose: MX motor driver to control an MX motor in units of
 *          XAFS electron wavenumber.
 *
 *          The XAFS electron wavenumber 'k' is computed using the equation:
 *
 *                                  2  2
 *                            (hbar)  k
 *          E       = E     + ----------
 *           photon    edge      2 m
 *                                  electron
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2006-2007, 2013 Illinois Institute of Technology
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
#include "mx_array.h"
#include "mx_motor.h"
#include "mx_variable.h"
#include "d_xafs_wavenumber.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_xafswn_motor_record_function_list = {
	NULL,
	mxd_xafswn_motor_create_record_structures,
	mxd_xafswn_motor_finish_record_initialization,
	NULL,
	mxd_xafswn_motor_print_structure
};

MX_MOTOR_FUNCTION_LIST mxd_xafswn_motor_motor_function_list = {
	mxd_xafswn_motor_motor_is_busy,
	mxd_xafswn_motor_move_absolute,
	mxd_xafswn_motor_get_position,
	mxd_xafswn_motor_set_position,
	mxd_xafswn_motor_soft_abort,
	mxd_xafswn_motor_immediate_abort,
	mxd_xafswn_motor_positive_limit_hit,
	mxd_xafswn_motor_negative_limit_hit,
	mxd_xafswn_motor_raw_home_command
};

/* XAFS electron wavenumber motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_xafswn_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_XAFSWN_MOTOR_STANDARD_FIELDS
};

long mxd_xafswn_motor_num_record_fields
		= sizeof( mxd_xafswn_motor_record_field_defaults )
			/ sizeof( mxd_xafswn_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_xafswn_motor_rfield_def_ptr
			= &mxd_xafswn_motor_record_field_defaults[0];

/* === */

MX_EXPORT mx_status_type
mxd_xafswn_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_xafswn_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_XAFS_WAVENUMBER_MOTOR *xafswn_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	xafswn_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
				malloc( sizeof(MX_XAFS_WAVENUMBER_MOTOR) );

	if ( xafswn_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_XAFS_WAVENUMBER_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = xafswn_motor;
	record->class_specific_function_list
				= &mxd_xafswn_motor_motor_function_list;

	motor->record = record;
	xafswn_motor->record = record;

	/* A XAFS wavenumber motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_finish_record_initialization( MX_RECORD *record )
{
	MX_MOTOR *motor;
	MX_XAFS_WAVENUMBER_MOTOR *xafswn_motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	xafswn_motor = (MX_XAFS_WAVENUMBER_MOTOR *) record->record_type_struct;

	motor->real_motor_record = xafswn_motor->energy_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_xafswn_motor_print_structure()";

	MX_MOTOR *motor;
	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file,"  Motor type               = XAFS_WAVENUMBER_MOTOR.\n\n");

	fprintf(file,"  name                     = %s\n", record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file,"  position                 = %g %s  (%g).\n",
		motor->position, motor->units, motor->raw_position.analog );
	fprintf(file,"  XAFS wavenumber scale    = %g %s per unscaled unit.\n",
		motor->scale, motor->units);
	fprintf(file,"  XAFS wavenumber offset   = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file,"  backlash                 = %g %s  (%g).\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog);
	fprintf(file,"  negative limit           = %g %s  (%g).\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog);
	fprintf(file,"  positive limit           = %g %s  (%g).\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file,"  move deadband            = %g %s  (%g).\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_motor_is_busy()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	mx_bool_type busy;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Energy motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_is_busy( energy_motor_record, &busy );

	motor->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_move_absolute()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	double energy, edge_energy, xafs_wavenumber;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Energy motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	xafs_wavenumber = motor->raw_destination.analog;

	mx_status = mxd_xafswn_motor_get_edge_energy(
				xafs_wavenumber_motor, &edge_energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy = edge_energy + MX_HBAR_SQUARED_OVER_2M_ELECTRON *
				xafs_wavenumber * xafs_wavenumber;

	mx_status = mx_motor_move_absolute( energy_motor_record, energy, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_get_position()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	double energy, edge_energy;
	double xafs_wavenumber, sqrt_argument;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer of energy motor for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mxd_xafswn_motor_get_edge_energy(
				xafs_wavenumber_motor, &edge_energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( energy_motor_record, &energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sqrt_argument = (energy - edge_energy)
				/ MX_HBAR_SQUARED_OVER_2M_ELECTRON;

	if ( sqrt_argument <= 0.0 ) {
		xafs_wavenumber = 0.0;
	} else {
		xafs_wavenumber = sqrt( sqrt_argument );
	}

	motor->raw_position.analog = xafs_wavenumber;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_set_position()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	double energy, edge_energy, xafs_wavenumber;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed NULL." );
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Energy motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	xafs_wavenumber = motor->raw_set_position.analog;

	mx_status = mxd_xafswn_motor_get_edge_energy(
					xafs_wavenumber_motor, &edge_energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	energy = edge_energy + MX_HBAR_SQUARED_OVER_2M_ELECTRON *
				xafs_wavenumber * xafs_wavenumber;

	mx_status = mx_motor_set_position( energy_motor_record, energy );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_soft_abort()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Energy motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_soft_abort( energy_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_immediate_abort()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Energy motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_immediate_abort( energy_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_positive_limit_hit()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	mx_bool_type energy_limit_hit;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Energy motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_positive_limit_hit(
				energy_motor_record, &energy_limit_hit);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( energy_limit_hit ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_negative_limit_hit()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	mx_bool_type energy_limit_hit;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Energy motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_negative_limit_hit(
				energy_motor_record, &energy_limit_hit);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( energy_limit_hit ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xafswn_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_xafswn_motor_raw_home_command()";

	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor;
	MX_RECORD *energy_motor_record;
	long direction;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	xafs_wavenumber_motor = (MX_XAFS_WAVENUMBER_MOTOR *)
					(motor->record->record_type_struct);

	if ( xafs_wavenumber_motor == (MX_XAFS_WAVENUMBER_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_WAVENUMBER_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	energy_motor_record = xafs_wavenumber_motor->energy_motor_record;

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Energy motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	direction = motor->raw_home_command;

	mx_status = mx_motor_raw_home_command( energy_motor_record,
							direction );

	return mx_status;
}

/*************************************************************************/

/* The following function _assumes_ that the "edge_energy" record is found
 * in the local MX_RECORD linked list.  This _MUST_ be changed once the
 * client / server code is running, since that assumption will no longer
 * be true in general.
 */

MX_EXPORT mx_status_type
mxd_xafswn_motor_get_edge_energy(
	MX_XAFS_WAVENUMBER_MOTOR *xafs_wavenumber_motor,
	double *edge_energy )
{
	MX_RECORD *record_list_head;
	mx_status_type mx_status;

	record_list_head = xafs_wavenumber_motor->record->list_head;

	mx_status = mx_get_double_variable_by_name( record_list_head,
			"edge_energy", edge_energy );

	return mx_status;
}

