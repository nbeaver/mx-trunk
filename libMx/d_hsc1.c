/*
 * Name:    d_hsc1.c
 *
 * Purpose: MX driver for the X-ray Instrumentation Associates
 *          HSC-1 Huber slit controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2006 Illinois Institute of Technology
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
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "mx_generic.h"
#include "d_hsc1.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_hsc1_record_function_list = {
	mxd_hsc1_initialize_type,
	mxd_hsc1_create_record_structures,
	mxd_hsc1_finish_record_initialization,
	mxd_hsc1_delete_record,
	mxd_hsc1_print_structure,
	mxd_hsc1_read_parms_from_hardware,
	mxd_hsc1_write_parms_to_hardware,
	mxd_hsc1_open,
	mxd_hsc1_close,
	NULL,
	mxd_hsc1_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_hsc1_motor_function_list = {
	mxd_hsc1_motor_is_busy,
	mxd_hsc1_move_absolute,
	mxd_hsc1_get_position,
	mxd_hsc1_set_position,
	mxd_hsc1_soft_abort,
	mxd_hsc1_immediate_abort,
	mxd_hsc1_positive_limit_hit,
	mxd_hsc1_negative_limit_hit,
	mxd_hsc1_find_home_position,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler
};

/* HSC-1 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_hsc1_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_HSC1_STANDARD_FIELDS
};

mx_length_type mxd_hsc1_num_record_fields
		= sizeof( mxd_hsc1_record_field_defaults )
			/ sizeof( mxd_hsc1_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_hsc1_rfield_def_ptr
			= &mxd_hsc1_record_field_defaults[0];

#define HSC1_MOTOR_DEBUG	FALSE

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxd_hsc1_get_pointers( MX_MOTOR *motor,
			MX_HSC1_MOTOR **hsc1_motor,
			MX_HSC1_INTERFACE **hsc1_interface,
			const char *calling_fname )
{
	const char fname[] = "mxd_hsc1_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( hsc1_motor == (MX_HSC1_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HSC1_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*hsc1_motor = (MX_HSC1_MOTOR *) (motor->record->record_type_struct);

	if ( *hsc1_motor == (MX_HSC1_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HSC1_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( hsc1_interface == (MX_HSC1_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HSC1_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( (*hsc1_motor)->hsc1_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The hsc1_interface_record pointer for HSC-1 motor '%s' is NULL.",
			motor->record->name );
	}

	*hsc1_interface = (MX_HSC1_INTERFACE *)
		((*hsc1_motor)->hsc1_interface_record->record_type_struct);

	if ( *hsc1_interface == (MX_HSC1_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HSC1_INTERFACE pointer for HSC-1 motor '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxd_hsc1_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hsc1_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_hsc1_create_record_structures()";

	MX_MOTOR *motor;
	MX_HSC1_MOTOR *hsc1_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	hsc1_motor = (MX_HSC1_MOTOR *) malloc( sizeof(MX_HSC1_MOTOR) );

	if ( hsc1_motor == (MX_HSC1_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HSC1_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = hsc1_motor;
	record->class_specific_function_list
				= &mxd_hsc1_motor_function_list;

	motor->record = record;

	/* An elapsed time motor is treated as an stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hsc1_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type status;

	status = mx_motor_finish_record_initialization( record );

	return status;
}

MX_EXPORT mx_status_type
mxd_hsc1_delete_record( MX_RECORD *record )
{
	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
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
mxd_hsc1_print_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_hsc1_print_structure()";

	MX_MOTOR *motor;
	MX_RECORD *hsc1_interface_record;
	MX_HSC1_MOTOR *hsc1_motor;
	double position, move_deadband;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	hsc1_motor = (MX_HSC1_MOTOR *) (record->record_type_struct);

	if ( hsc1_motor == (MX_HSC1_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_HSC1_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	hsc1_interface_record = (MX_RECORD *)
			(hsc1_motor->hsc1_interface_record);

	if ( hsc1_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Interface record pointer for Compumotor motor '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = HSC-1 motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  interface name    = %s\n",
						hsc1_interface_record->name);
	fprintf(file, "  module number     = %lu\n",
				(unsigned long) hsc1_motor->module_number);

	fprintf(file, "  motor name        = %c\n", hsc1_motor->motor_name);

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS ) {
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
mxd_hsc1_read_parms_from_hardware( MX_RECORD *record )
{
	double position;

	return mx_motor_get_position( record, &position );
}

MX_EXPORT mx_status_type
mxd_hsc1_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hsc1_open( MX_RECORD *record )
{
	return mxd_hsc1_resynchronize( record );
}

MX_EXPORT mx_status_type
mxd_hsc1_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hsc1_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_hsc1_resynchronize()";

	MX_MOTOR *motor;
	MX_HSC1_INTERFACE *hsc1_interface;
	MX_HSC1_MOTOR *hsc1_motor;
	char response[80];
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	status = mxd_hsc1_get_pointers( motor, &hsc1_motor,
					&hsc1_interface, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxi_hsc1_resynchronize(
			hsc1_motor->hsc1_interface_record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Verify that the controller is there by asking for the
	 * version number of it's EEPROM.
	 */

	status = mxi_hsc1_command( hsc1_interface, hsc1_motor->module_number,
			"R 14", response, sizeof(response), HSC1_MOTOR_DEBUG );

	return status;
}

/* ============ Motor specific functions ============ */

/* The HSC-1 does not provide a separate function for determining whether
 * or not a motor is busy, so we use a side effect of the get_position
 * function to find this out.
 */ 

MX_EXPORT mx_status_type
mxd_hsc1_motor_is_busy( MX_MOTOR *motor )
{
	return mxd_hsc1_get_position( motor );
}

MX_EXPORT mx_status_type
mxd_hsc1_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_hsc1_move_absolute()";

	MX_HSC1_INTERFACE *hsc1_interface;
	MX_HSC1_MOTOR *hsc1_motor;
	char command[40], response[80];
	long a_steps, b_steps, old_position, relative_distance;
	int num_items;
	mx_status_type status;

	status = mxd_hsc1_get_pointers( motor, &hsc1_motor,
					&hsc1_interface, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Get the current positions of the A and B motors. */

	status = mxi_hsc1_command( hsc1_interface, hsc1_motor->module_number,
			"P", response, sizeof response, HSC1_MOTOR_DEBUG );

	if ( status.code == MXE_NOT_READY ) {
		return mx_error( MXE_NOT_READY, fname,
		"One or more of the HSC-1 motors is already moving." );
	}
	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%ld %ld", &a_steps, &b_steps );

	if ( num_items != 2 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find motor positions in response '%s'",
			response );
	}

	/* Construct the move command required for this motor. */

	switch( hsc1_motor->motor_name ) {
	case 'A':
		sprintf( command, "M %ld %ld",
			(long) motor->raw_destination.stepper, b_steps );

		break;
	case 'B':
		sprintf( command, "M %ld %ld",
			a_steps, (long) motor->raw_destination.stepper );

		break;
	case 'C':
		old_position = mx_round( 0.5 * (double) (a_steps - b_steps ) );

		relative_distance
			= motor->raw_destination.stepper - old_position;

		sprintf( command, "S %+ld", relative_distance );
		break;
	case 'S':
	case 'W':
		old_position = a_steps + b_steps;

		relative_distance
			= motor->raw_destination.stepper - old_position;

		if ( relative_distance == 0 ) {

			return MX_SUCCESSFUL_RESULT;    /* No move required. */

		} else if ( relative_distance > 0L ) {

			sprintf( command, "O %ld", relative_distance );

		} else {
			sprintf( command, "C %ld", -relative_distance );
		}
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unrecognized HSC-1 motor name '%c'",
			hsc1_motor->motor_name );
	}

	status = mxi_hsc1_command( hsc1_interface, hsc1_motor->module_number,
			command, response, sizeof response, HSC1_MOTOR_DEBUG );

	if ( status.code == MXE_SUCCESS ) {

		/* Mark this module as busy. */

		hsc1_interface->module_is_busy[ hsc1_motor->module_number ]
			= TRUE;
	}

	return status;
}

MX_EXPORT mx_status_type
mxd_hsc1_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_hsc1_get_position()";

	MX_HSC1_INTERFACE *hsc1_interface;
	MX_HSC1_MOTOR *hsc1_motor;
	char response[50];
	int num_items;
	long a_steps, b_steps;
	mx_status_type status;

	status = mxd_hsc1_get_pointers( motor, &hsc1_motor,
					&hsc1_interface, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxi_hsc1_command( hsc1_interface, hsc1_motor->module_number,
			"P", response, sizeof response, HSC1_MOTOR_DEBUG );

	motor->busy = FALSE;

	if ( status.code == MXE_NOT_READY ) {

		motor->busy = TRUE;         /* The motor is moving */

		return MX_SUCCESSFUL_RESULT;
	}
	if ( status.code != MXE_SUCCESS )
		return status;

	/* Occasionally, a P command seems to return OK without any position
	 * value after it.  Treat this as a motor busy case.
	 */

	if ( strlen( response ) == 0 ) {

		motor->busy = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Parse the returned position value. */

	num_items = sscanf( response, "%ld %ld", &a_steps, &b_steps );

	if ( num_items != 2 ) {
		(void) mxd_hsc1_resynchronize( motor->record );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find motor positions in response '%s'",
			response );
	}

	switch( hsc1_motor->motor_name ) {
	case 'A':
		motor->raw_position.stepper = a_steps;
		break;
	case 'B':
		motor->raw_position.stepper = b_steps;
		break;
	case 'C':		/* Slit center */

		motor->raw_position.stepper
			= mx_round( 0.5 * (double) ( a_steps - b_steps ) );

		break;
	case 'S':		/* Slit size or width */
	case 'W':
		motor->raw_position.stepper = a_steps + b_steps;
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unrecognized HSC-1 motor name '%c'",
			hsc1_motor->motor_name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* For this driver, 'set_position' performs an HSC-1
 * Immediate Calibration command. 
 */

MX_EXPORT mx_status_type
mxd_hsc1_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_hsc1_set_position()";

	MX_HSC1_INTERFACE *hsc1_interface;
	MX_HSC1_MOTOR *hsc1_motor;
	mx_status_type status;

	status = mxd_hsc1_get_pointers( motor, &hsc1_motor,
					&hsc1_interface, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Only do the calibration if the requested user position is zero. */

	if ( fabs( motor->set_position ) > 1.0e-12 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"Setting the motor position to non-zero values is not supported." );
	}

	/* Send the Immediate Calibration command. */

	status = mxi_hsc1_command( hsc1_interface, hsc1_motor->module_number,
				"0 I", NULL, 0, HSC1_MOTOR_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_hsc1_soft_abort( MX_MOTOR *motor )
{
	const char fname[] = "mxd_hsc1_soft_abort()";

	MX_HSC1_INTERFACE *hsc1_interface;
	MX_HSC1_MOTOR *hsc1_motor;
	uint32_t module_number;
	mx_hex_type flags;
	mx_status_type status;

	status = mxd_hsc1_get_pointers( motor, &hsc1_motor,
					&hsc1_interface, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Send the stop command. */


	flags  = MXF_HSC1_IGNORE_BUSY_STATUS;

	flags |= MXF_HSC1_IGNORE_RESPONSE;

	flags |= HSC1_MOTOR_DEBUG;

	module_number = hsc1_motor->module_number;

	status = mxi_hsc1_command( hsc1_interface, module_number,
				"K", NULL, 0, flags );

	if ( status.code != MXE_SUCCESS )
		return status;

	mx_msleep(250);

	(void) mx_rs232_discard_unread_input( hsc1_interface->rs232_record,
						HSC1_MOTOR_DEBUG );

	hsc1_interface->module_is_busy[ module_number ] = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hsc1_immediate_abort( MX_MOTOR *motor )
{
	return mxd_hsc1_soft_abort( motor );
}

MX_EXPORT mx_status_type
mxd_hsc1_positive_limit_hit( MX_MOTOR *motor )
{
	motor->positive_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hsc1_negative_limit_hit( MX_MOTOR *motor )
{
	motor->negative_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

/* For this driver, 'find_home_position' performs a different HSC-1 command
 * depending on the value of motor->home_search.
 *
 * motor->home_search > 0    ---> Manual Calibration Command.
 * motor->home_search == 0   ---> Immediate Calibration Command.
 * motor->home_search < 0    ---> Uncalibrate Command.
 */

MX_EXPORT mx_status_type
mxd_hsc1_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_hsc1_find_home_position()";

	MX_HSC1_INTERFACE *hsc1_interface;
	MX_HSC1_MOTOR *hsc1_motor;
	char command[20];
	mx_status_type status;

	status = mxd_hsc1_get_pointers( motor, &hsc1_motor,
					&hsc1_interface, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->home_search > 0 ) {
		strcpy( command, "0 M" );
	} else
	if ( motor->home_search == 0 ) {
		strcpy( command, "0 I" );
	} else {
		strcpy( command, "0 -" );
	}

	/* Send the command. */

	status = mxi_hsc1_command( hsc1_interface, hsc1_motor->module_number,
				command, NULL, 0, HSC1_MOTOR_DEBUG );

	return status;
}

