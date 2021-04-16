/*
 * Name:    d_d8.c
 *
 * Purpose: MX driver for Bruker D8 controlled motors.
 *
 * Warning: This driver has only been tested with pre-production units.  Use
 *          with commercially released controllers may need a few changes.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006, 2010, 2013, 2018, 2020-2021
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define D8_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "d_d8.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_d8_motor_record_function_list = {
	NULL,
	mxd_d8_motor_create_record_structures,
	mxd_d8_motor_finish_record_initialization,
	NULL,
	mxd_d8_motor_print_structure,
	mxd_d8_motor_open,
	mxd_d8_motor_close
};

MX_MOTOR_FUNCTION_LIST mxd_d8_motor_motor_function_list = {
	mxd_d8_motor_motor_is_busy,
	mxd_d8_motor_move_absolute,
	mxd_d8_motor_get_position,
	mxd_d8_motor_set_position,
	mxd_d8_motor_soft_abort,
	mxd_d8_motor_immediate_abort,
	mxd_d8_motor_positive_limit_hit,
	mxd_d8_motor_negative_limit_hit,
	mxd_d8_motor_raw_home_command,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler
};

/* D8 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_d8_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_D8_MOTOR_STANDARD_FIELDS
};

long mxd_d8_motor_num_record_fields
		= sizeof( mxd_d8_motor_record_field_defaults )
			/ sizeof( mxd_d8_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_d8_motor_rfield_def_ptr
			= &mxd_d8_motor_record_field_defaults[0];

/* === */

static mx_status_type
mxd_d8_motor_get_pointers( MX_MOTOR *motor,
				MX_D8_MOTOR **d8_motor,
				MX_D8 **d8,
				const char *calling_fname )
{
	static const char fname[] = "mxd_d8_motor_get_pointers()";

	MX_RECORD *d8_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( d8_motor == (MX_D8_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_D8_MOTOR pointer passed by '%s' was NULL.",
		calling_fname );
	}

	if ( d8 == (MX_D8 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_D8 pointer passed by '%s' was NULL.",
		calling_fname );
	}

	*d8_motor = (MX_D8_MOTOR *) (motor->record->record_type_struct);

	if ( *d8_motor == (MX_D8_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_D8_MOTOR pointer for motor '%s' is NULL.",
				motor->record->name );
	}

	d8_record = (MX_RECORD *) ((*d8_motor)->d8_record);

	if ( d8_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Interface record pointer for D8 motor '%s' is NULL.",
			motor->record->name );
	}

	*d8 = (MX_D8 *)(d8_record->record_type_struct);

	if ( *d8 == (MX_D8 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_D8 pointer for D8 motor record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_d8_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_d8_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_D8_MOTOR *d8_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	d8_motor = (MX_D8_MOTOR *) malloc( sizeof(MX_D8_MOTOR) );

	if ( d8_motor == (MX_D8_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_D8_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = d8_motor;
	record->class_specific_function_list
				= &mxd_d8_motor_motor_function_list;

	motor->record = record;

	/* A D8 motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_d8_motor_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_d8_motor_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_d8_motor_print_structure()";

	MX_MOTOR *motor;
	MX_RECORD *d8_record;
	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
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

	d8_motor = (MX_D8_MOTOR *) (record->record_type_struct);

	if ( d8_motor == (MX_D8_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_D8_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	d8_record = (MX_RECORD *) (d8_motor->d8_record);

	if ( d8_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Interface record pointer for D8 motor '%s' is NULL.",
			record->name );
	}

	d8 = (MX_D8 *) (d8_record->record_type_struct);

	if ( d8 == (MX_D8 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_D8 pointer for D8 motor record '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type        = D8_MOTOR motor.\n\n");

	fprintf(file, "  name              = %s\n", record->name);
	fprintf(file, "  port name         = %s\n", d8_record->name);
	fprintf(file, "  drive number      = %ld\n", d8_motor->drive_number);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
		}
	
	fprintf(file, "  position          = %g %s  (%g)\n",
			motor->position, motor->units,
			motor->raw_position.analog );
	fprintf(file, "  scale             = %g %s per D8 unit.\n",
			motor->scale, motor->units);
	fprintf(file, "  offset            = %g %s.\n",
			motor->offset, motor->units);
	
	fprintf(file, "  backlash          = %g %s  (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );
	
	fprintf(file, "  negative limit    = %g %s  (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit    = %g %s  (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband     = %g %s  (%g)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_d8_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_d8_motor_open()";

	MX_MOTOR *motor;
	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[20];
	char response[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the drive is initialized already by sending it a
	 * get position command.
	 */

	snprintf( command, sizeof(command), "AV%ld", d8_motor->drive_number );

	mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( response, "AV0.--" ) == 0 ) {

		MX_DEBUG(-2,("%s: Axis %ld is not initialized yet.",
			fname, d8_motor->drive_number));

		snprintf( command, sizeof(command),
				"AV%ld,0.00", d8_motor->drive_number );

		mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_d8_motor_close( MX_RECORD *record )
{
	/* Nothing to do. */

	return MX_SUCCESSFUL_RESULT;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_d8_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_motor_is_busy()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[80];
	char response[80];
	int num_items;
	unsigned long drive_status;
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf(command, sizeof(command), "ST%ld", 5 + d8_motor->drive_number);

	mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "ST%lu", &drive_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Drive status not found in D8 response '%s'.", response );
	}

	if ( drive_status & 0x01 ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_d8_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_move_absolute()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[200];
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the move command. */

	snprintf( command, sizeof(command), "GO%ld,%g,%g",
					d8_motor->drive_number,
					motor->raw_destination.analog,
					d8_motor->d8_speed );

	mx_status = mxi_d8_command( d8, command, NULL, 0, D8_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_d8_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_get_position()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[20];
	char response[80];
	double raw_position;
	int num_tokens;
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "AV%ld", d8_motor->drive_number );

	mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_tokens = sscanf( response, "AV%lg", &raw_position );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No position value seen in response to '%s' command.  "
		"Response seen = '%s'", command, response );
	}

	motor->raw_position.analog = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_d8_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_set_position()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[40];
	char response[80];
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "AV%ld,%g", d8_motor->drive_number,
					motor->raw_set_position.analog );

	mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "ZI%ld,0", d8_motor->drive_number );

	mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "IN%ld", d8_motor->drive_number );

	mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_d8_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_soft_abort()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "HT%ld", d8_motor->drive_number );

	mx_status = mxi_d8_command( d8, command, NULL, 0, D8_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_d8_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_immediate_abort()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Abort all drives, measurements, and macros. */

	mx_status = mxi_d8_command( d8, "HT", NULL, 0, D8_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_d8_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_positive_limit_hit()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[80];
	char response[80];
	int num_items;
	unsigned long drive_status;
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf(command, sizeof(command), "ST%ld", 5 + d8_motor->drive_number);

	mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "ST%lu", &drive_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Drive status not found in D8 response '%s'.", response );
	}

	if ( (drive_status >> 9) & 0x01 ) {
		motor->positive_limit_hit = TRUE;
	} else {
		motor->positive_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_d8_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_negative_limit_hit()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[80];
	char response[80];
	int num_items;
	unsigned long drive_status;
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf(command, sizeof(command), "ST%ld", 5 + d8_motor->drive_number);

	mx_status = mxi_d8_command( d8, command,
			response, sizeof response, D8_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "ST%lu", &drive_status );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Drive status not found in D8 response '%s'.", response );
	}

	if ( (drive_status >> 8) & 0x01 ) {
		motor->negative_limit_hit = TRUE;
	} else {
		motor->negative_limit_hit = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_d8_motor_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_d8_motor_raw_home_command()";

	MX_D8 *d8;
	MX_D8_MOTOR *d8_motor;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxd_d8_motor_get_pointers( motor,
			&d8_motor, &d8, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->raw_home_command >= 0 ) {
		snprintf( command, sizeof(command),
					"FR%ld,U", d8_motor->drive_number );
	} else {
		snprintf( command, sizeof(command),
					"FR%ld,D", d8_motor->drive_number );
	}

	mx_status = mxi_d8_command( d8, command,
					NULL, 0, D8_MOTOR_DEBUG );

	return mx_status;
}

