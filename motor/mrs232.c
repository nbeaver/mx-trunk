/*
 * Name:    mrs232.c
 *
 * Purpose: Motor command to send and receive ASCII strings from RS-232 ports.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2005-2008, 2011, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"
#include "mx_rs232.h"

#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

#define RS232_DEBUG		FALSE

#define RS232_GETLINE_CMD	1
#define RS232_PUTLINE_CMD	2
#define RS232_COMMAND_CMD	3
#define RS232_GET_CMD		4
#define RS232_SET_CMD		5

static int
motor_rs232_readline( MX_RECORD *record )
{
	MX_RS232 *rs232;
	static char receive_buffer[10000];
	unsigned long num_input_bytes_available;
	unsigned long transfer_flags;
	mx_bool_type ignore_nulls;
	mx_status_type mx_status;

	rs232 = (MX_RS232 *) record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		fprintf( output,
		"Program bug: The MX_RS232 pointer for record '%s' is NULL.\n",
			record->name );
		return FAILURE;
	}

	/* See if there are any characters available to read. */

	num_input_bytes_available = 0;

	mx_status = mx_rs232_num_input_bytes_available( record,
						&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	if ( num_input_bytes_available == 0 )
		return FAILURE;

	/* We need to read a string here. */

	if ( rs232->rs232_flags & MXF_232_ALWAYS_IGNORE_NULLS ) {
		ignore_nulls = TRUE;
	} else {
		ignore_nulls = FALSE;
	}

	transfer_flags = RS232_DEBUG;

	if ( ignore_nulls ) {
		transfer_flags |= MXF_232_IGNORE_NULLS;
	}

#if 1
	memset( receive_buffer, 0, sizeof(receive_buffer) );
#else
	if ( rs232->num_read_terminator_chars == 0 ) {
		memset( receive_buffer, 0, sizeof(receive_buffer) );
	}
#endif

	mx_status = mx_rs232_getline( record,
				receive_buffer, num_input_bytes_available,
				NULL, transfer_flags );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_TIMED_OUT:
		break;
	default:
		return FAILURE;
		break;
	}

	/* Print the response we received. */

	if ( ignore_nulls && ( receive_buffer[0] == '\0' ) )
	{
		/* If we should ignore receive buffers that only contain nulls,
		 * then we show nothing here.
		 */
	} else {
		fprintf( output, "'%s'\n", receive_buffer );
	}

	return SUCCESS;
}

int
motor_rs232_fn( int argc, char *argv[] )
{
	static const char cname[] = "rs232";

	MX_RECORD *record;
	int cmd_type, status;
	size_t length;
	unsigned long signal_state;
	char *endptr;
	mx_status_type mx_status;

	static char usage[] =
		"\n"
		"Usage: rs232 'record_name' getline\n"
		"       rs232 'record_name' putline \"text to send\"\n"
		"       rs232 'record_name' command \"text to send\"\n"
		"       rs232 'record_name' cmd \"text to send\"\n"
		"       rs232 'record_name' get signal_state\n"
		"       rs232 'record_name' set signal_state 'value'\n\n";

	if ( argc <= 3 ) {
		fputs( usage, output );
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == NULL ) {
		fprintf( output, "\n%s: There is no record named '%s'.\n\n",
				cname, argv[2] );
		return FAILURE;
	}

	if ( ( record->mx_superclass != MXR_INTERFACE )
	  || ( record->mx_class != MXI_RS232 ) )
	{
		fprintf( output, "\n%s: Record '%s' is not an RS-232 port.\n\n",
				cname, argv[2] );
		return FAILURE;
	}

	length = strlen( argv[3] );

	if ( strncmp( "getline", argv[3], max(4,length) ) == 0 ) {
		cmd_type = RS232_GETLINE_CMD;

	} else if ( strncmp( "putline", argv[3], max(1,length) ) == 0 ) {
		cmd_type = RS232_PUTLINE_CMD;

	} else if ( strncmp( "command", argv[3], max(1,length) ) == 0 ) {
		cmd_type = RS232_COMMAND_CMD;

	} else if ( strncmp( "cmd", argv[3], max(1,length) ) == 0 ) {
		cmd_type = RS232_COMMAND_CMD;

	} else if ( strncmp( "get", argv[3], max(1,length) ) == 0 ) {
		cmd_type = RS232_GET_CMD;

	} else if ( strncmp( "set", argv[3], max(1,length) ) == 0 ) {
		cmd_type = RS232_SET_CMD;

	} else {
		fputs( usage, output );
		return FAILURE;
	}

	if ( cmd_type == RS232_GETLINE_CMD ) {
		if ( argc != 4 ) {
			fputs( usage, output );
			return FAILURE;
		}
	} else if ( cmd_type == RS232_SET_CMD ) {
		if ( argc != 6 ) {
			fputs( usage, output );
			return FAILURE;
		}
	} else {
		if ( argc != 5 ) {
			fputs( usage, output );
			return FAILURE;
		}
	}

	/* Both putline and command send a string here. */

	if ( ( cmd_type == RS232_PUTLINE_CMD )
	  || ( cmd_type == RS232_COMMAND_CMD ) )
	{
		mx_status = mx_rs232_putline( record,
					argv[4], NULL, RS232_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	}

	switch( cmd_type ) {
	case RS232_PUTLINE_CMD:

		/* Putline is done at this point. */

		return SUCCESS;

	case RS232_GETLINE_CMD:

		status = motor_rs232_readline( record );

		if ( status == FAILURE ) {
			fprintf( output,
			"rs232: No new characters are available.\n" );

			return FAILURE;
		}
		break;

	case RS232_COMMAND_CMD:

		mx_msleep(500);

		for (;;) {
			if ( mx_user_requested_interrupt() ) {
				fprintf( output,
"Warning: The output from the 'rs232' command was interrupted before all\n"
"         of the available characters were read.\n" );

				return FAILURE;
			}

			status = motor_rs232_readline( record );

			if ( status == FAILURE )
				break;		/* Exit the for(;;) loop. */

			mx_msleep(10);
		}
		break;
	case RS232_GET_CMD:

		length = strlen( argv[4] );

		if ( strncmp( "signal_state", argv[4], max(1,length) ) == 0 ) {

			mx_status = mx_rs232_print_signal_state( record );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else {
			fputs( usage, output );
			return FAILURE;
		}
		break;
	case RS232_SET_CMD:

		length = strlen( argv[4] );

		if ( strncmp( "signal_state", argv[4], max(1,length) ) == 0 ) {

			signal_state = strtoul( argv[5], &endptr, 16 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"rs232: Supplied value '%s' is not a hexadecimal number.\n",
					argv[5] );
				return FAILURE;
			}

			mx_status = mx_rs232_set_signal_state(
						record, signal_state );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else {
			fputs( usage, output );
			return FAILURE;
		}
		break;
	default:
		fprintf( output,
		"rs232: Unrecognized command line.\n" );
		return FAILURE;
	}
	return SUCCESS;
}

