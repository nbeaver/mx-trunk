/*
 * Name:    i_usbtmc.c
 *
 * Purpose: MX driver for treating a TCP socket connection as if
 *          it were an RS-232 device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2007, 2010, 2013, 2015-2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_USBTMC_DEBUG			FALSE

#define MXI_USBTMC_DEBUG_TIMING			FALSE

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
#include "i_usbtmc.h"

#if MXI_USBTMC_DEBUG_TIMING
#  include "mx_hrt.h"
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_usbtmc_record_function_list = {
	NULL,
	mxi_usbtmc_create_record_structures,
	mxi_usbtmc_finish_record_initialization,
	NULL,
	NULL,
	mxi_usbtmc_open,
	mxi_usbtmc_close,
	NULL,
	mxi_usbtmc_resynchronize
};

MX_RS232_FUNCTION_LIST mxi_usbtmc_rs232_function_list = {
	mxi_usbtmc_getchar,
	mxi_usbtmc_putchar,
	mxi_usbtmc_read,
	mxi_usbtmc_write,
	mxi_usbtmc_getline,
	mxi_usbtmc_putline,
	mxi_usbtmc_num_input_bytes_available,
	mxi_usbtmc_discard_unread_input,
	mxi_usbtmc_discard_unwritten_output,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_usbtmc_wait_for_input_available,
	NULL,
	NULL,
	mxi_usbtmc_flush
};

MX_RECORD_FIELD_DEFAULTS mxi_usbtmc_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_USBTMC_STANDARD_FIELDS
};

long mxi_usbtmc_num_record_fields
		= sizeof( mxi_usbtmc_record_field_defaults )
			/ sizeof( mxi_usbtmc_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_usbtmc_rfield_def_ptr
			= &mxi_usbtmc_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_usbtmc_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_usbtmc_create_record_structures()";

	MX_RS232 *rs232;
	MX_USBTMC *usbtmc;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	usbtmc = (MX_USBTMC *) malloc( sizeof(MX_USBTMC) );

	if ( usbtmc == (MX_USBTMC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_USBTMC structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = usbtmc;
	record->class_specific_function_list = &mxi_usbtmc_rs232_function_list;

	rs232->record = record;
	usbtmc->record = record;

	usbtmc->receive_buffer_size = -1;
	usbtmc->resync_delay_milliseconds = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_usbtmc_finish_record_initialization( MX_RECORD *record )
{
	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the USBTMC device as being closed. */

	usbtmc = (MX_USBTMC *) record->record_type_struct;

	usbtmc->socket = NULL;

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_usbtmc_open_socket( MX_RS232 *rs232, MX_USBTMC *usbtmc )
{
	static const char fname[] = "mxi_usbtmc_open_socket()";

	unsigned long socket_flags;
	mx_status_type mx_status;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RS232 pointer passed was NULL." );
	}

	if ( usbtmc == (MX_USBTMC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_USBTMC structure pointer passed is NULL." );
	}

#if MXI_USBTMC_DEBUG
	MX_DEBUG(-2, ("%s invoked for host '%s', port %ld.",
		fname, usbtmc->hostname, usbtmc->port_number));
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

	if ( usbtmc->usbtmc_flags & MXF_USBTMC_QUIET ) {
		socket_flags |= MXF_SOCKET_QUIET;
	}

	/* If the usbtmc device is currently open, close it. */

	if ( mx_socket_is_open( usbtmc->socket ) ) {
		mx_status = mx_socket_close( usbtmc->socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( usbtmc->usbtmc_flags & MXF_USBTMC_USE_MX_RECEIVE_BUFFER ) {
		socket_flags |= MXF_SOCKET_USE_MX_RECEIVE_BUFFER;
		usbtmc->receive_buffer_size = 2500;
	} else
	if ( usbtmc->receive_buffer_size >= 0 ) {
		socket_flags |= MXF_SOCKET_USE_MX_RECEIVE_BUFFER;
	} else {
		usbtmc->receive_buffer_size = MX_SOCKET_DEFAULT_BUFFER_SIZE;
	}

	/* Now open the usbtmc device. */

	mx_status = mx_tcp_socket_open_as_client( &(usbtmc->socket),
				usbtmc->hostname, usbtmc->port_number,
				socket_flags, usbtmc->receive_buffer_size );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_error( mx_status.code, fname,
"Failed to open TCP connection for record '%s' to host '%s' at port '%ld'",
			usbtmc->record->name,
			usbtmc->hostname,
			usbtmc->port_number );
	}
	return mx_status;
}

static mx_status_type
mxi_usbtmc_close_socket( MX_RS232 *rs232, MX_USBTMC *usbtmc )
{
	static const char fname[] = "mxi_usbtmc_close_socket()";

	mx_status_type mx_status;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RS232 pointer passed was NULL." );
	}

	if ( usbtmc == (MX_USBTMC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_USBTMC pointer passed was NULL." );
	}

	/* If the usbtmc device is currently open, close it. */

	if ( mx_socket_is_open( usbtmc->socket ) ) {
		mx_status = mx_socket_close( usbtmc->socket );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_usbtmc_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_usbtmc_open()";

	MX_RS232 *rs232;
	MX_USBTMC *usbtmc;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	usbtmc = (MX_USBTMC *) record->record_type_struct;

	flags = usbtmc->usbtmc_flags;

	/* If the socket is supposed to be open all the time, open it now. */

	if ( ( flags & MXF_USBTMC_OPEN_DURING_TRANSACTION ) == 0 ) {
		mx_status = mxi_usbtmc_open_socket( rs232, usbtmc );

		return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_usbtmc_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_usbtmc_close()";

	MX_RS232 *rs232;
	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	usbtmc = (MX_USBTMC *) record->record_type_struct;

	mx_status = mxi_usbtmc_close_socket( rs232, usbtmc );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_usbtmc_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_usbtmc_resynchronize()";

	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	usbtmc = (MX_USBTMC *) record->record_type_struct;

	if ( usbtmc == (MX_USBTMC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_USBTMC pointer for record '%s' is NULL.",
			record->name );
	}

	if ( usbtmc->usbtmc_flags & MXF_USBTMC_USE_MX_SOCKET_RESYNCHRONIZE ) {
		mx_status = mx_socket_resynchronize( &(usbtmc->socket) );
	} else {
		mx_status = mxi_usbtmc_close( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( usbtmc->resync_delay_milliseconds > 0 ) {
			mx_msleep( usbtmc->resync_delay_milliseconds );
		}

		mx_status = mxi_usbtmc_open( record );
	}

	return mx_status;
}

/*----*/

static mx_status_type
mxi_usbtmc_open_socket_if_necessary( MX_RS232 *rs232, MX_USBTMC *usbtmc )
{
	mx_bool_type open_socket;
	unsigned long flags;
	mx_status_type mx_status;

	flags = usbtmc->usbtmc_flags;

	open_socket = FALSE;

	if ( flags & MXF_USBTMC_OPEN_DURING_TRANSACTION ) {
		open_socket = TRUE;
	} else
	if ( flags & MXF_USBTMC_AUTOMATIC_REOPEN ) {
		if ( mx_socket_is_open( usbtmc->socket ) == FALSE ) {
			open_socket = TRUE;
		}
	}

#if MXI_USBTMC_DEBUG
	MX_DEBUG(-2,("open_socket = %d", (int) open_socket));
#endif

	if ( open_socket ) {
		mx_status = mxi_usbtmc_open_socket( rs232, usbtmc );
	} else {
		mx_status = MX_SUCCESSFUL_RESULT;
	}

	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxi_usbtmc_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_usbtmc_getchar()";

	MX_USBTMC *usbtmc;
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

#if MXI_USBTMC_DEBUG
	MX_DEBUG(-2, ("mxi_usbtmc_getchar() invoked."));
#endif

	usbtmc = (MX_USBTMC*) (rs232->record->record_type_struct);

	mx_status = mxi_usbtmc_open_socket_if_necessary( rs232, usbtmc );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if any characters are available. */

#ifdef OS_WIN32
	num_fds = -1;		/* Win32 doesn't really use this argument. */
#else
	num_fds = 1 + usbtmc->socket->socket_fd;
#endif

#if HAVE_FD_SET
	FD_ZERO( &read_mask );
	FD_SET( (usbtmc->socket->socket_fd), &read_mask );
#else
	read_mask = 1 << (usbtmc->socket);
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

	/* mxi_usbtmc_getchar() is often used to test whether there is
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

	num_chars = recv( usbtmc->socket->socket_fd, &c_temp, 1, 0 );

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

#if MXI_USBTMC_DEBUG
	MX_DEBUG(-2, ("mxi_usbtmc_getchar(): num_chars = %d, c = 0x%x, '%c'",
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
mxi_usbtmc_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_usbtmc_putchar()";

	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

#if MXI_USBTMC_DEBUG
	MX_DEBUG(-2, ("mxi_usbtmc_putchar() invoked.  c = 0x%x, '%c'", c, c));
#endif

	usbtmc = (MX_USBTMC*) rs232->record->record_type_struct;

	/* If needed, open the connection to the remote host. */

	mx_status = mxi_usbtmc_open_socket_if_necessary( rs232, usbtmc );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the character. */

	if ( rs232->transfer_flags & MXF_232_NOWAIT ) {

		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Non-blocking USBTMC I/O not yet implemented.");
	} else {
		mx_status = mx_socket_send( usbtmc->socket, &c, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_usbtmc_read( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

	usbtmc = (MX_USBTMC*) rs232->record->record_type_struct;

	mx_status = mx_socket_receive( usbtmc->socket,
					buffer, max_bytes_to_read,
					NULL, NULL, 0, 0 );

	if ( bytes_read != NULL ) {
		*bytes_read = max_bytes_to_read;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_usbtmc_write( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_write,
			size_t *bytes_written )
{
	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

	usbtmc = (MX_USBTMC*) rs232->record->record_type_struct;

	/* If needed, open the connection to the remote host. */

	mx_status = mxi_usbtmc_open_socket_if_necessary( rs232, usbtmc );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the data. */

	mx_status = mx_socket_send( usbtmc->socket,
					buffer, max_bytes_to_write );

	if ( bytes_written != NULL ) {
		*bytes_written = max_bytes_to_write;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_usbtmc_getline( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
#if MXI_USBTMC_DEBUG
	static const char fname[] = "mxi_usbtmc_getline()";
#endif

	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

	usbtmc = (MX_USBTMC*) rs232->record->record_type_struct;

	mx_status = mx_socket_getline( usbtmc->socket,
					buffer, max_bytes_to_read,
					rs232->read_terminator_array );

	if ( bytes_read != NULL ) {
		*bytes_read = strlen( buffer );
	}

#if MXI_USBTMC_DEBUG
	MX_DEBUG(-2,("%s buffer read = '%s'", fname, buffer));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_usbtmc_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
#if MXI_USBTMC_DEBUG
	static const char fname[] = "mxi_usbtmc_putline()";
#endif

	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

#if MXI_USBTMC_DEBUG
	MX_DEBUG(-2, ("%s invoked, buffer = '%s'", fname, buffer));
#endif

	usbtmc = (MX_USBTMC*) rs232->record->record_type_struct;

	/* If needed, open the connection to the remote host. */

	mx_status = mxi_usbtmc_open_socket_if_necessary( rs232, usbtmc );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_USBTMC_DEBUG
	MX_DEBUG(-2,("%s: transmitting the buffer '%s'.", fname, buffer));
#endif

	/* Send the line. */

	mx_status = mx_socket_putline( usbtmc->socket, buffer,
					rs232->write_terminator_array );

	if ( bytes_written != NULL ) {
		*bytes_written = strlen(buffer)
				+ strlen( rs232->write_terminator_array );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_usbtmc_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_usbtmc_num_input_bytes_available()";

	MX_USBTMC *usbtmc;
	long num_socket_bytes_available;
	mx_status_type mx_status;

	usbtmc = (MX_USBTMC *) (rs232->record->record_type_struct);

	if ( usbtmc == (MX_USBTMC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_USBTMC structure for USBTMC port '%s' is NULL.",
			rs232->record->name);
	}

	mx_status = mxi_usbtmc_open_socket_if_necessary( rs232, usbtmc );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_num_input_bytes_available( usbtmc->socket,
						&num_socket_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->num_input_bytes_available = num_socket_bytes_available;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_usbtmc_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_usbtmc_discard_unread_input()";

	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

	usbtmc = (MX_USBTMC *) (rs232->record->record_type_struct);

	if ( usbtmc == (MX_USBTMC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_USBTMC structure for USBTMC port '%s' is NULL.",
			rs232->record->name);
	}

	mx_status = mxi_usbtmc_open_socket_if_necessary( rs232, usbtmc );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_discard_unread_input( usbtmc->socket );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_usbtmc_discard_unwritten_output( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_usbtmc_discard_unwritten_output()";

	/* This is not generally possible for a USBTMC device. */

	return mx_error( (MXE_UNSUPPORTED | MXE_QUIET), fname,
		"This function is not supported for a USBTMC device." );
}

MX_EXPORT mx_status_type
mxi_usbtmc_wait_for_input_available( MX_RS232 *rs232,
				double wait_time_in_seconds )
{
	static const char fname[] = "mxi_usbtmc_wait_for_input_available()";

	MX_USBTMC *usbtmc;
	mx_status_type mx_status;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RS232 pointer passed was NULL." );
	}

	usbtmc = (MX_USBTMC *) rs232->record->record_type_struct;

	if ( usbtmc->socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOCKET pointer for RS-232 record '%s' is NULL.",
			rs232->record->name );
	}

	mx_status = mx_socket_wait_for_event( usbtmc->socket,
						wait_time_in_seconds );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_usbtmc_flush( MX_RS232 *rs232 )
{
	/* There is no general way to flush bytes to the socket destination,
	 * so we just return a success status without actually doing anything.
	 */

	return MX_SUCCESSFUL_RESULT;
}


#endif /* HAVE_TCPIP */

