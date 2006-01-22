/*
 * Name:    mgpib.c
 *
 * Purpose: Motor command to send and receive ASCII strings from GPIB addresses.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"
#include "mx_gpib.h"

#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

#define GPIB_DEBUG		TRUE

#define GPIB_GETLINE_CMD	1
#define GPIB_PUTLINE_CMD	2
#define GPIB_COMMAND_CMD	3

static int
motor_gpib_readline( MX_RECORD *record, int address )
{
	static char receive_buffer[2000];
	mx_status_type mx_status;

	/* We need to read a string here. */

	mx_status = mx_gpib_getline( record, address,
				receive_buffer, sizeof( receive_buffer ),
				NULL, GPIB_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Print the response we received. */

	fprintf( output, "'%s'\n", receive_buffer );

	return SUCCESS;
}

int
motor_gpib_fn( int argc, char *argv[] )
{
	static const char cname[] = "gpib";

	MX_RECORD *record;
	int cmd_type, status, address;
	size_t length;
	mx_status_type mx_status;

	static char usage[] =
		"\n"
		"Usage: gpib 'record_name' address getline\n"
		"       gpib 'record_name' address putline \"text to send\"\n"
		"       gpib 'record_name' address command \"text to send\"\n"
		"       gpib 'record_name' address cmd \"text to send\"\n\n";

	if ( argc <= 4 ) {
		fprintf( output, usage );
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == NULL ) {
		fprintf( output, "\n%s: There is no record named '%s'.\n\n",
				cname, argv[2] );
		return FAILURE;
	}

	if ( ( record->mx_superclass != MXR_INTERFACE )
	  || ( record->mx_class != MXI_GPIB ) )
	{
		fprintf( output, "\n%s: Record '%s' is not an GPIB port.\n\n",
				cname, argv[2] );
		return FAILURE;
	}

	address = atoi( argv[3] );

	length = strlen( argv[4] );

	if ( strncmp( "getline", argv[4], max(4,length) ) == 0 ) {
		cmd_type = GPIB_GETLINE_CMD;

	} else if ( strncmp( "putline", argv[4], max(1,length) ) == 0 ) {
		cmd_type = GPIB_PUTLINE_CMD;

	} else if ( strncmp( "command", argv[4], max(1,length) ) == 0 ) {
		cmd_type = GPIB_COMMAND_CMD;

	} else if ( strncmp( "cmd", argv[4], max(1,length) ) == 0 ) {
		cmd_type = GPIB_COMMAND_CMD;

	} else {
		fprintf( output, usage );
		return FAILURE;
	}

	if ( cmd_type == GPIB_GETLINE_CMD ) {
		if ( argc != 5 ) {
			fprintf( output, usage );
			return FAILURE;
		}
	} else {
		if ( argc != 6 ) {
			fprintf( output, usage );
			return FAILURE;
		}
	}

	/* Both putline and command send a string here. */

	if ( ( cmd_type == GPIB_PUTLINE_CMD )
	  || ( cmd_type == GPIB_COMMAND_CMD ) )
	{
		mx_status = mx_gpib_putline( record, address,
					argv[5], NULL, GPIB_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	}

	switch( cmd_type ) {
	case GPIB_PUTLINE_CMD:

		/* Putline is done at this point. */

		return SUCCESS;
		break;

	case GPIB_GETLINE_CMD:

		status = motor_gpib_readline( record, address );

		if ( status == FAILURE ) {
			fprintf( output,
			"gpib: No new response is available.\n" );

			return FAILURE;
		}
		break;

	case GPIB_COMMAND_CMD:

		mx_msleep(500);

		status = motor_gpib_readline( record, address );

		if ( status == FAILURE ) {
			fprintf( output,
			"gpib: No response is available.\n" );

			return FAILURE;
		}
		break;
	default:
		fprintf( output,
		"gpib: Unrecognized command line.\n" );
		return FAILURE;
	}
	return SUCCESS;
}

