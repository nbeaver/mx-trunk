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
 * Copyright 1999-2003, 2010, 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define HSC1_MOTOR_DEBUG	FALSE

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
	NULL,
	mxd_hsc1_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	mxd_hsc1_print_structure,
	mxd_hsc1_open,
	NULL,
	NULL,
	mxd_hsc1_open
};

MX_MOTOR_FUNCTION_LIST mxd_hsc1_motor_function_list = {
	mxd_hsc1_motor_is_busy,
	mxd_hsc1_move_absolute,
	mxd_hsc1_get_position,
	mxd_hsc1_set_position,
	mxd_hsc1_soft_abort,
	NULL,
	NULL,
	NULL,
	mxd_hsc1_raw_home_command,
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

long mxd_hsc1_num_record_fields
		= sizeof( mxd_hsc1_record_field_defaults )
			/ sizeof( mxd_hsc1_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_hsc1_rfield_def_ptr
			= &mxd_hsc1_record_field_defaults[0];

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxd_hsc1_get_pointers( MX_MOTOR *motor,
			MX_HSC1_MOTOR **hsc1_motor,
			MX_HSC1_INTERFACE **hsc1_interface,
			const char *calling_fname )
{
	static const char fname[] = "mxd_hsc1_get_pointers()";

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
mxd_hsc1_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_hsc1_create_record_structures()";

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
mxd_hsc1_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_hsc1_print_structure()";

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
					hsc1_motor->module_number);
	fprintf(file, "  motor name        = %c\n", hsc1_motor->motor_name);

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%ld steps)\n",
			motor->position, motor->units,
			motor->raw_position.stepper );
	fprintf(file, "  scale             = %g %s per step.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%ld steps)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.stepper );
	
	fprintf(file, "  negative limit    = %g %s  (%ld steps)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.stepper );

	fprintf(file, "  positive limit    = %g %s  (%ld steps)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.stepper );

	move_deadband = motor->scale * (double)motor->raw_move_deadband.stepper;

	fprintf(file, "  move deadband     = %g %s  (%ld steps)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.stepper );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_hsc1_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_hsc1_open()";

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
	static const char fname[] = "mxd_hsc1_move_absolute()";

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
		snprintf( command, sizeof(command),
			"M %ld %ld",
			motor->raw_destination.stepper, b_steps );

		break;
	case 'B':
		snprintf( command, sizeof(command),
			"M %ld %ld",
			a_steps, motor->raw_destination.stepper );

		break;
	case 'C':
		old_position = mx_round( 0.5 * (double) (a_steps - b_steps ) );

		relative_distance
			= motor->raw_destination.stepper - old_position;

		snprintf( command, sizeof(command),
				"S %+ld", relative_distance );
		break;
	case 'S':
	case 'W':
		old_position = a_steps + b_steps;

		relative_distance
			= motor->raw_destination.stepper - old_position;

		if ( relative_distance == 0 ) {

			return MX_SUCCESSFUL_RESULT;    /* No move required. */

		} else if ( relative_distance > 0L ) {

			snprintf( command, sizeof(command),
					"O %ld", relative_distance );

		} else {
			snprintf( command, sizeof(command),
					"C %ld", -relative_distance );
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
	static const char fname[] = "mxd_hsc1_get_position()";

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
		(void) mxd_hsc1_open( motor->record );

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
	static const char fname[] = "mxd_hsc1_set_position()";

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
	static const char fname[] = "mxd_hsc1_soft_abort()";

	MX_HSC1_INTERFACE *hsc1_interface;
	MX_HSC1_MOTOR *hsc1_motor;
	unsigned long module_number;
	int flags;
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

/* For this driver, 'raw_home_command' performs a different HSC-1 command
 * depending on the value of motor->home_search.
 *
 * motor->home_search > 0    ---> Manual Calibration Command.
 * motor->home_search == 0   ---> Immediate Calibration Command.
 * motor->home_search < 0    ---> Uncalibrate Command.
 */

MX_EXPORT mx_status_type
mxd_hsc1_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_hsc1_raw_home_command()";

	MX_HSC1_INTERFACE *hsc1_interface;
	MX_HSC1_MOTOR *hsc1_motor;
	char command[20];
	mx_status_type status;

	status = mxd_hsc1_get_pointers( motor, &hsc1_motor,
					&hsc1_interface, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( motor->raw_home_command > 0 ) {
		strlcpy( command, "0 M", sizeof(command) );
	} else
	if ( motor->raw_home_command == 0 ) {
		strlcpy( command, "0 I", sizeof(command) );
	} else {
		strlcpy( command, "0 -", sizeof(command) );
	}

	/* Send the command. */

	status = mxi_hsc1_command( hsc1_interface, hsc1_motor->module_number,
				command, NULL, 0, HSC1_MOTOR_DEBUG );

	return status;
}

