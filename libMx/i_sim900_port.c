/*
 * Name:    i_sim900_port.c
 *
 * Purpose: MX driver for connecting to SIM900 ports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SIM900_PORT_DEBUG		FALSE

#define MXI_SIM900_PORT_DEBUG_GETN	FALSE

#define MXI_SIM900_PORT_DEBUG_GETCHAR	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_sim900.h"
#include "i_sim900_port.h"

MX_RECORD_FUNCTION_LIST mxi_sim900_port_record_function_list = {
	NULL,
	mxi_sim900_port_create_record_structures,
	mxi_sim900_port_finish_record_initialization,
	NULL,
	NULL,
	mxi_sim900_port_open,
	mxi_sim900_port_close
};

MX_RS232_FUNCTION_LIST mxi_sim900_port_rs232_function_list = {
	mxi_sim900_port_getchar,
	mxi_sim900_port_putchar,
	mxi_sim900_port_read,
	mxi_sim900_port_write,
	mxi_sim900_port_getline,
	mxi_sim900_port_putline,
	mxi_sim900_port_num_input_bytes_available,
	mxi_sim900_port_discard_unread_input,
	NULL,
	NULL,
	NULL,
	mxi_sim900_port_get_configuration,
	mxi_sim900_port_set_configuration,
	mxi_sim900_port_send_break
};

MX_RECORD_FIELD_DEFAULTS mxi_sim900_port_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_SIM900_PORT_STANDARD_FIELDS
};

long mxi_sim900_port_num_record_fields
		= sizeof( mxi_sim900_port_record_field_defaults )
			/ sizeof( mxi_sim900_port_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_sim900_port_rfield_def_ptr
			= &mxi_sim900_port_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_sim900_port_get_pointers( MX_RS232 *rs232,
			MX_SIM900_PORT **sim900_port,
			MX_SIM900 **sim900,
			const char *calling_fname )
{
	static const char fname[] = "mxi_sim900_port_get_pointers()";

	MX_RECORD *sim900_port_record;
	MX_RECORD *sim900_record;
	MX_SIM900_PORT *sim900_port_ptr;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	sim900_port_record = rs232->record;

	if ( sim900_port_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The sim900_port_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	sim900_port_ptr = (MX_SIM900_PORT *)
				sim900_port_record->record_type_struct;

	if ( sim900_port_ptr == (MX_SIM900_PORT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIM900_PORT pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	if ( sim900_port != (MX_SIM900_PORT **) NULL ) {
		*sim900_port = sim900_port_ptr;
	}

	if ( sim900 != (MX_SIM900 **) NULL ) {
		sim900_record = sim900_port_ptr->sim900_record;

		if ( sim900_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The sim900_record pointer for record '%s' is NULL.",
				rs232->record->name );
		}

		*sim900 = (MX_SIM900 *) sim900_record->record_type_struct;

		if ( (*sim900) == (MX_SIM900 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SIM900 pointer for SIM900 record '%s' "
			"used by record '%s' is NULL.",
				sim900_record->name,
				rs232->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

static long
mxi_sim900_port_build_escaped_string( char *input_buffer,
					size_t input_buffer_length,
					char *output_buffer,
					size_t output_buffer_length )
{
	long i, j;

	for ( i = 0, j = 0;
	    (i < input_buffer_length) && (j < output_buffer_length) ;
	    i++, j++ )
	{
		output_buffer[i] = input_buffer[j];

		if ( input_buffer[j] == '\'' ) {
			i++;
			output_buffer[i] = '\'';
		}
		if ( input_buffer[j] == '\0' ) {
			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= input_buffer_length ) {
		i--;
	}

	return i;
}

/*----*/

MX_EXPORT mx_status_type
mxi_sim900_port_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_sim900_port_create_record_structures()";

	MX_RS232 *rs232;
	MX_SIM900_PORT *sim900_port;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	sim900_port = (MX_SIM900_PORT *) malloc( sizeof(MX_SIM900_PORT) );

	if ( sim900_port == (MX_SIM900_PORT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SIM900_PORT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = sim900_port;
	record->class_specific_function_list =
			&mxi_sim900_port_rs232_function_list;

	rs232->record = record;
	sim900_port->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sim900_port_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_sim900_port_finish_record_initialization()";

	MX_RS232 *rs232;
	MX_SIM900_PORT *sim900_port;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check for a valid port name. */

	switch( sim900_port->port_name ) {
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'A':
	case 'B':
	case 'C':
	case 'D':
		break;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
		sim900_port->port_name =
			toupper( (int) sim900_port->port_name );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invalid port name '%c' requested for RS-232 record '%s'.  "
		"The valid names are 1 to 9 and A, B, C, or D.",
			sim900_port->port_name, record->name );
		break;
	}

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the SIM900 RS-232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sim900_port_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_sim900_port_open()";

	MX_RS232 *rs232;
	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[100];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SIM900_PORT_DEBUG
	MX_DEBUG(-2, ("%s invoked by record '%s' for SIM900 '%s', port '%c'",
	fname, record->name, sim900->record->name, sim900_port->port_name));
#endif
	/* Ports C and D need special processing to be used as
	 * general purpose ports.
	 */

	if ( sim900_port->port_name == 'C' ) {
		mx_status = mxi_sim900_command( sim900, "PRTC PORT",
					NULL, 0, MXI_SIM900_PORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( sim900_port->port_name == 'D' ) {
		MX_RECORD *interface_record;

		interface_record = sim900->sim900_interface.record;

		if ( interface_record->mx_class != MXI_GPIB ) {
			return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
			"Record '%s' cannot use port 'D' of SIM900 '%s' as "
			"an RS-232 interface, since that can only be done "
			"if the SIM900 is using its GPIB interface.",
				record->name, sim900->record->name );
		}

		mx_status = mxi_sim900_command( sim900, "PRTD PORT",
					NULL, 0, MXI_SIM900_PORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Force the driver to use buffered I/O, since that lets the
	 * SIM900 automatically take care of line terminations.
	 */

	rs232->rs232_flags &= ( ~MXF_232_UNBUFFERED_IO );

	/* Tell the SIM900 what write terminators to use. */

	snprintf( command, sizeof(command), "TERM %c,",
					sim900_port->port_name );

	switch( rs232->write_terminators ) {
	case 0x0d:
		strlcat( command, "CR", sizeof(command) );
		break;
	case 0x0a:
		strlcat( command, "LF", sizeof(command) );
		break;
	case 0x0d0a:
		strlcat( command, "CRLF", sizeof(command) );
		break;
	case 0x0a0d:
		strlcat( command, "LFCR", sizeof(command) );
		break;
	case 0x0:
		strlcat( command, "NONE", sizeof(command) );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported write terminators %#lx requested for SIM900 "
		"RS-232 port '%s'.",
			rs232->write_terminators, rs232->record->name );
		break;
	}

	mx_status = mxi_sim900_command( sim900, command,
					NULL, 0, MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the standard RS-232 configuration commands. */

	mx_status = mxi_sim900_port_set_configuration( rs232 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sim900_port_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_sim900_port_close()";

	MX_RS232 *rs232;
	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sim900_port_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_sim900_port_getchar()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[40];
	mx_status_type mx_status;

#if MXI_SIM900_PORT_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The character pointer passed was NULL." );
	}

	snprintf( command, sizeof(command), "RAWN? %c,1",
					sim900_port->port_name );

	mx_status = mxi_sim900_command( sim900, command,
					c, 1, MXI_SIM900_PORT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sim900_port_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_sim900_port_putchar()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[40];
	mx_status_type mx_status;

#if MXI_SIM900_PORT_DEBUG
	MX_DEBUG(-2, ("%s invoked.  c = 0x%x, '%c'", fname, c, c));
#endif

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c == '\'' ) {
		snprintf( command, sizeof(command), "SEND %c,''''",
			sim900_port->port_name );
	} else {
		snprintf( command, sizeof(command), "SEND %c,'%c'",
			sim900_port->port_name, c );
	}

	mx_status = mxi_sim900_command( sim900, command,
					NULL, 0, MXI_SIM900_PORT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sim900_port_read( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_sim900_port_read()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "RAWN? %c,%lu",
				sim900_port->port_name,
				(unsigned long) max_bytes_to_read );

	mx_status = mxi_sim900_command( sim900, command,
					buffer, max_bytes_to_read,
					MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != (size_t *) NULL ) {
		*bytes_read = max_bytes_to_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sim900_port_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_sim900_port_write()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[2500];
	size_t bytes_to_write, initial_length, escaped_length;
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bytes_to_write = strlen( buffer ) + 1;

	if ( bytes_to_write > max_bytes_to_write ) {
		bytes_to_write = max_bytes_to_write;
	}

	snprintf( command, sizeof(command), "SEND %c,'",
					sim900_port->port_name );

	initial_length = strlen( command );

	escaped_length = mxi_sim900_port_build_escaped_string(
					buffer,
					bytes_to_write,
					command + initial_length,
					sizeof(command) - initial_length );

	strlcat( command, "'",
		sizeof(command) - initial_length - escaped_length );

	mx_status = mxi_sim900_command( sim900, command,
					NULL, 0, MXI_SIM900_PORT_DEBUG );

	if ( bytes_written != NULL ) {
		*bytes_written = bytes_to_write;
	}

	return mx_status;
}

/* MXP_GETN_PREFIX_LENGTH describes the number of bytes in the #3aaa
 * prefix returned by GETN?
 */

#define MXP_GETN_PREFIX_LENGTH	5

static mx_status_type
mxi_sim900_port_getn( MX_RS232 *rs232,
			MX_SIM900_PORT *sim900_port,
			MX_SIM900 *sim900,
			char *buffer,
			size_t max_bytes_to_read_including_prefix,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_sim900_port_getn()";

	char command[40];
	char *termarray;
	unsigned long i, j, max_bytes, length_from_header;
	unsigned long total_message_length, num_terminators;
	double getchar_timeout;
	mx_status_type mx_status;

	length_from_header = 0;

	if ( max_bytes_to_read_including_prefix < MXP_GETN_PREFIX_LENGTH ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The read buffer of '%s' for getn must be at least %d "
		"bytes long, but is actually %lu bytes long.",
			sim900_port->record->name,
			MXP_GETN_PREFIX_LENGTH,
			(unsigned long) max_bytes_to_read_including_prefix );
	}

	max_bytes = max_bytes_to_read_including_prefix - MXP_GETN_PREFIX_LENGTH;

	/* By experimentation, it appears that attempts to request more
	 * than 128 bytes at a time fails, so we must truncate max_bytes
	 * at 128.
	 */

	if ( max_bytes > 128 ) {
		max_bytes = 128;
	}

	/* Send the command to the SIM900. */

	snprintf( command, sizeof(command), "GETN? %c,%lu",
					sim900_port->port_name, max_bytes );

	/* Send the command. */

	mx_status = mxi_sim900_command( sim900, command, NULL, 0,
					MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sim900->sim900_interface.record->mx_class == MXI_GPIB ) {
		/* GPIB is _UNTESTED_! */

		mx_status = mx_gpib_getline( sim900->sim900_interface.record,
					sim900->sim900_interface.address,
					buffer,
					max_bytes_to_read_including_prefix,
					NULL,
					MXI_SIM900_PORT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( sim900->sim900_interface.record->mx_class == MXI_RS232 ) {
		/* RS232 is more difficult to work with. */

		/* Read back the prefix of the command. */

		getchar_timeout = 1.0;

		for ( i = 0; i < MXP_GETN_PREFIX_LENGTH; i++ ) {

			mx_status = mx_rs232_getchar_with_timeout(
						sim900->sim900_interface.record,
						&buffer[i],
						MXI_SIM900_PORT_DEBUG,
						getchar_timeout );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if MXI_SIM900_PORT_DEBUG_GETCHAR
			MX_DEBUG(-2,("%s: buffer[%lu] = %#x '%c'",
				fname, i, buffer[i], buffer[i]));
#endif
		}

		/* Is there a prefix at the start of the message? */

		if ( buffer[0] != '#' ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"The first byte '%c' of the response '%s' to the "
			"command '%s' sent to '%s' is not "
			"the prefix character '#'.",
			buffer[0], buffer, command, sim900_port->record->name );
		}

		/* How many bytes does the message header say
		 * are in the message?
		 */

		length_from_header = 100 * ( buffer[2] - '0' )
				    + 10 * ( buffer[3] - '0' )
				         + ( buffer[4] - '0' );

		num_terminators = rs232->num_read_terminator_chars;

		total_message_length =
		  MXP_GETN_PREFIX_LENGTH + length_from_header + num_terminators;

#if MXI_SIM900_PORT_DEBUG_GETCHAR
		MX_DEBUG(-2,("%s: total_message_length = %lu",
			fname, total_message_length));
#endif

		/* Read in the body of the message. */

		for ( i = MXP_GETN_PREFIX_LENGTH;
			i < ( total_message_length - num_terminators );
			i++ )
		{
			mx_status = mx_rs232_getchar_with_timeout(
						sim900->sim900_interface.record,
						&buffer[i],
						MXI_SIM900_PORT_DEBUG,
						getchar_timeout );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if MXI_SIM900_PORT_DEBUG_GETCHAR
			MX_DEBUG(-2,("%s: buffer[%lu] = %#x '%c'",
				fname, i, buffer[i], buffer[i]));
#endif
		}

		/* Read in and discard the terminators. */

		termarray = rs232->read_terminator_array;

		for ( i = (MXP_GETN_PREFIX_LENGTH + length_from_header), j = 0;
			i < total_message_length;
			i++, j++ )
		{
			mx_status = mx_rs232_getchar_with_timeout(
						sim900->sim900_interface.record,
						&buffer[i],
						MXI_SIM900_PORT_DEBUG,
						getchar_timeout );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if MXI_SIM900_PORT_DEBUG_GETCHAR
			MX_DEBUG(-2,("%s: buffer[%lu] = %#x '%c'",
				fname, i, buffer[i], buffer[i]));
#endif
			if ( buffer[i] != termarray[j] ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Did not see the expected read terminators "
				"%#lx in the buffer '%s' read from '%s'.",
					rs232->read_terminators,
					buffer, sim900->record->name );
			}

			buffer[i] = '\0';
		}
	}

#if MXI_SIM900_PORT_DEBUG_GETN
	MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#else
	if ( sim900->sim900_flags & MXF_SIM900_DEBUG ) {
		MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
	}
#endif

	/* Move the bytes after the prefix to their proper position at 
	 * the start of the buffer.
	 */

	memmove( buffer, buffer + MXP_GETN_PREFIX_LENGTH,
			length_from_header );

	buffer[length_from_header] = '\0';

	if ( bytes_read != NULL ) {
		*bytes_read = length_from_header;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sim900_port_getline( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_sim900_port_getline()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char *buffer_ptr, *terminators_ptr;
	size_t bytes_left_in_buffer, bytes_read_so_far, bytes_read_by_getn;
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	buffer_ptr = buffer;
	bytes_left_in_buffer = max_bytes_to_read;
	bytes_read_so_far = 0;

	bytes_read_by_getn = 0;

	while (1) {
		mx_status = mxi_sim900_port_getn( rs232, sim900_port, sim900,
						buffer_ptr,
						bytes_left_in_buffer,
						&bytes_read_by_getn );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_read_so_far += bytes_read_by_getn;

		/* See if the read terminators are found in the data
		 * returned by GETN.
		 */

		terminators_ptr = mx_rs232_find_terminators( buffer_ptr,
					bytes_left_in_buffer,
					rs232->num_read_terminator_chars,
					rs232->read_terminator_array );

		if ( terminators_ptr != NULL ) {
			*terminators_ptr = '\0';

			if ( bytes_read != NULL ) {
				*bytes_read = bytes_read_so_far
					- rs232->num_read_terminator_chars;
			}

#if MXI_SIM900_PORT_DEBUG
			MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, buffer, rs232->record->name));
#endif
			return MX_SUCCESSFUL_RESULT;
		}

		if ( bytes_read_by_getn >= bytes_left_in_buffer ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The total response beginning with '%s' from '%s'"
			"would exceed the specified buffer size of %lu.",
				buffer, rs232->record->name,
				(unsigned long) max_bytes_to_read );
		}

		buffer_ptr += bytes_read_by_getn;
		bytes_left_in_buffer -= bytes_read_by_getn;
	}

#if !defined(OS_SOLARIS)
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxi_sim900_port_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_sim900_port_putline()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[2500];
	size_t bytes_to_write, initial_length, escaped_length;
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bytes_to_write = strlen( buffer ) + 1;

	snprintf( command, sizeof(command), "SNDT %c,'",
					sim900_port->port_name );

	initial_length = strlen( command );

	escaped_length = mxi_sim900_port_build_escaped_string(
					buffer,
					bytes_to_write,
					command + initial_length,
					sizeof(command) - initial_length );

	strlcat( command, "'",
		sizeof(command) - initial_length - escaped_length );

	mx_status = mxi_sim900_command( sim900, command,
					NULL, 0, MXI_SIM900_PORT_DEBUG );

	if ( bytes_written != NULL ) {
		*bytes_written = bytes_to_write;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sim900_port_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
			"mxi_sim900_port_num_input_bytes_available()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[40];
	char response[40];
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "NINP? %c",
					sim900_port->port_name );

	mx_status = mxi_sim900_command( sim900, command,
					response, sizeof(response),
					MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->num_input_bytes_available = atol( response );

#if MXI_SIM900_PORT_DEBUG
	MX_DEBUG(-2,("%s: num_input_bytes_available = %ld",
			fname, rs232->num_input_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sim900_port_discard_unread_input( MX_RS232 *rs232 )
{
	char buffer[2500];
	long total_bytes_to_read, bytes_to_read;
	size_t bytes_read;
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_num_input_bytes_available( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	total_bytes_to_read = rs232->num_input_bytes_available;

	while ( total_bytes_to_read > 0 ) {
		if ( total_bytes_to_read >= sizeof(buffer) ) {
			bytes_to_read = sizeof(buffer);
		} else {
			bytes_to_read = total_bytes_to_read;
		}

		mx_status = mxi_sim900_port_read( rs232, buffer,
						bytes_to_read,
						&bytes_read );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		total_bytes_to_read -= bytes_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sim900_port_get_configuration( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_sim900_port_get_configuration()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[100];
	char response[100];
	long parity_token, flow_token;
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the baud rate. */

	snprintf( command, sizeof(command), "BAUD? %c",
					sim900_port->port_name );

	mx_status = mxi_sim900_command( sim900, command,
					response, sizeof(response),
					MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->speed = atol( response );

	/* Get the word size. */

	snprintf( command, sizeof(command), "WORD? %c",
					sim900_port->port_name );

	mx_status = mxi_sim900_command( sim900, command,
					response, sizeof(response),
					MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->word_size = atol( response );

	/* Get the parity. */

	snprintf( command, sizeof(command), "PARI? %c",
					sim900_port->port_name );

	mx_status = mxi_sim900_command( sim900, command,
					response, sizeof(response),
					MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	parity_token = atol( response );

	switch( parity_token ) {
	case 0:
		rs232->parity = MXF_232_NO_PARITY;
		break;
	case 1:
		rs232->parity = MXF_232_ODD_PARITY;
		break;
	case 2:
		rs232->parity = MXF_232_EVEN_PARITY;
		break;
	case 3:
		rs232->parity = MXF_232_MARK_PARITY;
		break;
	case 4:
		rs232->parity = MXF_232_SPACE_PARITY;
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unrecognized parity value %ld returned for '%s'.",
			parity_token, rs232->record->name );
		break;
	}

	/* Get the stop bits. */

	snprintf( command, sizeof(command), "SBIT? %c",
					sim900_port->port_name );

	mx_status = mxi_sim900_command( sim900, command,
					response, sizeof(response),
					MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->stop_bits = atol( response );

	/* Get the flow control. */

	snprintf( command, sizeof(command), "FLOW? %c",
					sim900_port->port_name );

	mx_status = mxi_sim900_command( sim900, command,
					response, sizeof(response),
					MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flow_token = atol( response );

	switch( flow_token ) {
	case 0:
		rs232->flow_control = MXF_232_NO_FLOW_CONTROL;
		break;
	case 1:
		rs232->flow_control = MXF_232_SOFTWARE_FLOW_CONTROL;
		break;
	case 2:
		rs232->flow_control = MXF_232_HARDWARE_FLOW_CONTROL;
		break;
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unrecognized flow control value %ld returned for '%s'.",
			flow_token, rs232->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sim900_port_set_configuration( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_sim900_port_set_configuration()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_get_pointers( rs232,
					&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the baud rate. */

	snprintf( command, sizeof(command), "BAUD %c,%ld",
			sim900_port->port_name, rs232->speed );

	mx_status = mxi_sim900_command( sim900, command,
					NULL, 0, MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the word size. */

	if ( isalpha( (int) sim900_port->port_name ) ) {
		switch( rs232->word_size ) {
		case 5:
		case 6:
		case 7:
		case 8:
			snprintf( command, sizeof(command),
				"WORD %c,%ld",
				sim900_port->port_name,
				rs232->word_size );

			mx_status = mxi_sim900_command( sim900, command,
					NULL, 0, MXI_SIM900_PORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported word size %lu requested for SIM900 "
			"RS-232 port '%s'.",
				rs232->word_size, rs232->record->name );
			break;
		}
	}

	/* Set the parity. */

	snprintf( command, sizeof(command), "PARI %c,",
			sim900_port->port_name );

	switch( rs232->parity ) {
	case MXF_232_NO_PARITY:
		strlcat( command, "NONE", sizeof(command) );
		break;
	case MXF_232_EVEN_PARITY:
		strlcat( command, "EVEN", sizeof(command) );
		break;
	case MXF_232_ODD_PARITY:
		strlcat( command, "ODD", sizeof(command) );
		break;
	case MXF_232_MARK_PARITY:
		strlcat( command, "MARK", sizeof(command) );
		break;
	case MXF_232_SPACE_PARITY:
		strlcat( command, "SPACE", sizeof(command) );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported parity type '%c' requested for SIM900 "
		"RS-232 port '%s'.",
			rs232->parity, rs232->record->name );
		break;
	}

	mx_status = mxi_sim900_command( sim900, command,
				NULL, 0, MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the stop bits. */

	if ( isalpha( (int) sim900_port->port_name ) ) {
		switch( rs232->stop_bits ) {
		case 1:
		case 2:
			snprintf( command, sizeof(command),
				"SBIT %c,%ld",
				sim900_port->port_name,
				rs232->stop_bits );

			mx_status = mxi_sim900_command( sim900, command,
					NULL, 0, MXI_SIM900_PORT_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported stop bits %lu requested for SIM900 "
			"RS-232 port '%s'.",
				rs232->stop_bits, rs232->record->name );
			break;
		}
	}

	/* Set the flow control. */

	snprintf( command, sizeof(command), "FLOW %c,",
			sim900_port->port_name );

	switch( rs232->flow_control ) {
	case MXF_232_NO_FLOW_CONTROL:
		strlcat( command, "NONE", sizeof(command) );
		break;
	case MXF_232_SOFTWARE_FLOW_CONTROL:
		strlcat( command, "XON", sizeof(command) );
		break;
	case MXF_232_HARDWARE_FLOW_CONTROL:
		strlcat( command, "RTS", sizeof(command) );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported flow control '%c' requested for SIM900 "
		"RS-232 port '%s'.",
			rs232->flow_control, rs232->record->name );
		break;
	}

	mx_status = mxi_sim900_command( sim900, command,
				NULL, 0, MXI_SIM900_PORT_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_sim900_port_send_break( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_sim900_port_set_configuration()";

	MX_SIM900_PORT *sim900_port;
	MX_SIM900 *sim900;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxi_sim900_port_get_pointers( rs232,
						&sim900_port, &sim900, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: For now we disable the break */

	/* Send the SRST command. */

	snprintf( command, sizeof(command), "SRST %c",
					sim900_port->port_name );

	mx_status = mxi_sim900_command( sim900, command,
					NULL, 0, MXI_SIM900_PORT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a while for the break to complete. */

	mx_msleep(200);

	return MX_SUCCESSFUL_RESULT;
}

