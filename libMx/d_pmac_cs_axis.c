/*
 * Name:    d_pmac_cs_axis.c
 *
 * Purpose: MX driver for a coordinate system axis in a Delta Tau PMAC
 *          motor controller.
 *
 *          This driver expects several things:
 *
 *          1.  The PMAC motion program must read the requested destination
 *              of this axis from a PMAC variable rather than hard coding
 *              the destination.  This variable must be specified in the
 *              MX record field 'destination_variable'.
 *
 *          2.  A PMAC PLC program must be running that computes and writes
 *              the position of the coordinate system axis to a PMAC variable.
 *              This means that the kinematic calculation logic in the PMAC
 *              motion program must be duplicated in the PLC program.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PMAC_CS_AXIS_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "d_pmac_cs_axis.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_pmac_cs_axis_record_function_list = {
	NULL,
	mxd_pmac_cs_axis_create_record_structures,
	mxd_pmac_cs_axis_finish_record_initialization,
	NULL,
	mxd_pmac_cs_axis_print_structure,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pmac_cs_axis_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_pmac_cs_axis_motor_function_list = {
	NULL,
	mxd_pmac_cs_axis_move_absolute,
	mxd_pmac_cs_axis_get_position,
	mxd_pmac_cs_axis_set_position,
	mxd_pmac_cs_axis_soft_abort,
	mxd_pmac_cs_axis_immediate_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_pmac_cs_axis_get_parameter,
	mxd_pmac_cs_axis_set_parameter,
	mxd_pmac_cs_axis_simultaneous_start,
	mxd_pmac_cs_axis_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_pmac_cs_axis_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PMAC_CS_AXIS_STANDARD_FIELDS
};

long mxd_pmac_cs_axis_num_record_fields
		= sizeof( mxd_pmac_cs_axis_record_field_defaults )
			/ sizeof( mxd_pmac_cs_axis_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pmac_cs_axis_rfield_def_ptr
			= &mxd_pmac_cs_axis_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pmac_cs_axis_get_pointers( MX_MOTOR *motor,
			MX_PMAC_COORDINATE_SYSTEM_AXIS **axis,
			MX_PMAC **pmac,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pmac_cs_axis_get_pointers()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *pmac_axis;
	MX_RECORD *pmac_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pmac_axis = (MX_PMAC_COORDINATE_SYSTEM_AXIS *)
			motor->record->record_type_struct;

	if ( axis != (MX_PMAC_COORDINATE_SYSTEM_AXIS **) NULL ) {
		*axis = pmac_axis;
	}

	if ( pmac != (MX_PMAC **) NULL ) {
		pmac_record = pmac_axis->pmac_record;

		if ( pmac_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			    "The pmac_record pointer for record '%s' is NULL.",
				pmac_axis->record->name );
		}

		*pmac = (MX_PMAC *) pmac_record->record_type_struct;

		if ( *pmac == (MX_PMAC *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PMAC pointer for record '%s' is NULL.",
				pmac_axis->record->name );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pmac_cs_axis_create_record_structures()";

	MX_MOTOR *motor;
	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	axis = (MX_PMAC_COORDINATE_SYSTEM_AXIS *)
			malloc( sizeof(MX_PMAC_COORDINATE_SYSTEM_AXIS) );

	if ( axis == (MX_PMAC_COORDINATE_SYSTEM_AXIS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_PMAC_COORDINATE_SYSTEM_AXIS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = axis;
	record->class_specific_function_list
		= &mxd_pmac_cs_axis_motor_function_list;

	motor->record = record;
	axis->record = record;

	/* A PMAC coordinate system axis is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* We express accelerations as acceleration times. */

	motor->acceleration_type = MXF_MTR_ACCEL_TIME;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pmac_cs_axis_finish_record_initialization()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	axis = (MX_PMAC_COORDINATE_SYSTEM_AXIS *) record->record_type_struct;

	if ( ( axis->coordinate_system < 1 )
				|| ( axis->coordinate_system > 32 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal PMAC coordinate_system number %d for record '%s'.  "
		"Allowed range is 1 to 32.",
			axis->coordinate_system, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_cs_axis_print_structure()";

	MX_MOTOR *motor;
	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	long motor_steps;
	double position, backlash;
	double negative_limit, positive_limit, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type           = PMAC coordinate system axis.\n\n");

	fprintf(file, "  name                 = %s\n", record->name);
	fprintf(file, "  port name            = %s\n", axis->pmac_record->name);
	fprintf(file, "  coordinate system    = %d\n", axis->coordinate_system);
	fprintf(file, "  axis name            = %c\n", axis->axis_name);
	fprintf(file, "  position variable    = %s\n",
					axis->position_variable);
	fprintf(file, "  destination variable = %s\n",
					axis->destination_variable);
	fprintf(file, "  move program number  = %d\n",
					axis->move_program_number);

	mx_status = mx_motor_get_position_steps( record, &motor_steps );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
				record->name );
	}
	
	position = motor->offset + motor->scale
			* (double) motor_steps;
	
	fprintf(file, "  position       = %ld steps (%g %s)\n",
			motor_steps, position, motor->units);
	fprintf(file, "  scale          = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset         = %g %s.\n",
			motor->offset, motor->units);
	
	backlash = motor->scale
			* (double) (motor->raw_backlash_correction.analog);
	
	fprintf(file, "  backlash       = %g (%g %s)\n",
			motor->raw_backlash_correction.analog,
			backlash, motor->units);
	
	negative_limit = motor->offset
	+ motor->scale * (double)(motor->raw_negative_limit.analog);
	
	fprintf(file, "  negative limit = %g (%g %s)\n",
			motor->raw_negative_limit.analog,
			negative_limit, motor->units);

	positive_limit = motor->offset
	+ motor->scale * (double)(motor->raw_positive_limit.analog);

	fprintf(file, "  positive limit = %g (%g %s)\n",
			motor->raw_positive_limit.analog,
			positive_limit, motor->units);

	move_deadband = motor->scale * (double)motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband  = %g (%g %s)\n\n",
			motor->raw_move_deadband.analog,
			move_deadband, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_pmac_cs_axis_resynchronize()";

	MX_MOTOR *motor;
	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	mx_status_type mx_status;
	MX_PMAC *pmac;
	char command[20];

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_resynchronize_record( axis->pmac_record );

	/* Add code to fix up open loop and incremental mode problems
	 * seen at SER-CAT
	 *   06/22/04 JF
	 *
	 * 'A' closes the loop on all motors
	 */

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, "A",
					NULL, 0, PMAC_CS_AXIS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, "ABS",
					NULL, 0, PMAC_CS_AXIS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s=%s", axis->position_variable,
					axis->destination_variable);

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, command,
					NULL, 0, PMAC_CS_AXIS_DEBUG );
	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_cs_axis_move_absolute()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	MX_PMAC *pmac;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	/* First, we must see if the coordinate system is currently running
	 * a motion program.  We cannot start a new move if a motion program
	 * is already running.
	 */

	mx_status = mxd_pmac_cs_axis_get_status( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Write the destination to the specified PMAC variable. */

	mx_status = mxd_pmac_cs_axis_set_variable( axis, pmac,
					axis->destination_variable,
					MXFT_DOUBLE,
					&(motor->raw_destination.analog),
					PMAC_CS_AXIS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now start the motion program. */

	sprintf( command, "B%dR", axis->move_program_number );

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, command,
						NULL, 0, PMAC_CS_AXIS_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_cs_axis_get_position_steps()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	MX_PMAC *pmac;
	mx_status_type mx_status;

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pmac_cs_axis_get_variable( axis, pmac,
					axis->position_variable,
					MXFT_DOUBLE,
					&(motor->raw_position.analog),
					PMAC_CS_AXIS_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_cs_axis_soft_abort()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	MX_PMAC *pmac;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a quick stop command. */

	/* Changed from "\\" to "A" JF 06/22/04*/

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, "A",
					NULL, 0, PMAC_CS_AXIS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s=%s", axis->position_variable,
					axis->destination_variable);

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, command,
					NULL, 0, PMAC_CS_AXIS_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_cs_axis_immediate_abort()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	MX_PMAC *pmac;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send an abort command. */

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, "A",
						NULL, 0, PMAC_CS_AXIS_DEBUG );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s=%s", axis->position_variable,
					axis->destination_variable);

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, command,
					NULL, 0, PMAC_CS_AXIS_DEBUG );

	return mx_status;
}

static mx_status_type
mxd_pmac_cs_axis_get_feedrate_time_unit( MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
					MX_PMAC *pmac )
{
	char variable_name[50];
	double coordinate_system_time_unit;
	mx_status_type mx_status;

	/* Get the coordinate system time unit in milliseconds. */

	sprintf( variable_name, "I%d55", 50 + axis->coordinate_system );

	mx_status = mxi_pmac_get_variable( pmac, axis->card_number,
						variable_name,
						MXFT_DOUBLE,
						&coordinate_system_time_unit,
						PMAC_CS_AXIS_DEBUG);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The PMAC expresses times in milliseconds while MX uses seconds,
	 * so we must convert this value into seconds.
	 */

	axis->feedrate_time_unit_in_seconds
				= 0.001 * coordinate_system_time_unit;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pmac_cs_axis_get_velocity( MX_MOTOR *motor,
				MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
				MX_PMAC *pmac )
{
	static const char fname[] = "mxd_pmac_cs_axis_get_velocity()";

	double feedrate_distance;
	mx_status_type mx_status;

	/* Get the feedrate time unit. */

	if ( strlen( axis->feedrate_variable ) == 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the speed of motor '%s' is not currently possible "
		"since the 'feedrate_variable' field is not configured.",
				motor->record->name );
	}

	mx_status = mxd_pmac_cs_axis_get_feedrate_time_unit( axis, pmac );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now get the feedrate itself which is expressed as the
	 * distance moved in one coordinate system time unit.
	 */

	mx_status = mxd_pmac_cs_axis_get_variable( axis, pmac,
					axis->feedrate_variable,
					MXFT_DOUBLE,
					&feedrate_distance,
					PMAC_CS_AXIS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_speed = mx_divide_safely( feedrate_distance,
					axis->feedrate_time_unit_in_seconds );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_pmac_cs_axis_get_acceleration_times( MX_MOTOR *motor,
				MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
				MX_PMAC *pmac )
{
	double acceleration_time_ms;
	double s_curve_acceleration_time_ms;
	mx_status_type mx_status;

	/* Get the PMAC coordinate system acceleration time variables. */

	if ( strlen( axis->acceleration_time_variable ) == 0 ) {
		acceleration_time_ms = 0.0;
	} else {
		mx_status = mxd_pmac_cs_axis_get_variable( axis, pmac,
					axis->acceleration_time_variable,
					MXFT_DOUBLE,
					&acceleration_time_ms,
					PMAC_CS_AXIS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( strlen( axis->s_curve_acceleration_time_var ) == 0 ) {
		s_curve_acceleration_time_ms = 0.0;
	} else {
		mx_status = mxd_pmac_cs_axis_get_variable( axis, pmac,
					axis->s_curve_acceleration_time_var,
					MXFT_DOUBLE,
					&s_curve_acceleration_time_ms,
					PMAC_CS_AXIS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* PMACs express time in units of milliseconds, while MX _always_
	 * uses seconds even for "raw" units.  Thus, we must convert the
	 * times into seconds when storing them.
	 */

	motor->raw_acceleration_parameters[0] = 0.001 * acceleration_time_ms;
	motor->raw_acceleration_parameters[1] = 
					0.001 * s_curve_acceleration_time_ms;
	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_cs_axis_get_parameter()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	MX_PMAC *pmac;
	double accel_time, twice_s_curve_time;
	mx_status_type mx_status;

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mxd_pmac_cs_axis_get_velocity( motor, axis, pmac );
		break;
	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mxd_pmac_cs_axis_get_acceleration_times( motor,
								axis, pmac );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		accel_time = motor->raw_acceleration_parameters[0];
		twice_s_curve_time =
			2.0 * motor->raw_acceleration_parameters[1];

		if ( twice_s_curve_time > accel_time ) {
			motor->acceleration_time = twice_s_curve_time;
		} else {
			motor->acceleration_time = accel_time;
		}
		break;
	default:
		return mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_cs_axis_set_parameter()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	MX_PMAC *pmac;
	double feedrate_distance;
	mx_status_type mx_status;

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		/* Get the feedrate time unit. */

		if ( strlen( axis->feedrate_variable ) == 0 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the speed of motor '%s' is not currently possible "
		"since the 'feedrate_variable' field is not configured.",
				motor->record->name );
		}

		mx_status = mxd_pmac_cs_axis_get_feedrate_time_unit(axis, pmac);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Compute the necessary feedrate distance per time unit. */

		feedrate_distance = motor->raw_speed
					* axis->feedrate_time_unit_in_seconds;

		/* Send the feedrate to the PMAC. */

		mx_status = mxd_pmac_cs_axis_set_variable( axis, pmac,
						axis->feedrate_variable,
						MXFT_DOUBLE,
						&feedrate_distance,
						PMAC_CS_AXIS_DEBUG );
		break;
	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_simultaneous_start( int num_motor_records,
				MX_RECORD **motor_record_array,
				double *position_array,
				int flags )
{
	static const char fname[] = "mxd_pmac_cs_axis_simultaneous_start()";

	MX_RECORD *pmac_interface_record, *motor_record;
	MX_MOTOR *motor;
	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	MX_PMAC *pmac;
	char command[100];
	int i, coordinate_system, move_program_number;
	double raw_destination;
	mx_status_type mx_status;

	if ( num_motor_records <= 0 )
		return MX_SUCCESSFUL_RESULT;

	pmac_interface_record = NULL;
	pmac = NULL;
	axis = NULL;
	coordinate_system = -1;
	move_program_number = -1;

	for ( i = 0; i < num_motor_records; i++ ) {
		motor_record = motor_record_array[i];

		motor = (MX_MOTOR *) motor_record->record_class_struct;

		if ( motor_record->mx_type != MXT_MTR_PMAC_COORDINATE_SYSTEM ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"Cannot perform a simultaneous start since motors "
		"'%s' and '%s' are not both PMAC coordinate system motors.",
				motor_record_array[0]->name,
				motor_record->name );
		}

		axis = (MX_PMAC_COORDINATE_SYSTEM_AXIS *)
				motor_record->record_type_struct;

		/* Verify that the PMAC motor records all belong to the same
		 * PMAC interface.
		 */

		if ( pmac_interface_record == (MX_RECORD *) NULL ) {
			pmac_interface_record = axis->pmac_record;

			pmac = (MX_PMAC *)
				pmac_interface_record->record_type_struct;
		}

		if ( pmac_interface_record != axis->pmac_record ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are controlled by different PMAC interfaces, "
		"namely '%s' and '%s'.",
				motor_record_array[0]->name,
				motor_record->name,
				pmac_interface_record->name,
				axis->pmac_record->name );
		}

		/* Verify that the PMAC motor records all belong to the same
		 * PMAC coordinate system.
		 */

		if ( coordinate_system == -1 ) {
			coordinate_system = axis->coordinate_system;
		}

		if ( coordinate_system != axis->coordinate_system ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they are in different coordinate systems, "
		"namely %d and %d.",
				motor_record_array[0]->name,
				motor_record->name,
				coordinate_system,
				axis->coordinate_system );
		}

		/* Verify that the PMAC motor records all use the same
		 * PMAC motion program number.
		 */

		if ( move_program_number == -1 ) {
			move_program_number = axis->move_program_number;
		}

		if ( move_program_number != axis->move_program_number ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"Cannot perform a simultaneous start for motors '%s' and '%s' "
		"since they use different move program numbers, "
		"namely %d and %d.",
				motor_record_array[0]->name,
				motor_record->name,
				move_program_number,
				axis->move_program_number );
		}

		/* Compute the new position in raw units. */

		raw_destination =
			mx_divide_safely( position_array[i] - motor->offset,
						motor->scale );

		/* Write the raw position to the PMAC destination variable. */

		mx_status = mxd_pmac_cs_axis_set_variable( axis, pmac,
						axis->destination_variable,
						MXFT_DOUBLE,
						&raw_destination,
						PMAC_CS_AXIS_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( pmac_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"No pmac interface record pointer was found for record '%s'.",
		motor_record_array[0]->name );
	}

	/* Tell the PMAC to run the motion program. */

	sprintf( command, "B%dR", move_program_number );

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, command,
						NULL, 0, PMAC_CS_AXIS_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pmac_cs_axis_get_status()";

	MX_PMAC_COORDINATE_SYSTEM_AXIS *axis;
	MX_PMAC *pmac;
	char response[50];
	unsigned long status[ MX_PMAC_CS_NUM_STATUS_CHARACTERS ];
	int i, length;
	mx_status_type mx_status;

	mx_status = mxd_pmac_cs_axis_get_pointers( motor, &axis, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Request the motor status. */

	mx_status = mxd_pmac_cs_axis_command( axis, pmac, "??",
				response, sizeof response, PMAC_CS_AXIS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length < MX_PMAC_CS_NUM_STATUS_CHARACTERS ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Too few characters seen in response to motor status command.  "
	"Only saw %d characters.", length );
	}

	/* Change the reported coordinate system status from ASCII characters
	 * to unsigned long integers.
	 */

	for ( i = 0; i < MX_PMAC_CS_NUM_STATUS_CHARACTERS; i++ ) {
		status[i] = mx_hex_char_to_unsigned_long( response[i] );

		MX_DEBUG( 2,("%s: status[%d] = %#lx",
			fname, i, status[i]));
	}
	
	/* Check for all the status bits that we are interested in.
	 *
	 * I used the Delta Tau Turbo PMAC Software Reference manual
	 * version 1.937 for the following information.  (W. Lavender)
	 */

	motor->status = 0;

	/* ============= First word returned. ============= */

	/* ----------- First character returned. ---------- */

	/* Bits 23 to 20: (ignored) */

	/* ----------- Second character returned. ---------- */

	/* Bits 19 to 16: (ignored) */

	/* ----------- Third character returned. ---------- */

	/* Bits 15 to 12: (ignored) */

	/* ----------- Fourth character returned. ---------- */

	/* Bits 11 to 8: (ignored) */

	/* ----------- Fifth character returned. ---------- */

	/* Bits 7 to 4: (ignored) */

	/* ----------- Sixth character returned. ---------- */

	/* Bits 3 to 1: (ignored) */

	/* Bit 0: Running program */

	if ( status[5] & 0x1 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* ============= Second word returned. ============= */

	/* ----------- Seventh character returned. ---------- */

	/* Bits 23 to 21: (ignored) */

	/* Bit 20: Amplifier Fault Error */

	if ( status[6] & 0x1 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* ----------- Eighth character returned. ---------- */

	/* Bit 19: Fatal Following Error */

	if ( status[7] & 0x8 ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Bit 18: Warning Following Error (ignored) */

	/* Bit 17: In Position (handled below) */

	/* Bit 16: (ignored) */

	/* ----------- Ninth character returned. ---------- */

	/* Bits 15 to 12: (ignored) */

	/* ----------- Tenth character returned. ---------- */

	/* Bits 11 to 9: (ignored) */

	/* Bit 8: Pre-jog Move Flag */

	if ( status[9] & 0x1 ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* ----------- Eleventh character returned. ---------- */

	/* Bits 7 to 4: (ignored) */

	/* ----------- Twelfth character returned. ---------- */

	/* Bits 3 to 0: (ignored) */

	/* ============== Third word returned. ============== */

	/* ---------- Thirteenth character returned. -------- */

	/* Bits 23 to 20: (ignored) */

	/* ---------- Fourteenth character returned. -------- */

	/* Bits 19 to 16: (ignored) */

	/* ---------- Fifteenth character returned. --------- */

	/* Bits 15 to 12: (ignored) */

	/* ---------- Sixteenth character returned. --------- */

	/* Bits 11 to 8: (ignored) */

	/* --------- Seventeenth character returned. -------- */

	/* Bits 7 to 4: (ignored) */

	/* ---------- Eighteenth character returned. -------- */

	/* Bits 3 to 0: (ignored) */

	/* ================================================== */

	if ( motor->status & MXSF_MTR_IS_BUSY ) {

		/* We have already determined that the motor is moving,
		 * so we do not need to look at the In Position flag.
		 */

	} else if ( status[7] & 0x2 ) {

		/* If In Position is set, then no move is in progress. */

		motor->status &= ( ~ MXSF_MTR_IS_BUSY );
	} else {
		/* If In Position is _not_ set, but an error flag is set,
		 * then no move is in progress.
		 */

		if ( motor->status & MXSF_MTR_FOLLOWING_ERROR ) {

			motor->status &= ( ~ MXSF_MTR_IS_BUSY );
		} else
		if ( motor->status & MXSF_MTR_DRIVE_FAULT ) {

			motor->status &= ( ~ MXSF_MTR_IS_BUSY );
		} else
		if ( motor->status & MXSF_MTR_AXIS_DISABLED ) {

			motor->status &= ( ~ MXSF_MTR_IS_BUSY );
		} else
		if ( motor->status & MXSF_MTR_OPEN_LOOP ) {

			motor->status &= ( ~ MXSF_MTR_IS_BUSY );
		} else {
			/* If we get to here, then we declare a move
			 * to be in progress.
			 */

			motor->status |= MXSF_MTR_IS_BUSY;
		}
	}

#if 0
	MX_DEBUG(-2,("%s: motor->status = %#lx", fname, motor->status));

	MX_DEBUG(-2,("%s: IS_BUSY               = %lu", fname,
			motor->status & MXSF_MTR_IS_BUSY ));

	MX_DEBUG(-2,("%s: POSITIVE_LIMIT_HIT    = %lu", fname,
			motor->status & MXSF_MTR_POSITIVE_LIMIT_HIT ));

	MX_DEBUG(-2,("%s: NEGATIVE_LIMIT_HIT    = %lu", fname,
			motor->status & MXSF_MTR_NEGATIVE_LIMIT_HIT ));

	MX_DEBUG(-2,("%s: HOME_SEARCH_SUCCEEDED = %lu", fname,
			motor->status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ));

	MX_DEBUG(-2,("%s: FOLLOWING_ERROR       = %lu", fname,
			motor->status & MXSF_MTR_FOLLOWING_ERROR ));

	MX_DEBUG(-2,("%s: DRIVE_FAULT           = %lu", fname,
			motor->status & MXSF_MTR_DRIVE_FAULT ));

	MX_DEBUG(-2,("%s: AXIS_DISABLED         = %lu", fname,
			motor->status & MXSF_MTR_AXIS_DISABLED ));

	MX_DEBUG(-2,("%s: OPEN_LOOP             = %lu", fname,
			motor->status & MXSF_MTR_OPEN_LOOP ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-----------*/

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_command( MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
			MX_PMAC *pmac,
			char *command,
			char *response,
			int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_pmac_cs_axis_command()";

	char command_buffer[100];
	mx_status_type mx_status;

	if ( axis == (MX_PMAC_COORDINATE_SYSTEM_AXIS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC_COORDINATE_SYSTEM_AXIS pointer passed is NULL." );
	}
	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC pointer passed is NULL." );
	}

	if ( pmac->num_cards > 1 ) {
		sprintf( command_buffer, "@%x&%d%s",
			axis->card_number, axis->coordinate_system, command );
	} else {
		sprintf( command_buffer, "&%d%s",
				axis->coordinate_system, command );
	}

	mx_status = mxi_pmac_command( pmac, command_buffer,
				response, response_buffer_length, debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_get_variable( MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
				MX_PMAC *pmac,
				char *variable_name,
				int variable_type,
				void *variable_ptr,
				int debug_flag )
{
	static const char fname[] = "mxd_pmac_cs_axis_get_variable()";

	char command_buffer[100];
	char response[100];
	int num_items;
	long long_value;
	long *long_ptr;
	double double_value;
	double *double_ptr;
	mx_status_type mx_status;

	if ( axis == (MX_PMAC_COORDINATE_SYSTEM_AXIS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC_COORDINATE_SYSTEM_AXIS pointer passed is NULL." );
	}
	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC pointer passed was NULL." );
	}
	if ( variable_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The variable pointer passed was NULL." );
	}

	if ( pmac->num_cards > 1 ) {
		sprintf( command_buffer, "@%x&%d%s",
		    axis->card_number, axis->coordinate_system, variable_name );
	} else {
		sprintf( command_buffer, "&%d%s",
				axis->coordinate_system, variable_name );
	}

	mx_status = mxi_pmac_command( pmac, command_buffer,
				response, sizeof response, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( variable_type ) {
	case MXFT_LONG:
		num_items = sscanf( response, "%ld", &long_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The response from PMAC '%s', card %d for the "
				"command '%s' is not an integer number.  "
				"Response = '%s'",
				pmac->record->name, axis->card_number,
				command_buffer, response );
		}
		long_ptr = (long *) variable_ptr;

		*long_ptr = long_value;
		break;
	case MXFT_DOUBLE:
		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The response from PMAC '%s', card %d for the "
				"command '%s' is not a number.  "
				"Response = '%s'",
				pmac->record->name, axis->card_number,
				command_buffer, response );
		}
		double_ptr = (double *) variable_ptr;

		*double_ptr = double_value;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG and MXFT_DOUBLE are supported." );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxd_pmac_cs_axis_set_variable( MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
				MX_PMAC *pmac,
				char *variable_name,
				int variable_type,
				void *variable_ptr,
				int debug_flag )
{
	static const char fname[] = "mxd_pmac_cs_axis_set_variable()";

	char command_buffer[100];
	char response[100];
	char *ptr;
	long *long_ptr;
	double *double_ptr;
	mx_status_type mx_status;

	if ( axis == (MX_PMAC_COORDINATE_SYSTEM_AXIS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC_COORDINATE_SYSTEM_AXIS pointer passed is NULL." );
	}
	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC pointer passed was NULL." );
	}
	if ( variable_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The variable pointer passed was NULL." );
	}

	if ( pmac->num_cards > 1 ) {
		sprintf( command_buffer, "@%x&%d%s=",
		    axis->card_number, axis->coordinate_system, variable_name );
	} else {
		sprintf( command_buffer, "&%d%s=",
				axis->coordinate_system, variable_name );
	}

	ptr = command_buffer + strlen( command_buffer );

	switch( variable_type ) {
	case MXFT_LONG:
		long_ptr = (long *) variable_ptr;

		sprintf( ptr, "%ld", *long_ptr );
		break;
	case MXFT_DOUBLE:
		double_ptr = (double *) variable_ptr;

		sprintf( ptr, "%f", *double_ptr );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG and MXFT_DOUBLE are supported." );
		break;
	}

	mx_status = mxi_pmac_command( pmac, command_buffer,
				response, sizeof response, debug_flag );

	return mx_status;
}

