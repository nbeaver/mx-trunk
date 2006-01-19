/*
 * Name:    d_vme58.c
 *
 * Purpose: MX driver for the Oregon Micro Systems VME58 motor controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_vme.h"
#include "i_vme58.h"
#include "d_vme58.h"

#define VME58_MOTOR_DEBUG	TRUE

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_vme58_record_function_list = {
	NULL,
	mxd_vme58_create_record_structures,
	mxd_vme58_finish_record_initialization,
	NULL,
	mxd_vme58_print_structure,
	NULL,
	NULL,
	mxd_vme58_open,
	NULL,
	NULL,
	mxd_vme58_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_vme58_motor_function_list = {
	mxd_vme58_motor_is_busy,
	mxd_vme58_move_absolute,
	mxd_vme58_get_position,
	mxd_vme58_set_position,
	mxd_vme58_soft_abort,
	mxd_vme58_immediate_abort,
	mxd_vme58_positive_limit_hit,
	mxd_vme58_negative_limit_hit,
	mxd_vme58_find_home_position,
	mxd_vme58_constant_velocity_move,
	mxd_vme58_get_parameter,
	mxd_vme58_set_parameter
};

/* VME58 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_vme58_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_VME58_MOTOR_STANDARD_FIELDS
};

long mxd_vme58_num_record_fields
		= sizeof( mxd_vme58_record_field_defaults )
			/ sizeof( mxd_vme58_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_vme58_rfield_def_ptr
			= &mxd_vme58_record_field_defaults[0];

static char mxd_vme58_axis_name[] = MX_VME58_AXIS_NAMES;

#define VME58_AXIS_NUMBER(x) 	(mxd_vme58_axis_name[ (x)->axis_number ])

/* A private function for the use of the driver. */

static mx_status_type
mxd_vme58_get_pointers( MX_MOTOR *motor,
			MX_VME58_MOTOR **vme58_motor,
			MX_VME58 **vme58,
			const char *calling_fname )
{
	const char fname[] = "mxd_vme58_get_pointers()";

	MX_RECORD *vme58_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vme58_motor == (MX_VME58_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME58_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vme58 == (MX_VME58 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VME58 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vme58_motor = (MX_VME58_MOTOR *) (motor->record->record_type_struct);

	if ( *vme58_motor == (MX_VME58_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VME58_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	vme58_record = (*vme58_motor)->vme58_record;

	if ( vme58_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The vme58_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	*vme58 = (MX_VME58 *) vme58_record->record_type_struct;

	if ( *vme58 == (MX_VME58 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VME58 pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_vme58_create_record_structures()";

	MX_MOTOR *motor;
	MX_VME58_MOTOR *vme58_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	vme58_motor = (MX_VME58_MOTOR *) malloc( sizeof(MX_VME58_MOTOR) );

	if ( vme58_motor == (MX_VME58_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VME58_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = vme58_motor;
	record->class_specific_function_list = &mxd_vme58_motor_function_list;

	motor->record = record;

	/* A VME58 motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	/* The VME58 reports accelerations in counts/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_vme58_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_VME58 *vme58;
	MX_RECORD *vme58_record;
	MX_VME58_MOTOR *vme58_motor;
	int i;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if the record pointed to by vme58_motor->vme58_record
	 * is actually a "vme58" interface record.
	 */

	vme58_record = vme58_motor->vme58_record;

	if ( ( vme58_record->mx_superclass != MXR_INTERFACE )
	  || ( vme58_record->mx_class != MXI_GENERIC )
	  || ( vme58_record->mx_type != MXI_GEN_VME58 ) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The field 'vme58_record' in record '%s' does not point to "
	"a 'vme58' interface record.  Instead, it points to record '%s'",
			record->name, vme58_record->name );
	}

	/* Store a pointer to this record in the MX_VME58 structure. */

	if ( vme58->motor_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The motor_array pointer for the VME58 interface '%s' is NULL.",
			vme58->record->name );
	}

	i = vme58_motor->axis_number - 1;

	if ( vme58->motor_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The motor_array pointer for VME58 interface '%s' is NULL.",
			vme58->record->name );
	}

	(vme58->motor_array)[i] = record;

	/* Set the initial speeds and accelerations. */

	if ( vme58_motor->flags & MXF_VME58_MOTOR_IGNORE_DATABASE_SETTINGS )
	{
		/* If we have been told to ignore the database settings, set
		 * the values to -1.  This way the function mxd_vme58_open()
		 * will know not to send any actual commands to the controller.
		 */

		motor->raw_speed = -1.0;
		motor->raw_base_speed = -1.0;
		motor->raw_acceleration_parameters[0] = -1.0;
	} else {
		/* Otherwise, use the values from the database file. */

		motor->raw_speed = (double) vme58_motor->default_speed;
		motor->raw_base_speed = (double)
					vme58_motor->default_base_speed;
		motor->raw_acceleration_parameters[0] = (double)
					vme58_motor->default_acceleration;
	}

	motor->raw_maximum_speed = MX_VME58_MOTOR_MAXIMUM_SPEED;

	motor->speed = fabs(motor->scale) * motor->raw_speed;
	motor->base_speed = fabs(motor->scale) * motor->raw_base_speed;
	motor->maximum_speed = fabs(motor->scale) * motor->raw_maximum_speed;

	motor->raw_acceleration_parameters[1] = 0.0;
	motor->raw_acceleration_parameters[2] = 0.0;
	motor->raw_acceleration_parameters[3] = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_vme58_print_structure()";

	MX_MOTOR *motor;
	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = VME58_MOTOR motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  controller        = %s\n", vme58->record->name);
	fprintf(file, "  axis number       = %d\n", vme58_motor->axis_number);
	fprintf(file, "  flags             = %#lx\n", vme58_motor->flags);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%ld counts)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
	fprintf(file, "  scale             = %g %s per count.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%ld counts)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit    = %g %s  (%ld counts)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit    = %g %s  (%ld counts)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband     = %g %s  (%ld counts)\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	fprintf(file, "  default speed     = %ld counts/second\n",
		vme58_motor->default_speed );

	fprintf(file, "  default base speed= %ld counts/second\n",
		vme58_motor->default_base_speed );

	fprintf(file, "  default acceleration = %ld counts/second**2\n\n",
		vme58_motor->default_base_speed );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_open( MX_RECORD *record )
{
	const char fname[] = "mxd_vme58_open()";

	MX_MOTOR *motor;
	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, disable the hardware limit switches.  Otherwise,
	 * enable them.
	 */

	if ( vme58_motor->flags & MXF_VME58_MOTOR_DISABLE_HARDWARE_LIMITS ) {

		sprintf( command, "A%c LF", VME58_AXIS_NUMBER( vme58_motor ) );
	} else {
		sprintf( command, "A%c LN", VME58_AXIS_NUMBER( vme58_motor ) );
	}

	mx_status = mxi_vme58_command( vme58, command,
					NULL, 0, VME58_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, set the home switch to active high.  This allows
	 * the use of normally closed home switches.
	 */

	if ( vme58_motor->flags & MXF_VME58_MOTOR_HOME_HIGH ) {

		sprintf( command, "A%c HH", VME58_AXIS_NUMBER( vme58_motor ) );
	} else {
		sprintf( command, "A%c HL", VME58_AXIS_NUMBER( vme58_motor ) );
	}

	mx_status = mxi_vme58_command( vme58, command,
					NULL, 0, VME58_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the speeds and acceleration.
	 * 
	 * If a raw speed or acceleration has a value less than zero,
	 * then we ignore the value.
	 */

	if ( motor->raw_speed >= 0.0 ) {

		mx_status = mx_motor_set_speed( record, motor->speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	if ( motor->raw_base_speed >= 0.0 ) {

		mx_status = mx_motor_set_base_speed( record,
						motor->base_speed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	if ( motor->raw_acceleration_parameters[0] >= 0.0 ) {

		mx_status = mx_motor_set_raw_acceleration_parameters( record,
					motor->raw_acceleration_parameters );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_vme58_resynchronize()";

	MX_VME58_MOTOR *vme58_motor;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	vme58_motor = (MX_VME58_MOTOR *) record->record_type_struct;

	if ( vme58_motor == (MX_VME58_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_VME58_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	/* Resynchronize the VME58 interface. */

	mx_status = mxi_vme58_resynchronize( vme58_motor->vme58_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the motor a soft abort.  This will stop it from moving
	 * and clear its command queue.
	 */

	mx_status = mx_motor_soft_abort( record );

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_vme58_motor_is_busy( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_motor_is_busy()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	char response[80];
	int length;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "A%c QA", VME58_AXIS_NUMBER( vme58_motor ) );

	mx_status = mxi_vme58_command( vme58, command,
			response, sizeof response, VME58_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Zero length response to motor status command.");
	}

	/* When constructing bit indices for the output of the QA
	 * command, remember that the output of QA starts with
	 * an <LF>, <CR>, <CR> sequence that you must skip over.
	 */

	if ( response[4] == 'N' ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_move_absolute()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	long motor_steps;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_destination.stepper;

	sprintf( command, "A%c MA%ld GD ID",
			VME58_AXIS_NUMBER( vme58_motor ), motor_steps );

	mx_status = mxi_vme58_command( vme58, command,
					NULL, 0, VME58_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vme58_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_get_position()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	char response[80];
	long motor_steps;
	int num_tokens;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "A%c RP", VME58_AXIS_NUMBER( vme58_motor ) );

	mx_status = mxi_vme58_command( vme58, command,
			response, sizeof response, VME58_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_tokens = sscanf( response, "%ld", &motor_steps );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to '%s' command.  "
		"Response seen = '%s'", command, response );
	}

	motor->raw_position.stepper = motor_steps;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_set_position()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	long motor_steps;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_steps = motor->raw_set_position.stepper;

	sprintf( command, "A%c LP%ld",
			VME58_AXIS_NUMBER(vme58_motor), motor_steps );

	mx_status = mxi_vme58_command( vme58, command,
			NULL, 0, VME58_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vme58_soft_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_soft_abort()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "A%c ST", VME58_AXIS_NUMBER( vme58_motor ) );

	mx_status = mxi_vme58_command( vme58, command,
					NULL, 0, VME58_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vme58_immediate_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_immediate_abort()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send a kill motion command that halts all axes attached to
	 * this controller.  This also clears the command queues for
	 * all axes.
	 */

	mx_status = mxi_vme58_command( vme58, "AX KL",
					NULL, 0, VME58_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vme58_positive_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_positive_limit_hit()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	char response[80];
	int length;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do not send the command if the hardware limits are disabled. */

	if ( vme58_motor->flags & MXF_VME58_MOTOR_DISABLE_HARDWARE_LIMITS ) {

		motor->positive_limit_hit = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Construct the status query command. */

	sprintf( command, "A%c QA", VME58_AXIS_NUMBER( vme58_motor ) );

	mx_status = mxi_vme58_command( vme58, command,
			response, sizeof response, VME58_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Zero length response to motor status command.");
	}

	/* When constructing bit indices for the output of the QA
	 * command, remember that the output of QA starts with
	 * an <LF>, <CR>, <CR> sequence that you must skip over.
	 */

	if ( ( response[5] == 'L' ) && ( response[3] == 'P' ) ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_negative_limit_hit( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_negative_limit_hit()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	char response[80];
	int length;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do not send the command if the hardware limits are disabled. */

	if ( vme58_motor->flags & MXF_VME58_MOTOR_DISABLE_HARDWARE_LIMITS ) {

		motor->negative_limit_hit = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Construct the status query command. */

	sprintf( command, "A%c QA", VME58_AXIS_NUMBER( vme58_motor ) );

	mx_status = mxi_vme58_command( vme58, command,
			response, sizeof response, VME58_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( response );

	if ( length <= 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Zero length response to motor status command.");
	}

	/* When constructing bit indices for the output of the QA
	 * command, remember that the output of QA starts with
	 * an <LF>, <CR>, <CR> sequence that you must skip over.
	 */

	if ( ( response[5] == 'L' ) && ( response[3] == 'M' ) ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_find_home_position()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This command performs a home search and then sets the position
	 * counter to zero.  The correct command depends on the direction
	 * of the home search.
	 */

	if ( motor->home_search >= 0 ) {
		sprintf( command, "A%c HM0 MA0 GO",
					VME58_AXIS_NUMBER( vme58_motor ) );
	} else {
		sprintf( command, "A%c HR0 MA0 GO",
					VME58_AXIS_NUMBER( vme58_motor ) );
	}

	/* Start the home search. */

	mx_status = mxi_vme58_command( vme58, command,
					NULL, 0, VME58_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vme58_constant_velocity_move( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_constant_velocity_move()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	double speed;
	long raw_speed;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the raw speed for this motor. */

	mx_status = mx_motor_get_speed( motor->record, &speed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_speed = mx_round( motor->raw_speed );

	/* Construct the jog command. */

	if ( motor->constant_velocity_move >= 0 ) {
		sprintf( command, "A%c JG%ld",
				VME58_AXIS_NUMBER( vme58_motor ),
				raw_speed );
	} else {
		sprintf( command, "A%c JG%ld",
				VME58_AXIS_NUMBER( vme58_motor ),
				-raw_speed );
	}
		
	/* Command the move to start. */

	mx_status = mxi_vme58_command( vme58, command,
					NULL, 0, VME58_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vme58_get_parameter( MX_MOTOR *motor )
{
	/* I have not yet found a way to read the settings of these
	 * parameters, so the body of this function is bypassed.
	 *
	 * Bypassing the body of this function means that the calling
	 * routine merely get the values that are already in the motor's
	 * internal MX datastructure.  As long as MX is the package that
	 * originally set the values of these parameters in the motor
	 * controller, then the returned values will be correct.
	 */
#if 0
	const char fname[] = "mxd_vme58_get_parameter()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	char response[80];
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor, &vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		sprintf( command, "A%c ??", VME58_AXIS_NUMBER( vme58_motor ) );

		mx_status = mxi_vme58_command( vme58, command,
				response, sizeof(response), VME58_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &(motor->raw_speed) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned speed for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

	} else if ( motor->parameter_type == MXLV_MTR_BASE_SPEED ) {

		sprintf( command, "A%c ??", VME58_AXIS_NUMBER( vme58_motor ) );

		mx_status = mxi_vme58_command( vme58, command,
				response, sizeof(response), VME58_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg", &(motor->raw_base_speed) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned base speed for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

	} else if ( motor->parameter_type == MXLV_MTR_MAXIMUM_SPEED ) {

		motor->raw_maximum_speed = MX_VME58_MOTOR_MAXIMUM_SPEED;

	} else if ( motor->parameter_type ==
			MXLV_MTR_RAW_ACCELERATION_PARAMETERS )
	{

		sprintf( command, "A%c ??", VME58_AXIS_NUMBER( vme58_motor ) );

		mx_status = mxi_vme58_command( vme58, command,
				response, sizeof(response), VME58_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%lg",
				&(motor->raw_acceleration_parameters[0]) );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Unable to parse returned speed for motor '%s'.  Response = '%s'",
				motor->record->name, response );
		}

		motor->raw_acceleration_parameters[1] = 0.0;
		motor->raw_acceleration_parameters[2] = 0.0;
		motor->raw_acceleration_parameters[3] = 0.0;

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
	}
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vme58_set_parameter( MX_MOTOR *motor )
{
	const char fname[] = "mxd_vme58_set_parameter()";

	MX_VME58_MOTOR *vme58_motor;
	MX_VME58 *vme58;
	char command[80];
	long raw_speed, raw_base_speed, acceleration;
	mx_status_type mx_status;

	mx_status = mxd_vme58_get_pointers( motor,
					&vme58_motor, &vme58, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Format the appropriate parameter setting command. */

	if ( motor->parameter_type == MXLV_MTR_SPEED ) {

		raw_speed = mx_round( motor->raw_speed );

		if ( (raw_speed < 0)
		  || (raw_speed > MX_VME58_MOTOR_MAXIMUM_SPEED) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested raw speed of %ld steps/sec is outside "
	"the allowed range of 0 <= raw speed <= %ld.",
				raw_speed, MX_VME58_MOTOR_MAXIMUM_SPEED );
		}

		sprintf( command, "A%c VL%ld",
			VME58_AXIS_NUMBER( vme58_motor ), raw_speed );

	} else if ( motor->parameter_type == MXLV_MTR_BASE_SPEED ) {

		raw_base_speed = mx_round( motor->raw_base_speed );

		if ( (raw_base_speed < 0)
		  || (raw_base_speed > MX_VME58_MOTOR_MAXIMUM_SPEED) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Requested raw base speed of %ld steps/sec is outside "
	"the allowed range of 0 <= raw base speed <= %ld.",
				raw_base_speed, MX_VME58_MOTOR_MAXIMUM_SPEED );
		}

		sprintf( command, "A%c VB%ld",
			VME58_AXIS_NUMBER( vme58_motor ), raw_base_speed );

	} else if ( motor->parameter_type == MXLV_MTR_MAXIMUM_SPEED ) {

		motor->raw_maximum_speed = MX_VME58_MOTOR_MAXIMUM_SPEED;

		return MX_SUCCESSFUL_RESULT;

	} else if ( motor->parameter_type ==
			MXLV_MTR_RAW_ACCELERATION_PARAMETERS )
	{

		acceleration = mx_round(motor->raw_acceleration_parameters[0]);

		if ( (acceleration <= 0)
		  || (acceleration >= MX_VME58_MOTOR_MAXIMUM_ACCELERATION) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested acceleration of %ld steps/sec/sec is outside "
		"the allowed range of 0 < acceleration < %ld.",
			acceleration, MX_VME58_MOTOR_MAXIMUM_ACCELERATION );
		}

		sprintf( command, "A%c AC%ld",
			VME58_AXIS_NUMBER( vme58_motor ), acceleration );

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
	}

	/* Send the command to the VME58 controller. */

	mx_status = mxi_vme58_command( vme58, command,
					NULL, 0, VME58_MOTOR_DEBUG );

	return mx_status;
}

