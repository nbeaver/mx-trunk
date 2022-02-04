/*
 * Name:    Socket_MX.c
 *
 * Purpose: A replacement for the Socket.cpp file in the Newport XPS source
 *          code that implements socket communication using MX facilities.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014-2016, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "Socket.h"
#include "XPS_C8_errors.h"

#include "mx_util.h"
#include "mx_handle.h"
#include "mx_clock.h"
#include "mx_socket.h"

/*---*/

#define NUM_SOCKETS		65536
#define TIMEOUT			300	/* As specified in the Newport code. */
#define SMALL_BUFFER_SIZE	1024

/* MX maintains information about sockets in an MX_SOCKET structure.
 * However, the Newport XPS library manipulates the socket file
 * descriptors directly.  We work around this by keeping the MX_SOCKET
 * structures in an MX_HANDLE_TABLE and arranging the handles to have
 * the same value as the socket file descriptor.
 */

static MX_HANDLE_TABLE *handle_table = NULL;

/* The timeout_table array contains the internal timeouts for each socket. */

static double *timeout_table = NULL;	/* in seconds */

/*---*/

/* mxp_newport_xps_comm_debug_flag enables XPS socket communication debugging.*/

static int mxp_newport_xps_comm_debug_flag = FALSE;

int
mxp_newport_xps_get_comm_debug_flag( void )
{
	return mxp_newport_xps_comm_debug_flag;
}

void
mxp_newport_xps_set_comm_debug_flag( int debug_flag )
{
	mxp_newport_xps_comm_debug_flag = debug_flag;
}

/*---*/

static void
mxp_newport_xps_error_string( char sReturnString[], int iReturnStringSize,
				long xps_error_code, const char *format, ... )
{
	va_list args;
	size_t prefix_length;

	snprintf( sReturnString, iReturnStringSize, "%ld,", xps_error_code );

	prefix_length = strlen( sReturnString );

	va_start( args, format );
	vsnprintf( sReturnString + prefix_length,
			iReturnStringSize - prefix_length,
			format, args );
	return;
}

/*---------------------------------------------------------------------------*/

int
ConnectToServer( char *Ip_Address,
		int Ip_Port,
		double Timeout )
{
	static const char fname[] = "ConnectToServer()";

	MX_SOCKET *client_socket;
	int fd;
	mx_status_type mx_status;

	if ( handle_table == NULL ) {
		mx_status = mx_create_handle_table( &handle_table,
						NUM_SOCKETS, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return (-1);

		timeout_table = (double *) calloc(NUM_SOCKETS, sizeof(double));

		if ( timeout_table == (double *) NULL ) {
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %d element "
			"array of socket timeout values.", NUM_SOCKETS );

			return (-1);
		}
	}

	mx_status = mx_tcp_socket_open_as_client( &client_socket,
						Ip_Address,
						Ip_Port,
						0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return (-1);

	fd = client_socket->socket_fd;

	mx_status = mx_replace_handle( fd, handle_table, client_socket );

	if ( mx_status.code != MXE_SUCCESS )
		return (-1);

	if ( mxp_newport_xps_comm_debug_flag ) {
		MX_DEBUG(-2,
		("* XPS: XPS socket %d opened for host '%s', port %d. *",
			fd, Ip_Address, Ip_Port ));
	}

	SetTCPTimeout( fd, Timeout );

	return fd;
}

void
SetTCPTimeout( int SocketID,
		double Timeout )
{
	static const char fname[] = "SetTCPTimeout()";

	if ( timeout_table == (double *) NULL ) {
		(void) mx_error( MXE_INITIALIZATION_ERROR, fname,
		"SetTCPTimeout() invoked, but no timeout table has been "
		"allocated yet.  Have you invoked ConnectToServer() yet?" );

		return;
	}

	if ( Timeout <= 0.0 ) {
		Timeout = TIMEOUT;
	} else
	if ( Timeout < 1.0e-3 ) {
		Timeout = 1.0e-3;
	}

	timeout_table[ SocketID ] = Timeout;	/* in seconds */

	if ( mxp_newport_xps_comm_debug_flag ) {
		MX_DEBUG(-2,("XPS: TCP timeout set to %f for XPS socket %d.",
			Timeout, SocketID ));
	}

	return;
}

/* WARNING: The Newport provided C++ library prototypes SendAndReceive() as
 * not having any return status code and doesn't throw any C++ exceptions
 * either.  The best solution I can come up with is to copy MX's MXE_...
 * error code and message into the sReturnString argument and have the
 * upper level MX code check for responses that begin with MXE_.
 */

void
SendAndReceive( int SocketID,
		char sSendString[],
		char sReturnString[],
		int iReturnStringSize )
{
	char receive_buffer[SMALL_BUFFER_SIZE];
	MX_SOCKET *client_socket;
	void *client_socket_ptr;
	size_t num_bytes_received;
	mx_bool_type exit_loop;
	double timeout;
	MX_CLOCK_TICK timeout_in_ticks;
	MX_CLOCK_TICK current_tick, finish_tick;
	int comparison;
	mx_bool_type check_for_timeouts;
	mx_status_type mx_status;

	mx_status = mx_get_pointer_from_handle( &client_socket_ptr,
						handle_table,
						SocketID );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_newport_xps_error_string( sReturnString, iReturnStringSize,
					ERR_INTERNAL_ERROR, mx_status.message );
		return;
	}

	client_socket = (MX_SOCKET *) client_socket_ptr;

	/*---*/

	mx_status = mx_socket_discard_unread_input( client_socket );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_newport_xps_error_string( sReturnString, iReturnStringSize,
					ERR_BUSY_SOCKET, mx_status.message );
		return;
	}

	/*---*/

	if ( mxp_newport_xps_comm_debug_flag ) {
		MX_DEBUG(-2,("XPS: sending to XPS socket %d: '%s'",
			SocketID, sSendString ));
	}

	mx_status = mx_socket_send( client_socket,
				sSendString,
				strlen( sSendString ) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mxp_newport_xps_error_string( sReturnString, iReturnStringSize,
					ERR_BUSY_SOCKET, mx_status.message );
		return;
	}

	/*---*/

	timeout = timeout_table[ SocketID ];

	/* Wait for the Newport to send us a response. */

	mx_status = mx_socket_wait_for_event( client_socket, timeout );

	/* Normally mx_socket_wait_for_event() hides MXE_TIMED_OUT errors.
	 * But in this case, we actually want to see the timeout errors.
	 */

	mx_status.code &= (~MXE_QUIET);

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_TIMED_OUT:
		(void) mx_error( mx_status.code,
				mx_status.location,
				mx_status.message );

		mxp_newport_xps_error_string( sReturnString, iReturnStringSize,
					ERR_TCP_TIMEOUT, mx_status.message );
		return;
		break;
	default:
		mxp_newport_xps_error_string( sReturnString, iReturnStringSize,
					ERR_INTERNAL_ERROR, mx_status.message );
		return;
		break;
	}

	/*
	 * Loop waiting until mx_socket_receive() returns a valid value
	 * or aborts.  Nominally, the fact that we only get here if the
	 * function mx_socket_wait_for_event() succeeds, means that
	 * mx_socket_receive() should succeed, unless the Newport started
	 * sending a message and stopped partway through.
	 */

	if ( timeout < 1.0e-6 ) {
		check_for_timeouts = FALSE;
	} else {
		check_for_timeouts = TRUE;

		timeout_in_ticks = mx_convert_seconds_to_clock_ticks( timeout );

		current_tick = mx_current_clock_tick();

		finish_tick = mx_add_clock_ticks( current_tick,
						timeout_in_ticks );
	}

	exit_loop = FALSE;

	sReturnString[0] = '\0';

	do {
		mx_status = mx_socket_receive( client_socket,
						receive_buffer,
						sizeof(receive_buffer),
						&num_bytes_received,
						NULL, 0, 0 );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;

		case MXE_TIMED_OUT:
			if ( mxp_newport_xps_comm_debug_flag ) {
				MX_DEBUG(-2,("XPS: mx_socket_receive() "
				"timed out for XPS socket %d.",
					SocketID ));
			}

			mxp_newport_xps_error_string(
					sReturnString, iReturnStringSize,
					ERR_TCP_TIMEOUT, mx_status.message );

			exit_loop = TRUE;
			break;
		default:
			mxp_newport_xps_error_string(
					sReturnString, iReturnStringSize,
					ERR_INTERNAL_ERROR, mx_status.message );

			exit_loop = TRUE;
			break;
		}

		receive_buffer[ num_bytes_received ] = '\0';

		strlcat( sReturnString, receive_buffer, iReturnStringSize );

		if ( strstr(sReturnString, "EndOfAPI") ) {
			exit_loop = TRUE;
		} else
		if ( check_for_timeouts ) {
			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
								finish_tick );

			if ( comparison >= 0 ) {
				exit_loop = TRUE;

				mxp_newport_xps_error_string(
					sReturnString, iReturnStringSize,
					ERR_TCP_TIMEOUT, mx_status.message );
			}
		}

	} while ( exit_loop == FALSE );

	if ( mxp_newport_xps_comm_debug_flag ) {
		MX_DEBUG(-2,("XPS: received from XPS socket %d: '%s'",
			SocketID, sReturnString ));
	}

	return;
}

void
CloseSocket( int SocketID )
{
	MX_SOCKET *client_socket;
	void *client_socket_ptr;
	mx_status_type mx_status;

	mx_status = mx_get_pointer_from_handle( &client_socket_ptr,
						handle_table,
						SocketID );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	client_socket = (MX_SOCKET *) client_socket_ptr;

	mx_status = mx_delete_handle( client_socket->socket_fd,
					handle_table );

	timeout_table[ SocketID ] = TIMEOUT;

	mx_status = mx_socket_close( client_socket );

	if ( mxp_newport_xps_comm_debug_flag ) {
		MX_DEBUG(-2,("* XPS: XPS socket %d closed. *", SocketID ));
	}

	return;
}

char *
GetError( int SocketID )
{
	int error_code;
	char *error_message;

	error_code = mx_socket_get_last_error();

	error_message = mx_socket_strerror( error_code );

	return error_message;
}

void
strncpyWithEOS( char *szStringOut,
		const char *szStringIn,
		int nNumberOfCharToCopy,
		int nStringOutSize )
{
	int bytes_to_copy;

	if ( nNumberOfCharToCopy >= nStringOutSize ) {
		bytes_to_copy = nStringOutSize;
	} else {
		bytes_to_copy = nNumberOfCharToCopy + 1;
	}

	strlcpy( szStringOut, szStringIn, bytes_to_copy );

	return;
}

