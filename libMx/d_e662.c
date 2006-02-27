/*
 * Name:    d_e662.c 
 *
 * Purpose: MX motor driver to control Physik Instrumente E-662 LVPZT
 *          position servo controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2006 Illinois Institute of Technology
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
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "d_e662.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_e662_record_function_list = {
	mxd_e662_initialize_type,
	mxd_e662_create_record_structures,
	mxd_e662_finish_record_initialization,
	mxd_e662_delete_record,
	mxd_e662_print_motor_structure,
	mxd_e662_read_parms_from_hardware,
	mxd_e662_write_parms_to_hardware,
	mxd_e662_open,
	mxd_e662_close,
	NULL,
	mxd_e662_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_e662_motor_function_list = {
	mxd_e662_motor_is_busy,
	mxd_e662_move_absolute,
	mxd_e662_get_position,
	mxd_e662_set_position,
	mxd_e662_soft_abort,
	mxd_e662_immediate_abort,
	mxd_e662_positive_limit_hit,
	mxd_e662_negative_limit_hit,
	mxd_e662_find_home_position
};

#define E662_DEBUG	FALSE

/* Physik Instrumente E662 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_e662_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_E662_STANDARD_FIELDS
};

long mxd_e662_num_record_fields
		= sizeof( mxd_e662_record_field_defaults )
			/ sizeof( mxd_e662_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_e662_rfield_def_ptr
			= &mxd_e662_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_e662_get_pointers( MX_MOTOR *motor,
			MX_E662 **e662,
			const char *calling_fname )
{
	const char fname[] = "mxd_e662_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( e662 == (MX_E662 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_E662 pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MOTOR pointer passed is NULL." );
	}

	*e662 = (MX_E662 *) (motor->record->record_type_struct);

	if ( *e662 == (MX_E662 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_E662 pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_e662_initialize_type( long type )
{
		return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_e662_create_record_structures()";

	MX_MOTOR *motor;
	MX_E662 *e662;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	e662 = (MX_E662 *) malloc( sizeof(MX_E662) );

	if ( e662 == (MX_E662 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_E662 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = e662;
	record->class_specific_function_list
				= &mxd_e662_motor_function_list;

	motor->record = record;
	e662->record = record;

	/* A Physik Instrumente E-662 motor is treated as an analog motor
	 * since commanded positions are specified in micrometers.
	 */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_finish_record_initialization( MX_RECORD *record )
{
	return mx_motor_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_e662_delete_record( MX_RECORD *record )
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
mxd_e662_print_motor_structure( FILE *file, MX_RECORD *record )
{
	const char fname[] = "mxd_e662_print_motor_structure()";

	MX_MOTOR *motor;
	MX_E662 *e662;
	double position, move_deadband;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	status = mxd_e662_get_pointers( motor, &e662, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type          = E662.\n\n");
	fprintf(file, "  name                = %s\n", record->name);
	fprintf(file, "  rs232 port          = %s\n",
						e662->rs232_record->name);

	status = mx_motor_get_position( record, &position );

	if ( status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position           = %g %s  (%g um)\n",
		motor->position, motor->units,
		motor->raw_position.analog );
	fprintf(file, "  scale              = %g %s per um.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset             = %g %s.\n",
		motor->offset, motor->units);

	fprintf(file, "  backlash           = %g %s  (%g um)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog);

	fprintf(file, "  negative limit     = %g %s  (%g um)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive limit     = %g %s  (%g um)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband      = %g %s  (%g um)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_read_parms_from_hardware( MX_RECORD *record )
{
	const char fname[] = "mxd_e662_read_parms_from_hardware()";

	double position;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	/* Update the current position in the motor structure. */

	status = mx_motor_get_position( record, &position );

	return status;
}

MX_EXPORT mx_status_type
mxd_e662_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_open( MX_RECORD *record )
{
	const char fname[] = "mxd_e662_open()";

	MX_E662 *e662;
	MX_RS232 *rs232;
	mx_status_type status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	e662 = (MX_E662 *) (record->record_type_struct);

	if ( e662 == (MX_E662 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_E662 pointer for record '%s' is NULL.", record->name );
	}

	if ( e662->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"RS-232 record pointer for E-662 record '%s' is NULL.",
			record->name );
	}

	rs232 = (MX_RS232 *) (e662->rs232_record->record_class_struct);

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 pointer for record '%s' is NULL.",
			e662->rs232_record->name );
	}

	if ( rs232->speed != 9600 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The E-662 controller '%s' requires a port speed of 9600 baud.  "
"Instead saw %ld.", record->name, rs232->speed );
	}
	if ( rs232->word_size != 8 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The E-662 controller '%s' requires 8 bit characters.  Instead saw %ld",
			record->name, rs232->word_size );
	}
	if ( rs232->parity != 'N' ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The E-662 controller '%s' requires no parity.  Instead saw '%c'.",
			record->name, rs232->parity );
	}
	if ( rs232->stop_bits != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The E-662 controller '%s' requires 1 stop bit.  Instead saw %ld.",
			record->name, rs232->stop_bits );
	}
	if ( (rs232->flow_control != 'H')
	  && (rs232->flow_control != 'R') )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The E-662 controller requires '%s' hardware RTS-CTS flow control.  "
"Instead ssaw '%c'.", record->name, rs232->flow_control );
	}
	if ( (rs232->read_terminators != 0xa)
	  || (rs232->write_terminators != 0xa) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The E-662 controller '%s' requires the RS-232 line terminator "
"to be a line feed character.  "
"Instead saw read terminator %#lx and write terminator %#lx.",
			record->name,
			rs232->read_terminators, rs232->write_terminators);
	}

	status = mxd_e662_resynchronize( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Put the E-662 into remote control mode. */

	status = mxd_e662_command( e662, "DEV:CONT REM", NULL, 0, E662_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_e662_close( MX_RECORD *record )
{
	const char fname[] = "mxd_e662_close()";

	MX_E662 *e662;
	mx_status_type status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	e662 = (MX_E662 *) (record->record_type_struct);

	if ( e662 == (MX_E662 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_E662 pointer for record '%s' is NULL.", record->name );
	}

	/* Put the E-662 into local control mode. */

	status = mxd_e662_command( e662, "DEV:CONT LOC", NULL, 0, E662_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_e662_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_e662_resynchronize()";

	MX_E662 *e662;
	char response[80], banner[40];
	size_t length;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	e662 = (MX_E662 *) record->record_type_struct;

	if ( e662 == (MX_E662 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_E662 pointer for motor is NULL." );
	}

	/* Discard any unread RS-232 input or unwritten RS-232 output. */

	status = mx_rs232_discard_unwritten_output( e662->rs232_record,
								E662_DEBUG );

	if ( status.code != MXE_SUCCESS && status.code != MXE_UNSUPPORTED )
		return status;

	status = mx_rs232_discard_unread_input( e662->rs232_record,
								E662_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Send a get version command to verify that the controller is there. */

	status = mxd_e662_command( e662, "PZT?",
			response, sizeof response, E662_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Verify that we got the identification message that we expected. */

	strcpy( banner, "PZT Ser" );

	length = strlen( banner );

	if ( strncmp( response, banner, length ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Did not get E-662 serial number in response to a 'PZT?' command.  "
"Instead got the response '%s'", response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_motor_is_busy( MX_MOTOR *motor )
{
	motor->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_move_absolute( MX_MOTOR *motor )
{
	const char fname[] = "mxd_e662_move_absolute()";

	MX_E662 *e662;
	char command[20];
	mx_status_type status;

	status = mxd_e662_get_pointers( motor, &e662, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Format the move command and send it. */

	sprintf( command, "POS %g", motor->raw_destination.analog );

	status = mxd_e662_command( e662, command, NULL, 0, E662_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxd_e662_get_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_e662_get_position()";

	MX_E662 *e662;
	char response[30];
	int num_items;
	double position;
	mx_status_type status;

	status = mxd_e662_get_pointers( motor, &e662, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_e662_command( e662, "POS?",
			response, sizeof response, E662_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	num_items = sscanf( response, "%lg", &position );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable position value '%s' was read.", response );
	}

	motor->raw_position.analog = position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_set_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_e662_set_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
"The E-662 controller does not allow the piezo position to be redefined." );
}

MX_EXPORT mx_status_type
mxd_e662_soft_abort( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_immediate_abort( MX_MOTOR *motor )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_positive_limit_hit( MX_MOTOR *motor )
{
	motor->positive_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_negative_limit_hit( MX_MOTOR *motor )
{
	motor->negative_limit_hit = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_e662_find_home_position( MX_MOTOR *motor )
{
	const char fname[] = "mxd_e662_find_home_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"The E-662 controller does not support home search operations." );
}

/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxd_e662_command( MX_E662 *e662, char *command, char *response,
		int response_buffer_length, int debug_flag )
{
	const char fname[] = "mxd_e662_command()";

	mx_status_type status;

	if ( e662 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_E662 record pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command buffer pointer for E-662 '%s' was NULL.",
			e662->record->name );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: command = '%s'", fname, command));
	}

	/* Send the command string. */

	status = mx_rs232_putline( e662->rs232_record,
					command, NULL, debug_flag );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Get the response. */

	if ( response != NULL ) {
		status = mx_rs232_getline( e662->rs232_record,
			response, response_buffer_length, NULL, debug_flag );

		if ( status.code != MXE_SUCCESS )
			return status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: response = '%s'", fname, response ));
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

