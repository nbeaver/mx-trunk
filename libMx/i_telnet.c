/*
 * Name:    i_telnet.c
 *
 * Purpose: MX driver for treating a connection to a Telnet server as if
 *          it were an RS-232 device.  This requires properly interpreting
 *          Telnet command sequences, escaping IAC and sometimes CR bytes
 *          and so forth.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_TELNET_DEBUG	FALSE

#define MXI_TELNET_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_osdef.h"

#if HAVE_TCPIP

#include <ctype.h>
#include <sys/types.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_socket.h"
#include "mx_select.h"
#include "i_telnet.h"

#if MXI_TELNET_DEBUG_TIMING
#  include "mx_hrt.h"
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_telnet_record_function_list = {
	NULL,
	mxi_telnet_create_record_structures,
	mxi_telnet_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_telnet_open,
	mxi_telnet_close
};

MX_RS232_FUNCTION_LIST mxi_telnet_rs232_function_list = {
	mxi_telnet_getchar,
	mxi_telnet_putchar,
	NULL,
	mxi_telnet_write,
	NULL,
	mxi_telnet_putline,
	mxi_telnet_num_input_bytes_available,
	mxi_telnet_discard_unread_input,
	mxi_telnet_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_telnet_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_TELNET_STANDARD_FIELDS
};

long mxi_telnet_num_record_fields
		= sizeof( mxi_telnet_record_field_defaults )
			/ sizeof( mxi_telnet_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_telnet_rfield_def_ptr
			= &mxi_telnet_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_telnet_get_pointers( MX_RS232 *rs232,
			MX_TELNET **telnet,
			const char *calling_fname )
{
	static const char fname[] = "mxi_telnet_get_pointers()";

	MX_RECORD *telnet_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( telnet == (MX_TELNET **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_TELNET pointer passed by '%s' was NULL.",
			calling_fname );
	}

	telnet_record = rs232->record;

	if ( telnet_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The telnet_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*telnet = (MX_TELNET *) telnet_record->record_type_struct;

	if ( *telnet == (MX_TELNET *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TELNET pointer for record '%s' is NULL.",
			telnet_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ---- */

MX_EXPORT mx_status_type
mxi_telnet_handle_commands( MX_RS232 *rs232,
				unsigned long delay_msec,
				unsigned long max_retries )
{
#if MXI_TELNET_DEBUG
	static const char fname[] = "mxi_telnet_handle_commands()";
#endif

	unsigned long i;
	mx_status_type mx_status;

	for ( i = 0; i < max_retries; i++ ) {

#if MXI_TELNET_DEBUG
		MX_DEBUG(-2,("%s: i = %lu", fname, i));
#endif
		mx_msleep( delay_msec );

		mx_status = mxi_telnet_num_input_bytes_available( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( rs232->num_input_bytes_available == 0 )
			break;

		mx_status = mxi_telnet_discard_unread_input( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXI_TELNET_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif
	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxi_telnet_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_telnet_create_record_structures()";

	MX_RS232 *rs232;
	MX_TELNET *telnet;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	telnet = (MX_TELNET *) malloc( sizeof(MX_TELNET) );

	if ( telnet == (MX_TELNET *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TELNET structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = telnet;
	record->class_specific_function_list = &mxi_telnet_rs232_function_list;

	rs232->record = record;
	telnet->record = record;

	telnet->saved_byte = 0;
	telnet->saved_byte_is_available = FALSE;

	/* Initialize some local copies of Telnet options. */

	telnet->client_sends_binary = FALSE;
	telnet->server_sends_binary = FALSE;

	telnet->server_echo_on = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_telnet_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_telnet_finish_record_initialization()";

	MX_RS232 *rs232;
	MX_TELNET *telnet;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the Telnet device as being closed. */

	telnet->socket = NULL;

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_telnet_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_telnet_open()";

	MX_RS232 *rs232;
	MX_TELNET *telnet;
	unsigned long socket_flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_TELNET_DEBUG
	MX_DEBUG(-2, ("%s invoked for host '%s', port %ld.",
		fname, telnet->hostname, telnet->port_number));
#endif

	/* Select the flags for the socket we are about to open. */

	socket_flags = 0;

	/* Are we going to use unbuffered I/O? */

	if ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) {
		socket_flags |= MXF_SOCKET_DISABLE_NAGLE_ALGORITHM;
	}

	/* Are we going to suppress error messages for the case that
	 * the remote server closes the connection before we do?
	 */

	if ( telnet->telnet_flags & MXF_TELNET_QUIET ) {
		socket_flags |= MXF_SOCKET_QUIET;
	}

	/* If the Telnet socket is currently open, close it. */

	if ( mx_socket_is_open( telnet->socket ) ) {
		mx_status = mx_socket_close( telnet->socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now open the Telnet socket. */

	mx_status = mx_tcp_socket_open_as_client( &(telnet->socket),
				telnet->hostname, telnet->port_number,
				socket_flags, MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_error( mx_status.code, fname,
"Failed to open TCP connection for record '%s' to host '%s' at port '%ld'",
			telnet->record->name,
			telnet->hostname,
			telnet->port_number );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_telnet_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_telnet_close()";

	MX_RS232 *rs232;
	MX_TELNET *telnet;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the Telnet socket is currently open, close it. */

	if ( mx_socket_is_open( telnet->socket ) ) {
		mx_status = mx_socket_close( telnet->socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return mx_status;
}

/* mxi_telnet_raw_getchar() does the raw reading of individual bytes from
 * the socket and does not know anything about Telnet commands.
 */

static mx_status_type
mxi_telnet_raw_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_telnet_raw_getchar()";

	MX_TELNET *telnet;
	struct timeval timeout;
	int num_fds, saved_errno;
	char *error_string;
	int num_chars, select_status;
	char c_temp;
	unsigned char c_mask;
	mx_status_type mx_status;

#if HAVE_FD_SET
	fd_set read_mask;
#else
	long read_mask;
#endif

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	telnet = (MX_TELNET*) (rs232->record->record_type_struct);

	if ( mx_socket_is_open( telnet->socket ) == FALSE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"TELNET socket '%s' is not open.", rs232->record->name);
	}

	/* Check to see if any characters are available. */

#ifdef OS_WIN32
	num_fds = -1;		/* Win32 doesn't really use this argument. */
#else
	num_fds = 1 + telnet->socket->socket_fd;
#endif

#if HAVE_FD_SET
	FD_ZERO( &read_mask );
	FD_SET( (telnet->socket->socket_fd), &read_mask );
#else
	read_mask = 1 << (telnet->socket);
#endif
	/* Set the timeout for waiting for input appropriately. 
	 * For MXF_232_WAIT, time out after 5 seconds.
	 */

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
	} else {
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
	}

	/* Do the test. */

	select_status = select( num_fds, &read_mask, NULL, NULL, &timeout );

	saved_errno = mx_socket_check_error_status(
		&select_status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Error while waiting for input to arrive for '%s'.  "
			"Errno = %d.  Error string = '%s'.",
			rs232->record->name, saved_errno, error_string );
	}

	/* mxi_telnet_getchar() is often used to test whether there is
	 * input ready on the TTY port.  Normally, it is not desirable
	 * to broadcast a message to the world when this fails, so we
	 * add the MXE_QUIET flag to the error code.
	 */

	if ( select_status == 0 ) {	/* No input available. */
		return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
			"Failed to read a character from port '%s'.",
			rs232->record->name );
	}

	/* Now try to read the character. */

	num_chars = recv( telnet->socket->socket_fd, &c_temp, 1, 0 );

	saved_errno = mx_socket_check_error_status(
			&num_chars, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		*c = '\0';
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Error while reading from '%s'.  "
			"Errno = %d.  Error string = '%s'.",
			rs232->record->name, saved_errno, error_string );
	}

	/* === Mask the character to the appropriate number of bits. === */

	/* Note that the following code _assumes_ that chars are 8 bits. */

	c_mask = 0xff >> ( 8 - rs232->word_size );

	c_temp &= c_mask;

	*c = (char) c_temp;

#if MXI_TELNET_DEBUG
	MX_DEBUG(-2, ("%s: num_chars = %d, c = 0x%x, '%c'",
		fname, num_chars, (*c) & 0xff, *c));
#endif

	if ( num_chars != 1 ) {
		return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
			"Failed to read a character from port '%s'.",
			rs232->record->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

/* mxi_telnet_getchar() examines raw bytes returned by mxi_telnet_raw_getchar()
 * looking for Telnet commands and handles the response to them.
 */

MX_EXPORT mx_status_type
mxi_telnet_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_telnet_getchar()";

	MX_TELNET *telnet;
	unsigned char local_byte, command_byte, option_byte;
	unsigned char option_response[3];
	mx_bool_type send_option_response;
	mx_status_type mx_status;

#if MXI_TELNET_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The character pointer passed was NULL." );
	}

	/* If we have a saved character from a previous call, then
	 * return that character.
	 */

	if ( telnet->saved_byte_is_available ) {
		*c = telnet->saved_byte;

		telnet->saved_byte_is_available = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If no saved byte was available, then we must get a new one
	 * from the Telnet socket.
	 */

	mx_status = mxi_telnet_raw_getchar( rs232, (char *) &local_byte );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the character returned is _not_ the IAC character 0xff, then
	 * just return the character as is.
	 */

	if ( local_byte != MXF_TELNET_IAC ) {
		*c = local_byte;

		return MX_SUCCESSFUL_RESULT;
	}

	/* We must now get another character from the socket so that we can
	 * figure out which telnet command this is.
	 */

	mx_status = mxi_telnet_raw_getchar( rs232, (char *) &command_byte );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we get two IACs in a row, then we return one of them to
	 * the caller.
	 */

	if ( command_byte == MXF_TELNET_IAC ) {
		*c = command_byte;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If the second character is not a valid Telnet command, then
	 * we abort noisily with an error.
	 */

	if ( command_byte < MXF_TELNET_SE ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The Telnet command prefix %#x received from Telnet "
		"record '%s' was followed by an "
		"invalid Telnet command value %#x.  We do not know how "
		"to recover from this situation, so we abort.",
			MXF_TELNET_IAC, rs232->record->name, command_byte );
			
	}

	/* Handle the various Telnet commands. */

	switch( command_byte ) {
	case MXF_TELNET_SE:
	case MXF_TELNET_NOP:
	case MXF_TELNET_DATA_MARK:
	case MXF_TELNET_BREAK:
	case MXF_TELNET_INTERRUPT_PROCESS:
	case MXF_TELNET_ABORT_OUTPUT:
	case MXF_TELNET_ARE_YOU_THERE:
	case MXF_TELNET_ERASE_CHARACTER:
	case MXF_TELNET_ERASE_LINE:
	case MXF_TELNET_GO_AHEAD:
	case MXF_TELNET_SB:
		mx_warning( "Support for Telnet command %#x used by "
			"record '%s' is not yet implemented.",
			command_byte, rs232->record->name );
		break;

	case MXF_TELNET_WILL:
	case MXF_TELNET_WONT:
	case MXF_TELNET_DO:
	case MXF_TELNET_DONT:
		/* Telnet option negotiation commands are three byte commands
		 * where the third byte is the option number.  We must now
		 * get the option number byte so that we can decide what
		 * to do.
		 */

		mx_status = mxi_telnet_raw_getchar( rs232,
						(char *) &option_byte );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		option_response[0] = MXF_TELNET_IAC;
		option_response[2] = option_byte;
		send_option_response = FALSE;

		if ( command_byte == MXF_TELNET_WILL ) {
			switch( option_byte ) {
#if 0
			case MXO_TELNET_TRANSMIT_BINARY:
				send_option_response = FALSE;
				break;
			case MXO_TELNET_ECHO:
				send_option_response = FALSE;
				break;
			case MXO_TELNET_SUPPRESS_GO_AHEAD:
				send_option_response = FALSE;
				break;
#endif
			default:
				send_option_response = TRUE;
				option_response[1] = MXF_TELNET_DONT;
				break;
			}
		} else
		if ( command_byte == MXF_TELNET_WONT ) {
			/* Currently we ignore all WONT commands. */
		} else
		if ( command_byte == MXF_TELNET_DO ) {
			/* Currently we reject all DO commands. */

			send_option_response = TRUE;

			option_response[1] = MXF_TELNET_WONT;
		} else
		if ( command_byte == MXF_TELNET_DONT ) {
			/* Currently we ignore all DONT commands. */
		}

		if ( send_option_response ) {
			mx_status = mxi_telnet_write( rs232,
						(char *) option_response,
						sizeof(option_response),
						NULL );
		}
		break;
	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"Somehow the command byte value %#x got to the switch() "
		"statement for Telnet record '%s'.  "
		"This should _never_ happen.",
			command_byte, rs232->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_telnet_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_telnet_putchar()";

	MX_TELNET *telnet;
	unsigned long flags;
	mx_status_type mx_status;

#if MXI_TELNET_DEBUG
	MX_DEBUG(-2, ("%s invoked.  c = 0x%x, '%c'", fname, c, c));
#endif

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = telnet->telnet_flags;

	/* Send the character. */

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {

		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Non-blocking TELNET I/O not yet implemented.");
	} else {
		mx_status = mx_socket_send( telnet->socket, &c, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_telnet_read() does not exist, since we have to examine the raw
 * byte stream in mxi_telnet_getchar() for Telnet commands.
 */

MX_EXPORT mx_status_type
mxi_telnet_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_telnet_write()";

	MX_TELNET *telnet;
	mx_status_type mx_status;

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_TELNET_DEBUG
	{
		int i;

		for ( i = 0; i < max_bytes_to_write; i++ ) {
			MX_DEBUG(-2,("%s:     buffer[%d] = %#x",
				fname, i, buffer[i] & 0xff ));
		}
	}
#endif

	/* Send the data. */

	mx_status = mx_socket_send( telnet->socket,
					buffer, max_bytes_to_write );

	if ( bytes_written != NULL ) {
		*bytes_written = max_bytes_to_write;
	}

	return mx_status;
}

/* mxi_telnet_getline() does not exist, since we have to examine the raw
 * byte stream in mxi_telnet_getchar() for Telnet commands.
 */

MX_EXPORT mx_status_type
mxi_telnet_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_telnet_putline()";

	MX_TELNET *telnet;
	mx_status_type mx_status;

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_TELNET_DEBUG
	MX_DEBUG(-2,("%s: sending the string '%s'.", fname, buffer));
#endif

	/* Send the line. */

	mx_status = mx_socket_putline( telnet->socket, buffer,
					rs232->write_terminator_array );

	if ( bytes_written != NULL ) {
		*bytes_written = strlen(buffer)
				+ strlen( rs232->write_terminator_array );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_telnet_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_telnet_num_input_bytes_available()";

	MX_TELNET *telnet;
	int num_fds, select_status, saved_errno;
	struct timeval timeout;
	char *error_string;
	mx_status_type mx_status;

#if HAVE_FD_SET
	fd_set read_mask;
#else
	long read_mask;
#endif

	mx_status = mxi_telnet_get_pointers( rs232, &telnet, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_socket_is_open( telnet->socket ) == FALSE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"TELNET socket '%s' is not open.", rs232->record->name);
	}
#ifdef OS_WIN32
	num_fds = -1;  /* Win32 doesn't really use this argument. */
#else
	num_fds = 1 + telnet->socket->socket_fd;
#endif

#if HAVE_FD_SET
	FD_ZERO( &read_mask );
	FD_SET( (telnet->socket->socket_fd), &read_mask );
#else
	read_mask = 1 << (telnet->socket);
#endif
	/* Set timeout to zero. */

	timeout.tv_usec = 0;
	timeout.tv_sec = 0;
	
	/* Do the test. */

	select_status = select( num_fds, &read_mask, NULL, NULL, &timeout );

	saved_errno = mx_socket_check_error_status(
		&select_status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Error while testing input ready status of '%s'.  "
			"Errno = %d.  Error string = '%s'.",
			rs232->record->name, saved_errno, error_string );
	}

	if ( select_status ) {
		rs232->num_input_bytes_available = 1;
	} else {
		rs232->num_input_bytes_available = 0;
	}

#if MXI_TELNET_DEBUG
	MX_DEBUG(-2,("%s: num_input_bytes_available = %ld",
			fname, rs232->num_input_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_telnet_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_telnet_discard_unread_input()";

	unsigned long i, timeout, debug_flag;
	char c;
	mx_status_type mx_status;

	char buffer[2500];
	unsigned long j, k;
	unsigned long buffer_length = sizeof(buffer) / sizeof(buffer[0]);

	timeout = 10000L;

	/* If input is available, read until there is no more input.
	 * If we do this to a device that is constantly generating
	 * output, then we will never get to the end.  Thus, we have
	 * built a timeout into the function.
	 */

	rs232->transfer_flags &= ( ~ MXF_232_NOWAIT );

	debug_flag = rs232->transfer_flags & MXF_232_DEBUG;

	for ( i=0; i < timeout; i++ ) {

		mx_msleep(5);

		mx_status = mxi_telnet_num_input_bytes_available( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( rs232->num_input_bytes_available == 0 ) {

			break;	/* Here we exit the for() loop. */

		} else {
			mx_status = mxi_telnet_getchar( rs232, &c );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( debug_flag ) {
				if ( i < buffer_length ) {
					buffer[i] = c;
				}
			}
		}
	}

	if ( debug_flag ) {
		if ( i > buffer_length )
			k = buffer_length;
		else
			k = i;

		if ( i > 0 ) {
			fprintf(stderr,
				"%s: First %lu characters discarded were:\n\n",
				fname,k);

			for ( j = 0; j < k; j++ ) {
				c = buffer[j];

				if ( isprint( (int) c ) ) {
					fputc( c, stderr );
				} else {
					fprintf( stderr, "(0x%x)", c & 0xff );
				}
			}
			fprintf(stderr,"\n\n");
		}
	}

	if ( i >= timeout ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"As many as %lu characters were read while trying to empty the input buffer.  "
"Perhaps this device continuously generates output?", timeout );

	} else if ( i > 0 ) {

		MX_DEBUG( 2,("%s: %lu characters discarded.", fname, i));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_telnet_discard_unwritten_output( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_telnet_discard_unwritten_output()";

	/* This is not generally possible for a TELNET device. */

	return mx_error( (MXE_UNSUPPORTED | MXE_QUIET), fname,
		"This function is not supported for a TELNET device." );
}

#endif /* HAVE_TCPIP */
