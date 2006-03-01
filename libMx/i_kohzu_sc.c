/*
 * Name:    i_kohzu_sc.c
 *
 * Purpose: MX driver for Kohzu SC-200, SC-400, and SC-800 motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_KOHZU_SC_DEBUG		FALSE

#define MXI_KOHZU_SC_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_kohzu_sc.h"
#include "d_kohzu_sc.h"

MX_RECORD_FUNCTION_LIST mxi_kohzu_sc_record_function_list = {
	NULL,
	mxi_kohzu_sc_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_kohzu_sc_open,
	NULL,
	NULL,
	mxi_kohzu_sc_resynchronize,
	mxi_kohzu_sc_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_kohzu_sc_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_KOHZU_SC_STANDARD_FIELDS
};

long mxi_kohzu_sc_num_record_fields
		= sizeof( mxi_kohzu_sc_record_field_defaults )
			/ sizeof( mxi_kohzu_sc_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_kohzu_sc_rfield_def_ptr
			= &mxi_kohzu_sc_record_field_defaults[0];

static mx_status_type mxi_kohzu_sc_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/* A private function for the use of the driver. */

static mx_status_type
mxi_kohzu_sc_get_pointers( MX_RECORD *record,
			MX_KOHZU_SC **kohzu_sc,
			MX_INTERFACE **port_interface,
			const char *calling_fname )
{
	static const char fname[] = "mxi_kohzu_sc_get_pointers()";

	MX_KOHZU_SC *kohzu_sc_pointer;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( kohzu_sc == (MX_KOHZU_SC **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KOHZU_SC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	kohzu_sc_pointer = (MX_KOHZU_SC *) record->record_type_struct;

	if ( kohzu_sc_pointer == (MX_KOHZU_SC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KOHZU_SC pointer for record '%s' is NULL.",
			record->name );
	}

	if ( kohzu_sc != (MX_KOHZU_SC **) NULL ) {
		*kohzu_sc = kohzu_sc_pointer;
	}

	if ( kohzu_sc_pointer->port_interface.record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the port interface used by "
		"Kohzu SC-x00 controller '%s' is NULL.",
			record->name );
	}

	if ( port_interface != (MX_INTERFACE **) NULL ) {
		*port_interface = &(kohzu_sc_pointer->port_interface);
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_kohzu_sc_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_kohzu_sc_create_record_structures()";

	MX_KOHZU_SC *kohzu_sc;

	/* Allocate memory for the necessary structures. */

	kohzu_sc = (MX_KOHZU_SC *) malloc( sizeof(MX_KOHZU_SC) );

	if ( kohzu_sc == (MX_KOHZU_SC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_KOHZU_SC structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = kohzu_sc;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	kohzu_sc->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_kohzu_sc_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_kohzu_sc_open()";

	MX_KOHZU_SC *kohzu_sc;
	MX_INTERFACE *port_interface;
	MX_RS232 *rs232;
	int i;
	mx_status_type mx_status;

	mx_status = mxi_kohzu_sc_get_pointers( record,
				&kohzu_sc, &port_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( port_interface->record->mx_class ) {
	case MXI_RS232:
		/* If this is an RS-232 connection, are the line terminators
		 * set correctly?
		 */

		rs232 = (MX_RS232 *)
			port_interface->record->record_class_struct;

		if ( rs232 == (MX_RS232 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RS232 pointer for port interface record '%s' "
			"used by Kohzu SC-x00 controller '%s' is NULL.",
				port_interface->record->name,
				record->name );
		}

		switch( rs232->speed ) {
		case 38400:
		case 19200:
		case 9600:
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported RS-232 speed %ld configured for "
			"RS-232 port '%s' used by Kohzu SC-x00 "
			"controller '%s'.  The supported speeds are "
			"38400, 19200, and 9600.",
				rs232->speed,
				port_interface->record->name,
				record->name );
		}

		if ( ( rs232->read_terminators != 0x0d0a )
		  || ( rs232->write_terminators != 0x0d0a ) )
		{
			return mx_error(MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
			"The read and write terminators for RS-232 "
			"interface '%s' used by Kohzu SC-x00 controller '%s' "
			"must both be 0x0d0a.  Instead, the read "
			"terminators were %#04lx and the write "
			"terminators were %#04lx",
				port_interface->record->name,
				record->name,
				rs232->read_terminators,
				rs232->write_terminators );
		}
		break;

	case MXI_GPIB:
		/* Nothing to do for this case. */

		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The port interface record '%s' for Kohzu SC-x00 "
		"controller '%s' is of unsupported type '%s'.  "
		"Only RS-232 and GPIB interfaces are supported.",
			port_interface->record->name, record->name,
			mx_get_driver_name( port_interface->record ) );
	}

	/* Initialize motor_array. */

	for ( i = 0; i < MX_MAX_KOHZU_SC_AXES; i++ ) {
		kohzu_sc->motor_array[i] = NULL;
	}

	/* Resynchronize with the controller. */

	mx_status = mxi_kohzu_sc_resynchronize( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_kohzu_sc_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_kohzu_sc_resynchronize()";

	MX_KOHZU_SC *kohzu_sc;
	MX_INTERFACE *port_interface;
	char response[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char token[80];
	mx_status_type mx_status;

	mx_status = mxi_kohzu_sc_get_pointers( record,
				&kohzu_sc, &port_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if( port_interface->record->mx_class == MXI_RS232 ) {

		/* Get rid of any existing characters in the input and output
		 * buffers and do it quietly.
		 */

		mx_status = mx_rs232_discard_unwritten_output(
				port_interface->record, FALSE );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
		case MXE_UNSUPPORTED:
			break;		/* Continue on. */
		default:
			return mx_status;
		}

		mx_status = mx_rs232_discard_unread_input(
				port_interface->record, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Verify that the controller is there by asking it for its
	 * model and version number.
	 */

	mx_status = mxi_kohzu_sc_command( kohzu_sc, "IDN",
				response, sizeof(response),
				MXI_KOHZU_SC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_kohzu_sc_select_token( response, 2,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	kohzu_sc->controller_type = mx_string_to_long( token );

	MX_DEBUG( 2,("%s: token = '%s', kohzu_sc->controller_type = %d",
		fname, token, kohzu_sc->controller_type));

	switch( kohzu_sc->controller_type ) {
	case MXT_KOHZU_SC_200:
		kohzu_sc->num_axes = 2;
		break;
	case MXT_KOHZU_SC_400:
		kohzu_sc->num_axes = 4;
		break;
	case MXT_KOHZU_SC_800:
		kohzu_sc->num_axes = 8;
		break;
	default:
		mx_warning( "Unrecognized model number %d seen for Kohzu SC-x00"
			" controller '%s' in response to an IDN command.  "
			"IDN response was '%s'.  We assume this controller "
			"has at least 8 motor axes.",
				kohzu_sc->controller_type,
				record->name, response );

		kohzu_sc->num_axes = 8;
		break;
	}

	mx_status = mxi_kohzu_sc_select_token( response, 3,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	kohzu_sc->firmware_version = mx_string_to_long( token );

	MX_DEBUG( 2,("%s: token = '%s', kohzu_sc->firmware_version = %d",
		fname, token, kohzu_sc->firmware_version));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_kohzu_sc_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_kohzu_sc_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_KOHZU_SC_COMMAND:
		case MXLV_KOHZU_SC_RESPONSE:
		case MXLV_KOHZU_SC_COMMAND_WITH_RESPONSE:
			record_field->process_function
					    = mxi_kohzu_sc_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_kohzu_sc_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_kohzu_sc_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_KOHZU_SC *kohzu_sc;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	kohzu_sc = (MX_KOHZU_SC *)
					record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_KOHZU_SC_RESPONSE:
			/* Nothing to do since the necessary string is
			 * already stored in the 'response' field.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_KOHZU_SC_COMMAND:
			mx_status = mxi_kohzu_sc_command(
				kohzu_sc,
				kohzu_sc->command,
				NULL, 0, MXI_KOHZU_SC_DEBUG );

			break;
		case MXLV_KOHZU_SC_COMMAND_WITH_RESPONSE:
			mx_status = mxi_kohzu_sc_command(
				kohzu_sc,
				kohzu_sc->command,
				kohzu_sc->response,
				MXU_KOHZU_SC_MAX_COMMAND_LENGTH,
				MXI_KOHZU_SC_DEBUG );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

/* === Functions specific to this driver. === */

#if MXI_KOHZU_SC_DEBUG_TIMING	
#include "mx_hrt_debug.h"
#endif

static void
mxi_kohzu_sc_handle_warning( MX_KOHZU_SC *kohzu_sc,
					char *response_buffer )
{
	char token[80];
	int warning_code;
	mx_status_type mx_status;

	mx_status = mxi_kohzu_sc_select_token( response_buffer, 2,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	warning_code = (int) mx_string_to_long( token );

	switch( warning_code ) {
	case 1:
		mx_warning( "The target position and present position "
			"are the same for Kohzu controller '%s'.",
				kohzu_sc->record->name );
		break;
	case 2:
		mx_warning( "In one move setting, waiting time is "
			"designated with OSC command "
			"for Kohzu controller '%s'.",
				kohzu_sc->record->name );
		break;
	case 100:
		mx_warning( "Designated address to which a coordinate "
			"is not registered by the TPS command "
			"for Kohzu controller '%s'.",
				kohzu_sc->record->name );
		break;
	default:
		mx_warning( "Warning code %d returned by Kohzu controller '%s'",
				warning_code, kohzu_sc->record->name );
		break;
	}

	return;
}

static mx_status_type
mxi_kohzu_sc_handle_error( MX_KOHZU_SC *kohzu_sc, char *response_buffer )
{
	static const char fname[] = "mxi_kohzu_sc_handle_error()";

	char token[80];
	int error_code;
	mx_status_type mx_status;

	mx_status = mxi_kohzu_sc_select_token( response_buffer, 2,
						token, sizeof(token) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	error_code = (int) mx_string_to_long( token );

	if ( ( error_code >= 101 ) && ( error_code <= 107 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The numerical value of parameter %d is out of range "
		"for Kohzu controller '%s'",
			error_code - 100,
			kohzu_sc->record->name );
	}

	switch( error_code ) {
	case 1:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The first byte in the command sent to Kohzu controller '%s' "
		"was not an STX character.", kohzu_sc->record->name );
	case 2:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"'Total number of commands is short'"
		" error for Kohzu controller '%s'", kohzu_sc->record->name );
	case 3:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"No CR character was found in the command sent to "
		"Kohzu controller '%s'", kohzu_sc->record->name );
	case 4:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Illegal characters were found in the command sent to "
		"Kohzu controller '%s'", kohzu_sc->record->name );
	case 5:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Illegal command error for "
		"Kohzu controller '%s'", kohzu_sc->record->name );
	case 10:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Kohzu controller '%s' is currently in manual mode, "
		"so it cannot accept any commands from the RS-232 port.",
			kohzu_sc->record->name );
	case 100:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The total number of paramters for the command sent "
		"to Kohzu controller '%s' is incorrect.",
			kohzu_sc->record->name );
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error code %d returned by Kohzu controller '%s'",
			error_code, kohzu_sc->record->name );
	}

#if defined(__BORLANDC__)
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxi_kohzu_sc_command( MX_KOHZU_SC *kohzu_sc, char *command,
		char *response, int max_response_length,
		int debug_flag )
{
	static const char fname[] = "mxi_kohzu_sc_command()";

	MX_INTERFACE *port_interface;
	char command_buffer[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char response_buffer[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char response_status;
	mx_status_type mx_status;

#if MXI_KOHZU_SC_DEBUG_TIMING	
	MX_HRT_RS232_TIMING command_timing, response_timing;
#endif

	if ( kohzu_sc == (MX_KOHZU_SC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KOHZU_SC pointer passed was NULL." );
	}

	port_interface = &(kohzu_sc->port_interface);

	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	/* Format the command buffer for the Kohzu.
	 *
	 * The first character must always be an ASCII STX (0x2) character.
	 *
	 * At the end, a CR-LF pair must be appended, but mx_..._putline()
	 * will take care of that part for us.
	 */

	command_buffer[0] = MX_STX;
	command_buffer[1] = '\0';

	strncat( command_buffer, command, sizeof(command_buffer) - 1 );

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, kohzu_sc->record->name));
	}

#if MXI_KOHZU_SC_DEBUG_TIMING	
	MX_HRT_RS232_START_COMMAND( command_timing, 2 + strlen(command) );
#endif

	if ( port_interface->record->mx_class == MXI_RS232 ) {
		mx_status = mx_rs232_putline( port_interface->record,
						command_buffer, NULL, 0 );
	} else {
		mx_status = mx_gpib_putline( port_interface->record,
						port_interface->address,
						command_buffer, NULL, 0 );
	}

#if MXI_KOHZU_SC_DEBUG_TIMING
	MX_HRT_RS232_END_COMMAND( command_timing );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_KOHZU_SC_DEBUG_TIMING
	MX_HRT_RS232_COMMAND_RESULTS( command_timing, command, fname );
#endif

	/* Get the response. */

#if MXI_KOHZU_SC_DEBUG_TIMING
	MX_HRT_RS232_START_RESPONSE( response_timing, NULL );
#endif

	if ( port_interface->record->mx_class == MXI_RS232 ) {
		mx_status = mx_rs232_getline( port_interface->record,
						response_buffer,
						sizeof( response_buffer ),
						NULL, 0 );
	} else {
		mx_status = mx_gpib_getline( port_interface->record,
						port_interface->address,
						response_buffer,
						sizeof( response_buffer ),
						NULL, 0 );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_KOHZU_SC_DEBUG_TIMING
	MX_HRT_RS232_END_RESPONSE( response_timing, strlen(response) );

	MX_HRT_TIME_BETWEEN_MEASUREMENTS( command_timing,
						response_timing, fname );

	MX_HRT_RS232_RESPONSE_RESULTS( response_timing,
						response_buffer, fname );
#endif

	/* Was there a warning or error in the response?  If so,
	 * return an appropriate error to the caller.
	 */

	response_status = response_buffer[0];

	switch( response_status ) {
	case 'C':
		/* There was no error. */
		break;
	case 'W':
		mxi_kohzu_sc_handle_warning( kohzu_sc, response_buffer );
		break;
	case 'E':
		return mxi_kohzu_sc_handle_error( kohzu_sc, response_buffer );
	default:
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The first character '%c' of response '%s' to command '%s' "
		"by Kohzu SC-x00 controller '%s' is not 'C', 'W', or 'E'.",
			response_status, response_buffer, command,
			kohzu_sc->record->name );
	}

	/* If the caller wanted a response, extract it from the
	 * response buffer.
	 */

	if ( response != (char *) NULL ) {

		strlcpy( response, response_buffer, max_response_length );

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response,
				kohzu_sc->record->name));
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_kohzu_sc_multiaxis_move( MX_KOHZU_SC *kohzu_sc,
				unsigned long num_motors,
				MX_RECORD **motor_record_array,
				double *motor_position_array,
				int simultaneous_start )
{
	static const char fname[] = "mxi_kohzu_sc_multiaxis_move()";

	MX_MOTOR *motor;
	MX_KOHZU_SC_MOTOR *kohzu_sc_motor;
	double destination;
	long axis, target_position;
	unsigned long i;
	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char buffer[80];
	mx_status_type mx_status;

	if ( num_motors < 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested Kohzu SC-x00 '%s' multiaxis move must move "
		"at least 2 axes, but only %lu were supplied.",
			kohzu_sc->record->name, num_motors );
	}

	if ( num_motors > kohzu_sc->num_axes ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The requested Kohzu SC-x00 '%s' multiaxis move wanted "
		"to move %lu axes, but the controller itself only has "
		"%d axes.", kohzu_sc->record->name, num_motors,
			kohzu_sc->num_axes );
	}

	/* Are we using a simultaneous start or are we using the 
	 * linear interpolation functionality?
	 */

	if ( simultaneous_start ) {
		sprintf( command, "MPS" );
	} else {
		sprintf( command, "SPS" );

		if ( num_motors > 3 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"No more than 3 axes may be used for linear "
			"interpolation moves by Kohzu SC-x00 '%s'.",
				kohzu_sc->record->name );
		}
	}

	/* Construct the command to set the destinations. */

	for ( i = 0; i < num_motors; i++ ) {
		kohzu_sc_motor = (MX_KOHZU_SC_MOTOR *)
				motor_record_array[i]->record_type_struct;

		axis = kohzu_sc_motor->axis_number;

		MX_DEBUG(-2,("%s: motor_record_array[%lu] = %p",
				fname, i, motor_record_array[i]));

		MX_DEBUG(-2,("%s: motor_record_array[%lu] = '%s'",
				fname, i, motor_record_array[i]->name));

		motor = (MX_MOTOR *) motor_record_array[i]->record_class_struct;

		MX_DEBUG(-2,("%s: motor_position_array[%lu] = %g, scale = %g",
			fname, i, motor_position_array[i], motor->scale ));

		destination = mx_divide_safely( motor_position_array[i],
						motor->scale );

		MX_DEBUG(-2, ("%s: axis = %d, destination = %g",
			fname, axis, destination ));

		target_position = mx_round( destination );

		MX_DEBUG(-2, ("%s: axis = %d, target_position = %ld",
			fname, axis, target_position ));

		sprintf( buffer, "%ld/%ld/", axis, target_position );

		strcat( command, buffer );
	}

	if ( simultaneous_start ) {
		strcat( command, "1" );
	} else {
		strcat( command, "2/0/0/0/0/0/1" );
	}

	MX_DEBUG(-2,("%s: command = '%s'", fname, command));

	/* Start the move. */

	mx_status = mxi_kohzu_sc_command( kohzu_sc, command,
			NULL, 0, MXI_KOHZU_SC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_kohzu_sc_select_token( char *response_buffer,
				unsigned int token_number,
				char *token_buffer,
				int max_token_length )
{
	static const char fname[] = "mxi_kohzu_sc_select_token()";

	char *ptr, *ptr_end, *next_ptr;
	unsigned int i, token_length;

	if ( response_buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"response_buffer pointer passed was NULL." );
	}
	if ( token_buffer == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"token_buffer pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s: response_buffer = '%s'", fname, response_buffer));

	/* Find the start of the token. */

	ptr = response_buffer;

	for ( i = 0; i < token_number; i++ ) {
		next_ptr = strchr( ptr, '\t' );

		MX_DEBUG( 2,("%s: #%u ptr = '%s', next_ptr = '%s'",
			fname, i, ptr, next_ptr));

		if ( next_ptr == (char *) NULL ) {
			return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
			"Cannot return token %u from response, since the "
			"response only has %u tokens.  Response = '%s'",
				token_number, i+1, response_buffer );
		}

		ptr = next_ptr + 1;
	}

	/* Find out how many characters are in the token. */

	ptr_end = strchr( ptr, '\t' );

	if ( ptr_end == (char *) NULL ) {
		token_length = (int) strlen( ptr );
	} else {
		token_length = (int) (ptr_end - ptr);
	}

	MX_DEBUG( 2,("%s: ptr = %p, ptr_end = %p, token_length = %u",
		fname, ptr, ptr_end, token_length));

	/* Copy the token. */

	if ( token_length > max_token_length ) {
		token_length = max_token_length - 1;
	}

	strlcpy( token_buffer, ptr, token_length + 1 );

	MX_DEBUG( 2,("%s: token %u = '%s'",
		fname, token_number, token_buffer ));

	return MX_SUCCESSFUL_RESULT;
}

