/*
 * Name:    i_tcp232.c
 *
 * Purpose: MX driver for treating a TCP socket connection as if
 *          it were an RS-232 device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_TCP232_DEBUG	FALSE

#define MXI_TCP232_DEBUG_TIMING	FALSE

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
#include "i_tcp232.h"

#if MXI_TCP232_DEBUG_TIMING
#  include "mx_hrt.h"
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_tcp232_record_function_list = {
	NULL,
	mxi_tcp232_create_record_structures,
	mxi_tcp232_finish_record_initialization,
	NULL,
	NULL,
	mxi_tcp232_open,
	mxi_tcp232_close,
	NULL,
	mxi_tcp232_resynchronize
};

MX_RS232_FUNCTION_LIST mxi_tcp232_rs232_function_list = {
	mxi_tcp232_getchar,
	mxi_tcp232_putchar,
	mxi_tcp232_read,
	mxi_tcp232_write,
	mxi_tcp232_getline,
	mxi_tcp232_putline,
	mxi_tcp232_num_input_bytes_available,
	mxi_tcp232_discard_unread_input,
	mxi_tcp232_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_tcp232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_TCP232_STANDARD_FIELDS
};

long mxi_tcp232_num_record_fields
		= sizeof( mxi_tcp232_record_field_defaults )
			/ sizeof( mxi_tcp232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_tcp232_rfield_def_ptr
			= &mxi_tcp232_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_tcp232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_tcp232_create_record_structures()";

	MX_RS232 *rs232;
	MX_TCP232 *tcp232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	tcp232 = (MX_TCP232 *) malloc( sizeof(MX_TCP232) );

	if ( tcp232 == (MX_TCP232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TCP232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = tcp232;
	record->class_specific_function_list = &mxi_tcp232_rs232_function_list;

	rs232->record = record;
	tcp232->record = record;

	tcp232->resync_delay_milliseconds = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tcp232_finish_record_initialization( MX_RECORD *record )
{
	MX_TCP232 *tcp232;
	mx_status_type mx_status;

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the TCP232 device as being closed. */

	tcp232 = (MX_TCP232 *) record->record_type_struct;

	tcp232->socket = NULL;

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_tcp232_open_socket( MX_RS232 *rs232, MX_TCP232 *tcp232 )
{
	static const char fname[] = "mxi_tcp232_open_socket()";

	unsigned long socket_flags;
	mx_status_type mx_status;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RS232 pointer passed was NULL." );
	}

	if ( tcp232 == (MX_TCP232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_TCP232 structure pointer passed is NULL." );
	}

#if MXI_TCP232_DEBUG
	MX_DEBUG(-2, ("%s invoked for host '%s', port %d.",
		fname, tcp232->hostname, tcp232->port_number));
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

	if ( tcp232->tcp232_flags & MXF_TCP232_QUIET ) {
		socket_flags |= MXF_SOCKET_QUIET;
	}

	/* If the tcp232 device is currently open, close it. */

	if ( mx_socket_is_open( tcp232->socket ) ) {
		mx_status = mx_socket_close( tcp232->socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now open the tcp232 device. */

	mx_status = mx_tcp_socket_open_as_client( &(tcp232->socket),
				tcp232->hostname, tcp232->port_number,
				socket_flags, MX_SOCKET_DEFAULT_BUFFER_SIZE );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_error( mx_status.code, fname,
"Failed to open TCP connection for record '%s' to host '%s' at port '%ld'",
			tcp232->record->name,
			tcp232->hostname,
			tcp232->port_number );
	}
	return mx_status;
}

static mx_status_type
mxi_tcp232_close_socket( MX_RS232 *rs232, MX_TCP232 *tcp232 )
{
	static const char fname[] = "mxi_tcp232_close_socket()";

	mx_status_type mx_status;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RS232 pointer passed was NULL." );
	}

	if ( tcp232 == (MX_TCP232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_TCP232 pointer passed was NULL." );
	}

	/* If the tcp232 device is currently open, close it. */

	if ( mx_socket_is_open( tcp232->socket ) ) {
		mx_status = mx_socket_close( tcp232->socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tcp232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_tcp232_open()";

	MX_RS232 *rs232;
	MX_TCP232 *tcp232;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	tcp232 = (MX_TCP232 *) record->record_type_struct;

	flags = tcp232->tcp232_flags;

	/* If the socket is supposed to be open all the time, open it now. */

	if ( ( flags & MXF_TCP232_OPEN_DURING_TRANSACTION ) == 0 ) {
		mx_status = mxi_tcp232_open_socket( rs232, tcp232 );

		return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tcp232_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_tcp232_close()";

	MX_RS232 *rs232;
	MX_TCP232 *tcp232;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	tcp232 = (MX_TCP232 *) record->record_type_struct;

	mx_status = mxi_tcp232_close_socket( rs232, tcp232 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tcp232_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_tcp232_resynchronize()";

	MX_TCP232 *tcp232;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	tcp232 = (MX_TCP232 *) record->record_type_struct;

	if ( tcp232 == (MX_TCP232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TCP232 pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxi_tcp232_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( tcp232->resync_delay_milliseconds > 0 ) {
		mx_msleep( tcp232->resync_delay_milliseconds );
	}

	mx_status = mxi_tcp232_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tcp232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_tcp232_getchar()";

	MX_TCP232 *tcp232;
	struct timeval timeout;
	int num_fds, saved_errno;
	char *error_string;
	int num_chars, select_status;
	char c_temp;
	unsigned char c_mask;

#if HAVE_FD_SET
	fd_set read_mask;
#else
	long read_mask;
#endif

#if MXI_TCP232_DEBUG
	MX_DEBUG(-2, ("mxi_tcp232_getchar() invoked."));
#endif

	tcp232 = (MX_TCP232*) (rs232->record->record_type_struct);

	if ( mx_socket_is_open( tcp232->socket ) == FALSE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"TCP232 socket '%s' is not open.", rs232->record->name);
	}

	/* Check to see if any characters are available. */

#ifdef OS_WIN32
	num_fds = -1;		/* Win32 doesn't really use this argument. */
#else
	num_fds = 1 + tcp232->socket->socket_fd;
#endif

#if HAVE_FD_SET
	FD_ZERO( &read_mask );
	FD_SET( (tcp232->socket->socket_fd), &read_mask );
#else
	read_mask = 1 << (tcp232->socket);
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

	/* mxi_tcp232_getchar() is often used to test whether there is
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

	num_chars = recv( tcp232->socket->socket_fd, &c_temp, 1, 0 );

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

#if MXI_TCP232_DEBUG
	MX_DEBUG(-2, ("mxi_tcp232_getchar(): num_chars = %d, c = 0x%x, '%c'",
		num_chars, *c, *c));
#endif

	if ( num_chars != 1 ) {
		return mx_error( (MXE_NOT_READY | MXE_QUIET), fname,
			"Failed to read a character from port '%s'.",
			rs232->record->name );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxi_tcp232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_tcp232_putchar()";

	MX_TCP232 *tcp232;
	unsigned long flags;
	mx_status_type mx_status;

#if MXI_TCP232_DEBUG
	MX_DEBUG(-2, ("mxi_tcp232_putchar() invoked.  c = 0x%x, '%c'", c, c));
#endif

	tcp232 = (MX_TCP232*) rs232->record->record_type_struct;

	flags = tcp232->tcp232_flags;

	/* If needed, open the connection to the remote host. */

	if ( flags & MXF_TCP232_OPEN_DURING_TRANSACTION ) {
		mx_status = mxi_tcp232_open_socket( rs232, tcp232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send the character. */

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {

		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Non-blocking TCP232 I/O not yet implemented.");
	} else {
		mx_status = mx_socket_send( tcp232->socket, &c, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tcp232_read( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	MX_TCP232 *tcp232;
	mx_status_type mx_status;

	tcp232 = (MX_TCP232*) rs232->record->record_type_struct;

	mx_status = mx_socket_receive( tcp232->socket,
					buffer, max_bytes_to_read,
					NULL, NULL, 0 );

	if ( bytes_read != NULL ) {
		*bytes_read = max_bytes_to_read;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tcp232_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	MX_TCP232 *tcp232;
	unsigned long flags;
	mx_status_type mx_status;

	tcp232 = (MX_TCP232*) rs232->record->record_type_struct;

	flags = tcp232->tcp232_flags;

	/* If needed, open the connection to the remote host. */

	if ( flags & MXF_TCP232_OPEN_DURING_TRANSACTION ) {
		mx_status = mxi_tcp232_open_socket( rs232, tcp232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send the data. */

	mx_status = mx_socket_send( tcp232->socket,
					buffer, max_bytes_to_write );

	if ( bytes_written != NULL ) {
		*bytes_written = max_bytes_to_write;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tcp232_getline( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
#if MXI_TCP232_DEBUG
	static const char fname[] = "mxi_tcp232_getline()";
#endif

	MX_TCP232 *tcp232;
	mx_status_type mx_status;

	tcp232 = (MX_TCP232*) rs232->record->record_type_struct;

	mx_status = mx_socket_getline( tcp232->socket,
					buffer, max_bytes_to_read,
					rs232->read_terminator_array );

	if ( bytes_read != NULL ) {
		*bytes_read = strlen( buffer );
	}

#if MXI_TCP232_DEBUG
	MX_DEBUG(-2,("%s buffer read = '%s'", fname, buffer));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tcp232_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
#if MXI_TCP232_DEBUG
	static const char fname[] = "mxi_tcp232_putline()";
#endif

	MX_TCP232 *tcp232;
	unsigned long flags;
	mx_status_type mx_status;

#if MXI_TCP232_DEBUG
	MX_DEBUG(-2, ("%s invoked, buffer = '%s'", fname, buffer));
#endif

	tcp232 = (MX_TCP232*) rs232->record->record_type_struct;

	flags = tcp232->tcp232_flags;

	/* If needed, open the connection to the remote host. */

	if ( flags & MXF_TCP232_OPEN_DURING_TRANSACTION ) {
		mx_status = mxi_tcp232_open_socket( rs232, tcp232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MXI_TCP232_DEBUG
	MX_DEBUG(-2,("%s: transmitting the buffer '%s'.", fname, buffer));
#endif

	/* Send the line. */

	mx_status = mx_socket_putline( tcp232->socket, buffer,
					rs232->write_terminator_array );

	if ( bytes_written != NULL ) {
		*bytes_written = strlen(buffer)
				+ strlen( rs232->write_terminator_array );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tcp232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tcp232_num_input_bytes_available()";

	MX_TCP232 *tcp232;
	int num_fds, select_status, saved_errno;
	struct timeval timeout;
	char *error_string;

#if HAVE_FD_SET
	fd_set read_mask;
#else
	long read_mask;
#endif

	tcp232 = (MX_TCP232 *) (rs232->record->record_type_struct);

	if ( tcp232 == (MX_TCP232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TCP232 structure for TCP232 port '%s' is NULL.",
			rs232->record->name);
	}

	if ( mx_socket_is_open( tcp232->socket ) == FALSE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"TCP232 socket '%s' is not open.", rs232->record->name);
	}
#ifdef OS_WIN32
	num_fds = -1;  /* Win32 doesn't really use this argument. */
#else
	num_fds = 1 + tcp232->socket->socket_fd;
#endif

#if HAVE_FD_SET
	FD_ZERO( &read_mask );
	FD_SET( (tcp232->socket->socket_fd), &read_mask );
#else
	read_mask = 1 << (tcp232->socket);
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

#if MXI_TCP232_DEBUG
	MX_DEBUG(-2,("%s: num_input_bytes_available = %ld",
			fname, rs232->num_input_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tcp232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tcp232_discard_unread_input()";

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

		mx_status = mxi_tcp232_num_input_bytes_available( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( rs232->num_input_bytes_available == 0 ) {

			break;	/* Here we exit the for() loop. */

		} else {
			mx_status = mxi_tcp232_getchar( rs232, &c );

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
mxi_tcp232_discard_unwritten_output( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_tcp232_discard_unwritten_output()";

	/* This is not generally possible for a TCP232 device. */

	return mx_error( (MXE_UNSUPPORTED | MXE_QUIET), fname,
		"This function is not supported for a TCP232 device." );
}

#endif /* HAVE_TCPIP */
