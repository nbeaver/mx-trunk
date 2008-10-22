/*
 * Name:    d_pcmotion32.c
 *
 * Purpose: MX motor driver for the National Instruments PCMOTION32.DLL
 *          distributed for use with the ValueMotion series of motor
 *          controllers (formerly made by nuLogic).
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_PCMOTION32

#ifndef OS_WIN32
#error "This driver is only supported under Win32 (Windows NT/98/95)."
#endif

#include <stdlib.h>
#include <math.h>

#include <windows.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_motor.h"
#include "d_pcmotion32.h"

#include "pcMotion32.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_pcmotion32_record_function_list = {
	NULL,
	mxd_pcmotion32_create_record_structures,
	mxd_pcmotion32_finish_record_initialization,
	mxd_pcmotion32_delete_record,
	mxd_pcmotion32_print_structure,
	NULL,
	NULL,
	mxd_pcmotion32_open,
	NULL,
	NULL,
	mxd_pcmotion32_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_pcmotion32_motor_function_list = {
	mxd_pcmotion32_motor_is_busy,
	mxd_pcmotion32_move_absolute,
	mxd_pcmotion32_get_position,
	mxd_pcmotion32_set_position,
	mxd_pcmotion32_soft_abort,
	mxd_pcmotion32_immediate_abort,
	mxd_pcmotion32_positive_limit_hit,
	mxd_pcmotion32_negative_limit_hit,
	mxd_pcmotion32_find_home_position,
	mxd_pcmotion32_constant_velocity_move,
	mxd_pcmotion32_get_parameter,
	mxd_pcmotion32_set_parameter
};

/* pcStep motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_pcmotion32_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_PCMOTION32_MOTOR_STANDARD_FIELDS
};

long mxd_pcmotion32_num_record_fields
		= sizeof( mxd_pcmotion32_record_field_defaults )
			/ sizeof( mxd_pcmotion32_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pcmotion32_rfield_def_ptr
			= &mxd_pcmotion32_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_pcmotion32_get_pointers( MX_MOTOR *motor,
				MX_PCMOTION32_MOTOR **pcmotion32_motor,
				MX_PCMOTION32 **pcmotion32,
				const char *calling_fname )
{
	static const char fname[] = "mxd_pcmotion32_get_pointers()";

	MX_RECORD *pcmotion32_controller_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pcmotion32_motor == (MX_PCMOTION32_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCMOTION32_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( pcmotion32 == (MX_PCMOTION32 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PCMOTION32 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pcmotion32_motor = (MX_PCMOTION32_MOTOR *)
					motor->record->record_type_struct;

	if ( *pcmotion32_motor == (MX_PCMOTION32_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PCMOTION32_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	pcmotion32_controller_record = (*pcmotion32_motor)->controller_record;

	if ( pcmotion32_controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The pcStep controller record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*pcmotion32 = (MX_PCMOTION32 *)
			pcmotion32_controller_record->record_type_struct;

	if ( *pcmotion32 == (MX_PCMOTION32 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_PCMOTION32 pointer for controller '%s' containing motor '%s' is NULL.",
			pcmotion32_controller_record->name,
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_pcmotion32_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_pcmotion32_create_record_structures()";

	MX_MOTOR *motor;
	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	pcmotion32_motor = (MX_PCMOTION32_MOTOR *)
				malloc( sizeof(MX_PCMOTION32_MOTOR) );

	if ( pcmotion32_motor == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PCMOTION32_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = pcmotion32_motor;
	record->class_specific_function_list
			= &mxd_pcmotion32_motor_function_list;

	motor->record = record;

	/* A pcStep 4A motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pcmotion32_finish_record_initialization()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	int axis_id;
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	pcmotion32_motor = (MX_PCMOTION32_MOTOR *) record->record_type_struct;

	if ( pcmotion32_motor == (MX_PCMOTION32_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PCMOTION32_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	axis_id = pcmotion32_motor->axis_id;

	if ( (axis_id < 1) || (axis_id > 4) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"pcMotion32 motor axis number %d is outside the allowed range of 1 to 4.",
			axis_id );
	}

	if ( pcmotion32_motor->controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The controller_record pointer for motor '%s' is NULL.",
			record->name );
	}

	pcmotion32 = (MX_PCMOTION32 *)
		pcmotion32_motor->controller_record->record_type_struct;

	if ( pcmotion32 == (MX_PCMOTION32 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PCMOTION32 pointer for controller '%s' used by motor '%s' is NULL.",
			pcmotion32_motor->controller_record->name,
			record->name );
	}

	pcmotion32->motor_array[ axis_id - 1 ] = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxd_pcmotion32_delete_record()";

	MX_PCMOTION32 *pcmotion32 = NULL;
	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_RECORD *motor_record_ptr;
	int axis_id;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	pcmotion32_motor = (MX_PCMOTION32_MOTOR *) record->record_type_struct;

	if ( pcmotion32_motor != NULL ) {

	    /* Carefully delete the reference to this motor in the controller's
	     * motor_array structure.
	     */

	    axis_id = pcmotion32_motor->axis_id;

	    if ( (axis_id >= 1) && (axis_id <= 4) ) {

		if ( pcmotion32_motor->controller_record != NULL ) {

		    pcmotion32 = (MX_PCMOTION32 *)
			pcmotion32_motor->controller_record->record_type_struct;

		    if ( pcmotion32 != NULL ) {
			motor_record_ptr = pcmotion32->motor_array[axis_id - 1];

			if ( record != motor_record_ptr ) {
			    (void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"While deleting motor '%s', we found that it was not correctly recorded in "
"the motor_array for its controller '%s'.", record->name,
				pcmotion32_motor->controller_record->name );
			}

			pcmotion32->motor_array[ axis_id - 1 ] = NULL;
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
mxd_pcmotion32_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_pcmotion32_print_structure()";

	MX_MOTOR *motor;
	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type           = PCMOTION32 motor.\n\n");

	fprintf(file, "  name                 = %s\n", record->name);
	fprintf(file, "  controller record    = %s\n",
				pcmotion32->record->name);
	fprintf(file, "  axis id              = %d\n\n",
				pcmotion32_motor->axis_id);

	fprintf(file, "  default speed        = %lu steps per sec\n",
				pcmotion32_motor->default_speed);
	fprintf(file, "  default base speed   = %lu steps per sec\n",
				pcmotion32_motor->default_base_speed);
	fprintf(file, "  default acceleration = %lu steps per sec^2\n",
				pcmotion32_motor->default_acceleration);
	fprintf(file, "  default accel factor = %lu\n",
				pcmotion32_motor->default_acceleration_factor);
	if ( pcmotion32_motor->lines_per_revolution > 0 ) {
		fprintf(file,
		      "  lines per revolution = %hu\n",
				pcmotion32_motor->lines_per_revolution);
		fprintf(file,
		      "  steps per revolution = %hu\n",
				pcmotion32_motor->steps_per_revolution);
	}
	fprintf(file, "\n");

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}
	
	fprintf(file, "  position             = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
	fprintf(file, "  scale                = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset               = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash             = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit       = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit       = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  deadband             = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pcmotion32_open()";

	MX_MOTOR *motor;
	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	double speed, base_speed;
	double raw_acceleration_parameters[MX_MOTOR_NUM_ACCELERATION_PARAMS];
	DWORD steps_and_lines_factor;
	WORD stop_mode_word;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s called.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	/* Initialize the motor using the initialization procedure 
	 * recommended in the manual.
	 */

	/* Set the motor position mode to absolute. */

	status = set_pos_mode( boardID, axisID, 0 );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"Error setting the position mode for motor '%s' to absolute.  Reason = '%s'",
			record->name, mxi_pcmotion32_strerror( status ) );
	}	

	/* Set the motor stop mode to 'decelerate to stop'. */

	stop_mode_word = 0x0101;

	status = set_stop_mode( boardID, axisID, stop_mode_word );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error setting the stop mode for motor '%s'.  Reason = '%s'",
			record->name, mxi_pcmotion32_strerror( status ) );
	}	

	/* If lines_per_revolution has a non-zero value, we assume that
	 * the controller will be using closed loop encoder feedback.
	 */

	if ( pcmotion32_motor->lines_per_revolution != 0 ) {

		/* Set the controller loop mode.  This turns on closed
		 * loop feedback.
		 */

		status = set_loop_mode( boardID, axisID, 0xFFFF );

		if ( status != 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error changing the loop mode for motor '%s'.  Reason = '%s'",
			record->name, mxi_pcmotion32_strerror( status ) );
		}	

		/* Load steps and lines per revolution. */

		steps_and_lines_factor = pcmotion32_motor->steps_per_revolution;

		steps_and_lines_factor &= 0xFFFF;

		steps_and_lines_factor
			|= ( pcmotion32_motor->lines_per_revolution << 16 );

		status = load_steps_lines( boardID, axisID,
						steps_and_lines_factor );

		if ( status != 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error setting the steps and lines per revolution for motor '%s'.  "
	"Reason = '%s'",
			record->name, mxi_pcmotion32_strerror( status ) );
		}	
	}

	/* Set the motor speed. */

	speed = fabs( motor->scale ) * (double) pcmotion32_motor->default_speed;

	mx_status = mx_motor_set_speed( record, speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the motor base speed. */

	base_speed = fabs( motor->scale )
			* (double) pcmotion32_motor->default_base_speed;

	mx_status = mx_motor_set_base_speed( record, base_speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the acceleration parameters. */

	raw_acceleration_parameters[0]
		= (double) pcmotion32_motor->default_acceleration;

	raw_acceleration_parameters[1]
		= (double) pcmotion32_motor->default_acceleration_factor;

	raw_acceleration_parameters[2] = 0.0;
	raw_acceleration_parameters[3] = 0.0;

	mx_status = mx_motor_set_raw_acceleration_parameters( record,
						raw_acceleration_parameters );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_pcmotion32_resynchronize()";

	MX_MOTOR *motor;
	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	mx_status_type mx_status;

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pcmotion32_open( pcmotion32->record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_pcmotion32_open( pcmotion32->record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_pcmotion32_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_motor_is_busy()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	WORD status_word;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	status = read_axis_stat( boardID, axisID, &status_word );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error getting the axis status for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	motor->busy = FALSE;

	/* Check the run/stop status bit. */

	if ( status_word & 0x0080 ) {
		motor->busy = TRUE;
	}

	/* Check the profile complete bit and the motor on/off bit. */

	if ( (status_word & 0x0404) == 0 ) {
		motor->busy = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_move_absolute()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	DWORD destination;
	WORD mode_control_data_word;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	/* Select absolute position mode. */

	mode_control_data_word = 0;

	status = set_pos_mode( boardID, axisID, mode_control_data_word );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"Error setting the position mode for motor '%s' to absolute.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	/* Set the destination. */

	destination = (DWORD) motor->raw_destination.stepper;

	status = load_target_pos( boardID, axisID, destination );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error setting the move destination for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	/* Start the move. */

	status = start_motion( boardID, axisID );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error starting the move for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_get_position()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	DWORD current_position;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	status = read_pos( boardID, axisID, &current_position );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error reading the position for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	motor->raw_position.stepper = current_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_set_position()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->raw_set_position.stepper != 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"pcStep motor '%s' cannot have its position set to a value other than zero.",
			motor->record->name );
	}

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	status = reset_pos( boardID, axisID );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error resetting the position for motor '%s' to zero.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_soft_abort()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	status = stop_motion( boardID, axisID );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error send a stop motion command to motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_immediate_abort()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	status = kill_motion( boardID, axisID );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error send a kill motion command to motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_positive_limit_hit()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	WORD limit_switch_status, limit_bit;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	status = read_lim_stat( boardID, &limit_switch_status );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error getting the limit switch status for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	limit_bit = limit_switch_status & ( 1 << (2*(4 - axisID) + 1) );

	if ( limit_bit ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_negative_limit_hit()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	WORD limit_switch_status, limit_bit;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	status = read_lim_stat( boardID, &limit_switch_status );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error getting the limit switch status for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	limit_bit = limit_switch_status & ( 1 << (2*(4 - axisID)) );

	if ( limit_bit ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_find_home_position()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	WORD direction_bits;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->home_search >= 0 ) {
		direction_bits = 0;
	} else {
		direction_bits = 0xFFFF;
	}

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	status = find_home( boardID, axisID, direction_bits );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error starting a home search for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_constant_velocity_move()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	DWORD direction;
	WORD mode_control_data_word;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	/* Select velocity mode. */

	mode_control_data_word = 0xFF00;

	status = set_pos_mode( boardID, axisID, mode_control_data_word );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
"Error setting the position mode for motor '%s' to velocity.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	/* Set the destination. */

	if ( motor->constant_velocity_move >= 0 ) {
		direction = 1;
	} else {
		direction = -1;
	}

	status = load_target_pos( boardID, axisID, direction );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error setting the direction for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	/* Start the move. */

	status = start_motion( boardID, axisID );

	if ( status != 0 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error starting the move for motor '%s'.  Reason = '%s'",
		motor->record->name, mxi_pcmotion32_strerror( status ) );
	}	

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_get_parameter()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	WORD velocity;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		status = read_vel( boardID, axisID, &velocity );

		if ( status != 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error getting the velocity for motor '%s'.  Reason = '%s'",
				motor->record->name,
				mxi_pcmotion32_strerror( status ) );
		}	

		motor->raw_speed = (double) velocity;

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
		 * motor->raw_acceleration_parameters, since presumably that was
		 * set the last time we set the acceleration parameters.
		 */

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pcmotion32_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_pcmotion32_set_parameter()";

	MX_PCMOTION32_MOTOR *pcmotion32_motor = NULL;
	MX_PCMOTION32 *pcmotion32 = NULL;
	DWORD velocity, acceleration;
	WORD base_velocity, acceleration_factor;
	BYTE boardID, axisID;
	int status;
	mx_status_type mx_status;

	mx_status = mxd_pcmotion32_get_pointers( motor,
					&pcmotion32_motor, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;
	axisID = pcmotion32_motor->axis_id;

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		velocity = mx_round( motor->raw_speed );

		status = load_vel( boardID, axisID, velocity );

		if ( status != 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error setting the speed for motor '%s'.  Reason = '%s'",
				motor->record->name,
				mxi_pcmotion32_strerror( status ) );
		}

	} else if ( motor->parameter_type == MXLV_MTR_BASE_SPEED ) {

		base_velocity = (WORD) mx_round( motor->raw_base_speed );

		status = set_base_vel( boardID, axisID, base_velocity );

		if ( status != 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error setting the base speed for motor '%s'.  Reason = '%s'",
				motor->record->name,
				mxi_pcmotion32_strerror( status ) );
		}

	} else if ( motor->parameter_type == MXLV_MTR_MAXIMUM_SPEED ) {

		motor->raw_maximum_speed = 750000;  /* steps per second */

		return MX_SUCCESSFUL_RESULT;

	} else if ( motor->parameter_type ==
			MXLV_MTR_RAW_ACCELERATION_PARAMETERS )
	{

		/* First, set the acceleration. */

		acceleration = mx_round(motor->raw_acceleration_parameters[0]);

		status = load_accel( boardID, axisID, acceleration );

		if ( status != 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Error setting the acceleration for motor '%s'.  Reason = '%s'",
				motor->record->name,
				mxi_pcmotion32_strerror( status ) );
		}

		/* Then, set the acceleration factor. */

		acceleration_factor = (WORD)
			mx_round( motor->raw_acceleration_parameters[1] );

		status = load_accel_fact( boardID, axisID,
						acceleration_factor );

		if ( status != 0 ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"Error setting the acceleration factor for motor '%s'.  Reason = '%s'",
				motor->record->name,
				mxi_pcmotion32_strerror( status ) );
		}

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
	}

	return mx_status;
}

#endif /* HAVE_PCMOTION32 */

