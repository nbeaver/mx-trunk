/*
 * Name:    mrs232.c
 *
 * Purpose: Motor command to send and receive ASCII strings from RS-232 ports.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2005-2008, 2011, 2013-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "motor.h"
#include "mx_rs232.h"
#include "mx_io.h"

#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

#define RS232_DEBUG		FALSE

#define RS232_GETLINE_CMD	1
#define RS232_PUTLINE_CMD	2
#define RS232_COMMAND_CMD	3
#define RS232_GET_CMD		4
#define RS232_SET_CMD		5
#define RS232_SENDFILE_CMD	6
#define RS232_DISCARD_CMD	7

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

static int
motor_rs232_sendfile( MX_RECORD *record, char *filename )
{
	MX_RS232 *rs232;
	FILE *file;
	size_t block_size, bytes_read;
	int64_t file_size, remaining_file_size;
	int saved_errno;
	char buffer[512];
	mx_status_type mx_status;

	rs232 = (MX_RS232 *) record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		fprintf( output,
		"Program bug: The MX_RS232 pointer for record '%s' is NULL.\n",
			record->name );
		return FAILURE;
	}

	fprintf( output, "Sending file '%s' to serial port '%s'.\n",
					filename, record->name );

	file_size = mx_get_file_size( filename );

	if ( file_size < 0 ) {
		saved_errno = errno;

		fprintf( output,
		"The attempt to get the size of file '%s' failed.  "
		"Errno = %d, error message = '%s'\n",
			filename, saved_errno, strerror(saved_errno) );

		return FAILURE;
	}

	file = fopen( filename, "rb" );

	if ( file == (FILE *) NULL ) {
		saved_errno = errno;

		fprintf( output,
		"The attempt to open file '%s' failed.  "
		"Errno = %d, error message = '%s'\n",
			filename, saved_errno, strerror(saved_errno) );

		return FAILURE;
	}

	remaining_file_size = file_size;
		
	block_size = sizeof(buffer);

	while( remaining_file_size > 0 ) {
		bytes_read = fread( buffer, 1, block_size, file );

		if ( bytes_read == 0 ) {
			(void) fclose( file );

			return SUCCESS;
		}
		
		mx_status = mx_rs232_write( record, buffer,
					bytes_read, NULL, RS232_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		remaining_file_size -= bytes_read;
	}

	(void) fclose( file );

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
	unsigned long num_input_bytes_available;
	char *endptr;
	mx_status_type mx_status;

	static char usage[] =
		"\n"
		"Usage: rs232 'record_name' getline\n"
		"       rs232 'record_name' putline \"text to send\"\n"
		"       rs232 'record_name' command \"text to send\"\n"
		"       rs232 'record_name' cmd \"text to send\"\n"
		"       rs232 'record_name' get signal_state\n"
		"       rs232 'record_name' set signal_state 'value'\n"
		"       rs232 'record_name' sendfile 'filename'\n"
		"       rs232 'record_name' discard\n"
		"       rs232 'record_name' get input_bytes\n\n";

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

	} else if ( strncmp( "set", argv[3], max(3,length) ) == 0 ) {
		cmd_type = RS232_SET_CMD;

	} else if ( strncmp( "sendfile", argv[3], max(1,length) ) == 0 ) {
		cmd_type = RS232_SENDFILE_CMD;

	} else if ( strncmp( "discard", argv[3], max(1,length) ) == 0 ) {
		cmd_type = RS232_DISCARD_CMD;

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
	} else if ( cmd_type == RS232_DISCARD_CMD ) {
		if ( argc != 4 ) {
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
		} else if (strncmp("input_bytes", argv[4], max(1,length)) == 0)
		{
			mx_status = mx_rs232_num_input_bytes_available( record,
						&num_input_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"rs232: %lu input bytes available for '%s'.\n",
				num_input_bytes_available, record->name );
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

	case RS232_SENDFILE_CMD:
		status = motor_rs232_sendfile( record, argv[4] );

		if ( status == FAILURE ) {
			fprintf( output,
		"rs232: The attempt to send file '%s' to port '%s' failed.\n",
				argv[4], record->name );

			return FAILURE;
		}
		break;

	case RS232_DISCARD_CMD:
		(void) mx_rs232_discard_unwritten_output( record, RS232_DEBUG );

		mx_status = mx_rs232_num_input_bytes_available( record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		fprintf( output,
		"rs232: Discarding %lu input bytes for '%s'.\n",
			num_input_bytes_available, record->name );

		(void) mx_rs232_discard_unread_input( record, RS232_DEBUG );
		break;

	default:
		fprintf( output,
		"rs232: Unrecognized command line.\n" );
		return FAILURE;
	}
	return SUCCESS;
}

