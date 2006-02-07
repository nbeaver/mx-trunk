/*
 * Name:    d_pcstep.c
 *
 * Purpose: MX motor driver for National Instruments PC-Step motor
 *          controllers (formerly made by nuLogic).
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "d_pcstep.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_pcstep_record_function_list = {
	mxd_pcstep_initialize_type,
	mxd_pcstep_create_record_structures,
	mxd_pcstep_finish_record_initialization,
	mxd_pcstep_delete_record,
	mxd_pcstep_print_structure,
	mxd_pcstep_read_parms_from_hardware,
	mxd_pcstep_write_parms_to_hardware,
	mxd_pcstep_open,
	mxd_pcstep_close,
	NULL,
	mxd_pcstep_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_pcstep_motor_function_list = {
	mxd_pcstep_motor_is_busy,
	mxd_pcstep_move_absolute,
	mxd_pcstep_get_position,
	mxd_pcstep_set_position,
	mxd_pcstep_soft_abort,
	mxd_pcstep_immediate_abort,
	mxd_pcstep_positive_limit_hit,
	mxd_pcstep_negative_limit_hit,
	mxd_pcstep_find_home_position,
	mxd_pcstep_constant_velocity_move,
	mxd_pcstep_get_parameter,
	mxd_pcstep_set_parameter
};

/* PC-Step motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pcstep_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PCSTEP_MOTOR_STANDARD_FIELDS
};

mx_length_type mxd_pcstep_num_record_fields
		= sizeof( mxd_pcstep_record_field_defaults )
			/ sizeof( mxd_pcstep_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pcstep_rfield_def_ptr
			= &mxd_pcstep_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pcstep_get_pointers( MX_MOTOR *motor,
				MX_PCSTEP_MOTOR **pcstep_motor,
				MX_PCSTEP **pcstep,
				const char *calling_fname )
{
	const char fname[] = "mxd_pcstep_get_pointers()";

	MX_RECORD *pcstep_controller_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pcstep_motor == (MX_PCSTEP_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCSTEP_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pcstep == (MX_PCSTEP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCSTEP pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pcstep_motor = (MX_PCSTEP_MOTOR *) motor->record->record_type_struct;

	if ( *pcstep_motor == (MX_PCSTEP_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PCSTEP_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	pcstep_controller_record = (*pcstep_motor)->controller_record;

	if ( pcstep_controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The PC-Step controller record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*pcstep = (MX_PCSTEP *) pcstep_controller_record->record_type_struct;

	if ( *pcstep == (MX_PCSTEP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_PCSTEP pointer for controller '%s' containing motor '%s' is NULL.",
			pcstep_controller_record->name,
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_pcstep_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_pcstep_create_record_structures()";

	MX_MOTOR *motor;
	MX_PCSTEP_MOTOR *pcstep_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	pcstep_motor = (MX_PCSTEP_MOTOR *) malloc( sizeof(MX_PCSTEP_MOTOR) );

	if ( pcstep_motor == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PCSTEP_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = pcstep_motor;
	record->class_specific_function_list
			= &mxd_pcstep_motor_function_list;

	motor->record = record;

	/* A PC-Step motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_pcstep_finish_record_initialization()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	int axis_id;
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	pcstep_motor = (MX_PCSTEP_MOTOR *) record->record_type_struct;

	if ( pcstep_motor == (MX_PCSTEP_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PCSTEP_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	axis_id = pcstep_motor->axis_id;

	if ( (axis_id < 1) || (axis_id > 4) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"PC-Step motor axis number %d is outside the allowed range of 1 to 4.",
			axis_id );
	}

	if ( pcstep_motor->controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The controller_record pointer for motor '%s' is NULL.",
			record->name );
	}

	pcstep = (MX_PCSTEP *)
			pcstep_motor->controller_record->record_type_struct;

	if ( pcstep == (MX_PCSTEP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PCSTEP pointer for controller '%s' used by motor '%s' is NULL.",
			pcstep_motor->controller_record->name,
			record->name );
	}

	pcstep->motor_array[ axis_id - 1 ] = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_delete_record( MX_RECORD *record )
{
	const char fname[] = "mxd_pcstep_delete_record()";

	MX_PCSTEP *pcstep;
	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_RECORD *motor_record_ptr;
	int axis_id;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	pcstep_motor = (MX_PCSTEP_MOTOR *) record->record_type_struct;

	if ( pcstep_motor != NULL ) {

	    /* Carefully delete the reference to this motor in the controller's
	     * motor_array structure.
	     */

	    axis_id = pcstep_motor->axis_id;

	    if ( (axis_id >= 1) && (axis_id <= 4) ) {

		if ( pcstep_motor->controller_record != NULL ) {

		    pcstep = (MX_PCSTEP *)
			pcstep_motor->controller_record->record_type_struct;

		    if ( pcstep != NULL ) {
			motor_record_ptr = pcstep->motor_array[ axis_id - 1 ];

			if ( record != motor_record_ptr ) {
			    (void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"While deleting motor '%s', we found that it was not correctly recorded in "
"the motor_array for its controller '%s'.", record->name,
				pcstep_motor->controller_record->name );
			}

			pcstep->motor_array[ axis_id - 1 ] = NULL;
		    }
		}
	    }

	    free( record->record_type_struct );

	    record->record_type_struct = NULL;
	}

	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_pcstep_print_structure()";

	MX_MOTOR *motor;
	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = PCSTEP motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  controller record = %s\n", pcstep->record->name);
	fprintf(file, "  axis id           = %d\n\n", pcstep_motor->axis_id);

	fprintf(file, "  default speed        = %lu steps per sec\n",
			(unsigned long) pcstep_motor->default_speed);

	fprintf(file, "  default base speed   = %lu steps per sec\n",
			(unsigned long) pcstep_motor->default_base_speed);

	fprintf(file, "  default acceleration = %lu steps per sec^2\n",
			(unsigned long) pcstep_motor->default_acceleration);

	fprintf(file, "  default accel factor = %lu\n",
		(unsigned long) pcstep_motor->default_acceleration_factor);

	if ( pcstep_motor->lines_per_revolution > 0 ) {
		fprintf(file,
		      "  lines per revolution = %hu\n",
				pcstep_motor->lines_per_revolution);
		fprintf(file,
		      "  steps per revolution = %hu\n",
				pcstep_motor->steps_per_revolution);
	}
	fprintf(file, "\n");

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}
	
	fprintf(file, "  position          = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			(long) motor->raw_position.stepper );

	fprintf(file, "  scale             = %g %s per step.\n",
			motor->scale, motor->units);

	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		(long) motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit    = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		(long) motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit    = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		(long) motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband     = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		(long) motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_write_parms_to_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_pcstep_write_parms_to_hardware()";

	MX_MOTOR *motor;
	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	double speed, base_speed;
	double raw_acceleration_parameters[MX_MOTOR_NUM_ACCELERATION_PARAMS];
	uint32_t steps_and_lines_factor;
	uint16_t stop_mode_word;
	int n;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	n = pcstep_motor->axis_id;

	/* Initialize the motor using the initialization procedure 
	 * recommended in the manual.
	 */

	/* Set the motor position mode to absolute. */

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_SET_OPERATION_MODE(n),
				0, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the motor stop mode to 'decelerate to stop'. */

	stop_mode_word = 0x0101;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_SET_STOP_MODE(n),
				stop_mode_word, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If lines_per_revolution has a non-zero value, we assume that
	 * the controller will be using closed loop encoder feedback.
	 */

	if ( pcstep_motor->lines_per_revolution != 0 ) {

		/* Set the controller loop mode.  This turns on closed
		 * loop feedback.
		 */

		mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_SET_LOOP_MODE(n),
				0xFFFF, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Load steps and lines per revolution. */

		steps_and_lines_factor = pcstep_motor->steps_per_revolution;

		steps_and_lines_factor &= 0xFFFF;

		steps_and_lines_factor
			|= ( pcstep_motor->lines_per_revolution << 16 );

		mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_LOAD_STEPS_AND_LINES_PER_REV(n),
				steps_and_lines_factor, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the motor speed. */

	speed = fabs( motor->scale ) * (double) pcstep_motor->default_speed;

	mx_status = mx_motor_set_speed( record, speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the motor base speed. */

	base_speed = fabs( motor->scale )
			* (double) pcstep_motor->default_base_speed;

	mx_status = mx_motor_set_base_speed( record, base_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the acceleration parameters. */

	raw_acceleration_parameters[0]
			= (double) pcstep_motor->default_acceleration;

	raw_acceleration_parameters[1]
			= (double) pcstep_motor->default_acceleration_factor;

	raw_acceleration_parameters[2] = 0.0;
	raw_acceleration_parameters[3] = 0.0;

	mx_status = mx_motor_set_raw_acceleration_parameters( record,
						raw_acceleration_parameters );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pcstep_open( MX_RECORD *record )
{
	const char fname[] = "mxd_pcstep_open()";

	MX_MOTOR *motor;
	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_pcstep_resynchronize()";

	MX_MOTOR *motor;
	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	mx_status_type mx_status;

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pcstep_resynchronize( pcstep->record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_pcstep_motor_is_busy( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_motor_is_busy()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t long_axis_status;
	uint16_t axis_status;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	n = pcstep_motor->axis_id;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_READ_PER_AXIS_HW_STATUS(n),
				0, &long_axis_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	axis_status = (uint16_t) long_axis_status;

	motor->busy = FALSE;

	/* Check the run/stop status bit. */

	if ( axis_status & 0x0080 ) {
		motor->busy = TRUE;
	}

	/* Check the profile complete bit and the motor on/off bit. */

	if ( (axis_status & 0x0404) == 0 ) {
		motor->busy = TRUE;
	}

	if ( motor->busy == FALSE ) {
		if ( pcstep->home_search_in_progress == pcstep_motor->axis_id )
		{
			pcstep->home_search_in_progress = 0;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_move_absolute()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t mode_control_data_word;
	uint32_t axis_hardware_status;
	uint32_t limit_switch_status;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pcstep->home_search_in_progress != 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"Cannot perform a move command for motor '%s' since a home search is "
	"currently in progress for axis %d of controller '%s'.",
			motor->record->name,
			pcstep->home_search_in_progress,
			pcstep->record->name );
	}

	/* Select absolute position mode. */

	n = pcstep_motor->axis_id;

	mode_control_data_word = 0;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_SET_OPERATION_MODE(n),
				mode_control_data_word, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the destination. */

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_LOAD_TARGET_POSITION(n),
				motor->raw_destination.stepper, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the move. */

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_START_MOTION(n), 0, NULL );

	/* We are done unless we received an MXE_INTERFACE_ACTION_FAILED
	 * status code.
	 */

	if ( mx_status.code != MXE_INTERFACE_ACTION_FAILED )
		return mx_status;

	/*****************************************************************
	 * The rest of this function merely consists of trying to figure *
	 * out why we got an MXE_INTERFACE_ACTION_FAILED status code.    *
	 *****************************************************************/

	/* Clear the error by sending a stop command to the motor. */

	mx_status = mxi_pcstep_command( pcstep,
			MX_PCSTEP_STOP_MOTION(n), 0, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we got an MXE_INTERFACE_ACTION_FAILED status code, then for some
	 * reason the controller intentionally refused to start the motor.
	 * Fetch the per axis hardware status to try to figure out what
	 * happened.
	 */

	mx_status = mxi_pcstep_command( pcstep,
			MX_PCSTEP_READ_PER_AXIS_HW_STATUS(n), 0,
			&axis_hardware_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if a limit switch is tripped. */

	if ( axis_hardware_status & 0x01 ) {

		/* See which limit switches were tripped. */

		mx_status = mxi_pcstep_command( pcstep,
			MX_PCSTEP_READ_LIMIT_SWITCH_STATUS, 0,
			&limit_switch_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Select the limit switch bits for this axis. */

		limit_switch_status >>= (2*(4-n));

		limit_switch_status &= 0x03;

		if ( limit_switch_status & 0x02 ) {
			if ( limit_switch_status & 0x01 ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested move of motor '%s' was not performed since both the "
	"positive and negative limit switches are tripped.",
					motor->record->name );
			} else {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested move of motor '%s' was not performed since the "
	"positive limit switch is tripped.", motor->record->name );
			}
		} else if ( limit_switch_status & 0x01 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The requested move of motor '%s' was not performed since the "
	"negative limit switch is tripped.", motor->record->name );
		}

		/* Neither limit switch shows itself to be tripped? */

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"The requested move of motor '%s' was not performed since the "
	"motor controller says that a limit switch was tripped.  However, "
	"no limit switch is currently tripped.  Perhaps it was only a "
	"momentary trip.", motor->record->name );
	}

	/* Check for a position error. */

	if ( axis_hardware_status & 0x20 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"The motor controller for motor '%s' says that it has a position error.",
			motor->record->name );
	}

	/* Check for a wraparound count error. */

	if ( axis_hardware_status & 0x10 ) {
		return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
"The position value for motor '%s' has either overflowed or underflowed.",
			motor->record->name );
	}

	/* The rest of the bits do not seem to correspond to error conditions,
	 * so if we get here, I do not know why.  Give up and report this
	 * fact to the user.
	 */

	return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"The requested move of motor '%s' was not performed, but the "
	"reason for this has not been determined.  Axis status = %#6.4lx",
		motor->record->name, (unsigned long) axis_hardware_status );
}

MX_EXPORT mx_status_type
mxd_pcstep_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_get_position()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t current_position;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	n = pcstep_motor->axis_id;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_READ_POSITION(n), 0,
				&current_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.stepper = (long) current_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_set_position()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pcstep->home_search_in_progress != 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"Cannot redefine the position of motor '%s' since a home search is "
	"currently in progress for axis %d of controller '%s'.",
			motor->record->name,
			pcstep->home_search_in_progress,
			pcstep->record->name );
	}

	if ( motor->raw_set_position.stepper != 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"PC-Step motor '%s' cannot have its position set to a value other than zero.",
			motor->record->name );
	}

	n = pcstep_motor->axis_id;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_RESET_POSITION_COUNTER_TO_ZERO(n),
				0, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pcstep_soft_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_soft_abort()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	n = pcstep_motor->axis_id;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_STOP_MOTION(n), 0, NULL );

	if ( pcstep->home_search_in_progress == pcstep_motor->axis_id ) {
		pcstep->home_search_in_progress = 0;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pcstep_immediate_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_immediate_abort()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	n = pcstep_motor->axis_id;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_KILL_MOTION(n), 0, NULL );

	if ( pcstep->home_search_in_progress == pcstep_motor->axis_id ) {
		pcstep->home_search_in_progress = 0;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pcstep_positive_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_positive_limit_hit()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t limit_switch_status, limit_bit;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_READ_LIMIT_SWITCH_STATUS, 0,
				&limit_switch_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	n = pcstep_motor->axis_id;

	limit_bit = limit_switch_status & ( 1 << (2*(4-n) + 1) );

	if ( limit_bit ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_negative_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_negative_limit_hit()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t limit_switch_status, limit_bit;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_READ_LIMIT_SWITCH_STATUS, 0,
				&limit_switch_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	n = pcstep_motor->axis_id;

	limit_bit = limit_switch_status & ( 1 << (2*(4-n)) );

	if ( limit_bit ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_find_home_position()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t direction_bits;
	unsigned long bit_mask, home_switch_settings, limit_switch_settings;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Are home switches enabled for this axis? */

	home_switch_settings = ( pcstep->enable_limit_switches >> 8 ) & 0xff;

	bit_mask = 1 << ( 4 - pcstep_motor->axis_id );

	if ( (bit_mask & home_switch_settings) == 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
			"The home switch for motor '%s' is not enabled.",
			motor->record->name );
	}

	/* Are both limit switches enabled for this axis? */

	limit_switch_settings = pcstep->enable_limit_switches & 0xff;

	bit_mask = 3 << ( 2 * (4 - pcstep_motor->axis_id ) );

	if ( (bit_mask & limit_switch_settings) == 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
		"One or both limit switches for motor '%s' are not enabled.",
			motor->record->name );
	}

	/* Is a home search in progress? */

	if ( pcstep->home_search_in_progress != 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"Cannot perform a home search on motor '%s' since a home search is "
	"already in progress for axis %d of controller '%s'.",
			motor->record->name,
			pcstep->home_search_in_progress,
			pcstep->record->name );
	}

	if ( motor->home_search >= 0 ) {
		direction_bits = 0;
	} else {
		direction_bits = 0xFFFF;
	}

	n = pcstep_motor->axis_id;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_FIND_HOME(n), direction_bits, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save the axis number for axis performing the home search. */

	pcstep->home_search_in_progress = pcstep_motor->axis_id;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_constant_velocity_move( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_constant_velocity_move()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t mode_control_data_word, direction;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pcstep->home_search_in_progress != 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"Cannot change the speed of motor '%s' since a home search is "
	"currently in progress for axis %d of controller '%s'.",
			motor->record->name,
			pcstep->home_search_in_progress,
			pcstep->record->name );
	}

	/* Select velocity mode. */

	n = pcstep_motor->axis_id;

	mode_control_data_word = 0xFF00;

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_SET_OPERATION_MODE(n),
				mode_control_data_word, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the destination. */

	if ( motor->constant_velocity_move >= 0 ) {
		direction = 1;
	} else {
		direction = -1;
	}

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_LOAD_TARGET_POSITION(n),
				direction, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the move. */

	mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_START_MOTION(n), 0, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pcstep_get_parameter( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_get_parameter()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t returned_value;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0 /* FIXME: This driver should be rewritten to use the motor->status
       * variable since things like motor->home_search_succeeded are
       * going away.
       */

	if ( motor->parameter_type != MXLV_MTR_HOME_SEARCH_SUCCEEDED ) {
		if ( pcstep->home_search_in_progress != 0 ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
	"Cannot get motor parameters for motor '%s' since a home search is "
	"currently in progress for axis %d of controller '%s'.",
				motor->record->name,
				pcstep->home_search_in_progress,
				pcstep->record->name );
		}
	}
#endif

	n = pcstep_motor->axis_id;

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_READ_STEPS_PER_SECOND(n), 0,
				&returned_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_speed = (double) returned_value;

	} else if ( motor->parameter_type == MXLV_MTR_BASE_SPEED ) {

		/* You cannot read back the base velocity.  You can only
		 * set it.  Thus, we return the value already in
		 * motor->raw_base_speed, since presumably that was set
		 * the last time we set the base speed.
		 */

	} else if ( motor->parameter_type == MXLV_MTR_MAXIMUM_SPEED ) {

		motor->raw_maximum_speed = 750000;  /* steps per second */

	} else if ( motor->parameter_type == 
			MXLV_MTR_RAW_ACCELERATION_PARAMETERS )
	{

		/* You cannot read back the acceleration parameters.  You
		 * can only set them.  Thus, we return the value already in
		 * motor->raw_acceleration_parameters, since presumably that
		 * was set the last time we set the acceleration parameters.
		 */

#if 0 /* FIXME: This driver should be rewritten to use the motor->status
       * variable since things like motor->home_search_succeeded are
       * going away.
       */

	} else if ( motor->parameter_type == MXLV_MTR_HOME_SEARCH_SUCCEEDED ) {

		uint16_t axis_status;

		mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_READ_PER_AXIS_HW_STATUS(n),
				0, &returned_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		axis_status = (uint16_t) returned_value;

		if ( axis_status & 0x0002 ) {
			motor->home_search_succeeded = TRUE;
		} else {
			motor->home_search_succeeded = FALSE;
		}
#endif

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcstep_set_parameter( MX_MOTOR *motor )
{
	const char fname[] = "mxd_pcstep_set_parameter()";

	MX_PCSTEP_MOTOR *pcstep_motor;
	MX_PCSTEP *pcstep;
	uint32_t value_to_send;
	int n;
	mx_status_type mx_status;

	mx_status = mxd_pcstep_get_pointers( motor,
					&pcstep_motor, &pcstep, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pcstep->home_search_in_progress != 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
	"Cannot set motor parameters for motor '%s' since a home search is "
	"currently in progress for axis %d of controller '%s'.",
			motor->record->name,
			pcstep->home_search_in_progress,
			pcstep->record->name );
	}

	n = pcstep_motor->axis_id;

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		value_to_send = mx_round( motor->raw_speed );

		mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_LOAD_STEPS_PER_SECOND(n),
				value_to_send, NULL );

	} else if ( motor->parameter_type == MXLV_MTR_BASE_SPEED ) {

		value_to_send = mx_round( motor->raw_base_speed );

		mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_SET_BASE_VELOCITY(n),
				value_to_send, NULL );

	} else if ( motor->parameter_type == MXLV_MTR_MAXIMUM_SPEED ) {

		motor->raw_maximum_speed = 750000;  /* steps per second */

		return MX_SUCCESSFUL_RESULT;

	} else if ( motor->parameter_type ==
			MXLV_MTR_RAW_ACCELERATION_PARAMETERS )
	{

		/* First, set the acceleration. */

		value_to_send = 
			mx_round( motor->raw_acceleration_parameters[0] );

		mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_LOAD_ACCELERATION(n),
				value_to_send, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Then, set the acceleration factor. */

		value_to_send =
			mx_round( motor->raw_acceleration_parameters[1] );

		mx_status = mxi_pcstep_command( pcstep,
				MX_PCSTEP_LOAD_ACCELERATION_FACTOR(n),
				value_to_send, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
	}

	return mx_status;
}

