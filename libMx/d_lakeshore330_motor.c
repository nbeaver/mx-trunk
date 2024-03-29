/*
 * Name:    d_lakeshore330_motor.c 
 *
 * Purpose: MX motor driver to control LakeShore 330 temperature controllers
 *          as if they were motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006-2007, 2009-2010, 2013, 2015, 2022
 *   Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define LS330_MOTOR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "d_lakeshore330_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_ls330_motor_record_function_list = {
	NULL,
	mxd_ls330_motor_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	mxd_ls330_motor_print_motor_structure,
	mxd_ls330_motor_open,
	mxd_ls330_motor_close,
	NULL,
	mxd_ls330_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_ls330_motor_motor_function_list = {
	mxd_ls330_motor_motor_is_busy,
	mxd_ls330_motor_move_absolute,
	mxd_ls330_motor_get_position,
	mxd_ls330_motor_set_position,
	mxd_ls330_motor_soft_abort,
	mxd_ls330_motor_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler
};

/* LakeShore 330 motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_ls330_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_LS330_MOTOR_STANDARD_FIELDS
};

long mxd_ls330_motor_num_record_fields
		= sizeof( mxd_ls330_motor_record_field_defaults )
			/ sizeof( mxd_ls330_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_ls330_motor_rfield_def_ptr
			= &mxd_ls330_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_ls330_motor_get_pointers( MX_MOTOR *motor,
			MX_LS330_MOTOR **ls330_motor,
			MX_INTERFACE **port_interface,
			const char *calling_fname )
{
	static const char fname[] = "mxd_ls330_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( ls330_motor == (MX_LS330_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LS330_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MOTOR pointer passed is NULL." );
	}

	*ls330_motor = (MX_LS330_MOTOR *) (motor->record->record_type_struct);

	if ( *ls330_motor == (MX_LS330_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LS330_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	/* If the port_interface pointer is NULL, the calling routine
	 * is not interested in port interface.
	 */

	if ( port_interface == (MX_INTERFACE **) NULL )
		return MX_SUCCESSFUL_RESULT;

	*port_interface = &((*ls330_motor)->port_interface);

	if ( *port_interface == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( (*port_interface)->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the port interface for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_ls330_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_ls330_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_LS330_MOTOR *ls330_motor;

	/* Allocate memory for the necessary structures. */

	motor = malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	ls330_motor = malloc( sizeof(MX_LS330_MOTOR) );

	if ( ls330_motor == (MX_LS330_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_LS330_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = ls330_motor;
	record->class_specific_function_list
				= &mxd_ls330_motor_motor_function_list;

	motor->record = record;
	ls330_motor->record = record;

	/* The LakeShore 330 temperature controller motor is treated
	 * as an analog motor.
	 */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ls330_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_ls330_motor_print_motor_structure()";

	MX_MOTOR *motor = NULL;
	MX_LS330_MOTOR *ls330_motor = NULL;
	MX_INTERFACE *port_interface = NULL;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_ls330_motor_get_pointers( motor, &ls330_motor,
						&port_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);

	fprintf(file, "  Motor type          = LS330_MOTOR.\n\n");
	fprintf(file, "  name                = %s\n", record->name);

	if ( port_interface->record->mx_class == MXI_GPIB ) {
		fprintf(file,
		      "  GPIB interface      = %s:%ld\n",
			port_interface->record->name,
			port_interface->address );
	} else {
		fprintf(file,
		      "  RS-232 interface     = %s\n",
			port_interface->record->name );
	}

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
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
mxd_ls330_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_ls330_motor_open()";

	MX_LS330_MOTOR *ls330_motor = NULL;
	MX_INTERFACE *port_interface = NULL;
	MX_RS232 *rs232 = NULL;
	MX_GPIB *gpib = NULL;
	long gpib_address;
	unsigned long read_terminator, write_terminator;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mx_status = mxd_ls330_motor_get_pointers(
			(MX_MOTOR *) record->record_class_struct,
			&ls330_motor, &port_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( port_interface->record->mx_class == MXI_GPIB ) {
		/* The GPIB interface is being used. */

		gpib = (MX_GPIB *) port_interface->record->record_class_struct;

		gpib_address = ls330_motor->port_interface.address;

		read_terminator = gpib->read_terminator[ gpib_address - 1 ];
		write_terminator = gpib->write_terminator[ gpib_address - 1 ];

		if ( read_terminator != write_terminator ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The LakeShore 330 controller '%s' requires that the read terminator %#lx "
"and the write terminator %#lx for the GPIB bus to be the same.",
			record->name, read_terminator, write_terminator );
		}
		if ( (write_terminator != 0x0d0a)
		  && (write_terminator != 0x0a0d)
		  && (write_terminator != 0x0a) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The LakeShore 330 controller '%s' requires that the read and write "
"terminators be either 0x0d0a, 0x0a0d, or 0x0a.  Instead saw %#lx.",
				record->name, write_terminator );
		}

	} else {
		/* The RS-232 interface is being used. */

		rs232 = (MX_RS232 *)
				port_interface->record->record_class_struct;

		if ( rs232 == (MX_RS232 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"MX_RS232 pointer for record '%s' is NULL.",
				port_interface->record->name );
		}

		if ( (rs232->speed != 300) && (rs232->speed != 1200) ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The LakeShore 330 controller '%s' requires a port speed of either 300 "
"or 1200 baud.  Instead saw %ld.", record->name, rs232->speed );
		}
		if ( rs232->word_size != 7 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The LakeShore 330 controller '%s' requires 7 bit characters.  Instead saw %ld",
				record->name, rs232->word_size );
		}
		if ( rs232->parity != 'O' ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The LakeShore 330 controller '%s' requires odd parity.  Instead saw '%c'.",
				record->name, rs232->parity );
		}
		if ( rs232->stop_bits != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The LakeShore 330 controller '%s' requires 1 stop bit.  Instead saw %ld.",
				record->name, rs232->stop_bits );
		}
		if (rs232->flow_control != 'N') {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The LakeShore 330 controller '%s' requires no flow control.  "
"Instead saw '%c'.", record->name, rs232->flow_control );
		}
		if ( (rs232->read_terminators != 0x0d0a)
		  || (rs232->write_terminators != 0x0d0a) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The LakeShore controller '%s' requires the RS-232 line terminator "
"to be a carriage return-line feed sequence.  "
"Instead saw read terminator %#lx and write terminator %#lx.",
			record->name,
			rs232->read_terminators, rs232->write_terminators);
		}
	}

	mx_status = mxd_ls330_motor_resynchronize( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ls330_motor_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_ls330_motor_close()";

	MX_LS330_MOTOR *ls330_motor = NULL;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mx_status = mxd_ls330_motor_get_pointers(
			(MX_MOTOR *) record->record_class_struct,
			&ls330_motor, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Put the LakeShore 330 controller into local control mode, just
	 * in case it was in a remote control mode.
	 */

	mx_status = mxd_ls330_motor_command( ls330_motor,
			"MODE 0", NULL, 0, LS330_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ls330_motor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_ls330_motor_resynchronize()";

	MX_LS330_MOTOR *ls330_motor = NULL;
	MX_INTERFACE *port_interface = NULL;
	char response[80], banner[40];
	size_t length;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mx_status = mxd_ls330_motor_get_pointers(
			(MX_MOTOR *) record->record_class_struct,
			&ls330_motor, &port_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any unread input or unwritten output. */

	if ( port_interface->record->mx_class == MXI_RS232 ) {

		mx_status = mx_rs232_discard_unwritten_output(
				port_interface->record, LS330_MOTOR_DEBUG );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_UNSUPPORTED) )
		{
			return mx_status;
		}

		mx_status = mx_rs232_discard_unread_input( 
				port_interface->record, LS330_MOTOR_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send a query identification command to verify that the controller
	 * is there.
	 */

	mx_status = mxd_ls330_motor_command( ls330_motor, "*IDN?",
			response, sizeof response, LS330_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that we got the identification message that we expected. */

	strlcpy( banner, "LSCI,", sizeof(banner) );

	length = strlen( banner );

	if ( strncmp( response, banner, length ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
"Did not get LakeShore 330 controller identification string in response to "
"a '*IDN?' command.  Instead got the response '%s'", response );
	}

	/* Switch to control channel A. */

	mx_status = mxd_ls330_motor_command( ls330_motor, "CCHN A",
						NULL, 0, LS330_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the units for control channel A to kelvin. */

	mx_status = mxd_ls330_motor_command( ls330_motor, "CUNI K",
						NULL, 0, LS330_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ls330_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ls330_motor_motor_is_busy()";

	MX_LS330_MOTOR *ls330_motor = NULL;
	double position, difference;
	mx_status_type mx_status;

	mx_status = mxd_ls330_motor_get_pointers( motor,
					&ls330_motor, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current temperature and compare it to the destination.
	 * If the difference is larger than the 'busy_deadband', we say
	 * that the motor is busy.
	 *
	 * Note that it is desirable to use mx_motor_get_position() here
	 * rather than calling mxd_ls330_motor_get_position() directly,
	 * since the former will update the user units and the latter
	 * will not.
	 */

	mx_status = mx_motor_get_position( motor->record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	difference = motor->raw_destination.analog - motor->raw_position.analog;

	if ( fabs(difference) >= ls330_motor->busy_deadband ) {
		motor->busy = TRUE;
	} else {
		motor->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ls330_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ls330_motor_move_absolute()";

	MX_LS330_MOTOR *ls330_motor = NULL;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_ls330_motor_get_pointers( motor,
					&ls330_motor, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Format the move command and send it. */

	snprintf( command, sizeof(command),
		"SETP %g", motor->raw_destination.analog );

	mx_status = mxd_ls330_motor_command( ls330_motor,
			command, NULL, 0, LS330_MOTOR_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_ls330_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ls330_motor_get_position()";

	MX_LS330_MOTOR *ls330_motor = NULL;
	MX_INTERFACE *port_interface = NULL;
	char response[40];
	int num_items;
	double position;
	mx_status_type mx_status;

	mx_status = mxd_ls330_motor_get_pointers( motor,
				&ls330_motor, &port_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_ls330_motor_command( ls330_motor, "CDAT?",
			response, sizeof response, LS330_MOTOR_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%lg", &position );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable position value '%s' was read.", response );
	}

	motor->raw_position.analog = position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_ls330_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ls330_motor_set_position()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"LakeShore 330 motor controller '%s' does not allow "
		"the temperature to be redefined.",
		motor->record->name );
}

MX_EXPORT mx_status_type
mxd_ls330_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_ls330_motor_soft_abort()";

	MX_LS330_MOTOR *ls330_motor = NULL;
	double position;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_ls330_motor_get_pointers( motor,
					&ls330_motor, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We implement an abort here by reading the current temperature
	 * and changing the setpoint to be the current temperature.
	 */

	mx_status = mx_motor_get_position( motor->record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Change the setpoint to the current temperature. */

	snprintf( command, sizeof(command),
		"SETP %g", motor->raw_position.analog );

	mx_status = mxd_ls330_motor_command( ls330_motor,
			command, NULL, 0, LS330_MOTOR_DEBUG );

	return mx_status;
}

/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxd_ls330_motor_command( MX_LS330_MOTOR *ls330_motor,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_ls330_motor_command()";

	MX_RECORD *port_record = NULL;
	long port_address;
	char *error_code_ptr = NULL;
	int error_code, num_items;
	mx_status_type mx_status;

	if ( ls330_motor == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LS330_MOTOR record pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer for LakeShore 330 motor '%s' was NULL.",
			ls330_motor->record->name );
	}

	port_record = ls330_motor->port_interface.record;

	if ( port_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The port interface record pointer for "
		"LakeShore 330 motor '%s' is NULL.",
			ls330_motor->record->name );
	}

	port_address = ls330_motor->port_interface.address;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: command = '%s'", fname, command));
	}

	/* Send the command string. */

	if ( port_record->mx_class == MXI_GPIB ) {
		mx_status = mx_gpib_putline( port_record, port_address,
					command, NULL, debug_flag );
	} else {
		mx_status = mx_rs232_putline( port_record,
					command, NULL, debug_flag );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response. */

	if ( response != NULL ) {
		if ( port_record->mx_class == MXI_GPIB ) {
			mx_status = mx_gpib_getline( port_record, port_address,
				response, response_buffer_length,
				NULL, debug_flag );
		} else {
			mx_status = mx_rs232_getline( port_record,
				response, response_buffer_length,
				NULL, debug_flag );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: response = '%s'", fname, response ));
		}

		if ( strncmp( response, " ER", 3 ) == 0 ) {
			error_code_ptr = response + 3;

			num_items = sscanf( error_code_ptr, "%d", &error_code );

			if ( num_items != 1 ) {
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unparseable error response '%s' from Lakeshore "
			"controller '%s' for command '%s'.",
					response, ls330_motor->record->name,
					command );
			}

			switch( error_code ) {
			case 1:
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
			"Uncorrectable non-volatile RAM error for Lakeshore "
			"controller '%s'.",
					ls330_motor->record->name );
			case 2:
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
			"An error occurred while attempting to read the "
			"internal non-volatile RAM for Lakeshore controller "
			"'%s'.  Try initializing the memory by pressing both "
			"the Escape and Units keys for 20 seconds.",
					ls330_motor->record->name );
			case 27:
				return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
			"No signal or wrong polarity signal for channel A of "
			"Lakeshore controller '%s'.",
					ls330_motor->record->name );
			case 28:
				return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
			"No signal or wrong polarity signal for channel B of "
			"Lakeshore controller '%s'.",
					ls330_motor->record->name );
			case 30:
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
			"The measured heater output for Lakeshore controller "
			"'%s' does not match the predicted output.  Perhaps "
			"there is a short in the heater wiring?",
					ls330_motor->record->name );
			default:
				return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Command '%s' for Lakeshore controller '%s' returned error code %d.",
					command, ls330_motor->record->name,
					error_code );
			}
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

