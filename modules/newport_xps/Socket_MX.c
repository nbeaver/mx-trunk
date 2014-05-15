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
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <errno.h>

#include "Socket.h"

#include "mx_util.h"
#include "mx_handle.h"
#include "mx_socket.h"

/*---*/

/* MX maintains information about sockets in an MX_SOCKET structure.
 * However, the Newport XPS library manipulates the socket file
 * descriptors directly.  We work around this by keeping the MX_SOCKET
 * structures in an MX_HANDLE_TABLE and arranging the handles to have
 * the same value as the socket file descriptor.
 */

static MX_HANDLE_TABLE *handle_table = NULL;

/*---*/

int
ConnectToServer( char *Ip_Address,
		int Ip_Port,
		double Timeout )
{
	MX_SOCKET *client_socket;
	int fd;
	mx_status_type mx_status;

	if ( handle_table == NULL ) {
		mx_status = mx_create_handle_table( &handle_table, 65536, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return (-1);
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

	return fd;
}

void
SetTCPTimeout( int SocketID,
		double Timeout )
{
	return;
}

void
SendAndReceive( int SocketID,
		char sSendString[],
		char sReturnString[],
		int iReturnStringSize )
{
	static const char fname[] = "SendAndReceive()";

	char receive_buffer[1024];
	MX_SOCKET *client_socket;
	void *client_socket_ptr;
	size_t num_bytes_received;
	mx_bool_type exit_loop;
	mx_status_type mx_status;

	mx_status = mx_get_pointer_from_handle( &client_socket_ptr,
						handle_table,
						SocketID );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	client_socket = (MX_SOCKET *) client_socket_ptr;

	MX_DEBUG(-2,("%s: sending '%s'", fname, sSendString));

	mx_status = mx_socket_send( client_socket,
				sSendString,
				strlen( sSendString ) );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	exit_loop = FALSE;

	sReturnString[0] = '\0';

	do {
		mx_status = mx_socket_receive( client_socket,
						receive_buffer,
						sizeof(receive_buffer),
						&num_bytes_received,
						NULL, 0 );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_TIMED_OUT:
			MX_DEBUG(-2,("%s: mx_socket_receive() timed out.",
				fname));
			exit_loop = TRUE;
			break;
		default:
			exit_loop = TRUE;
			break;
		}

		receive_buffer[ num_bytes_received ] = '\0';

		strlcat( sReturnString, receive_buffer, iReturnStringSize );

		if ( strstr(sReturnString, "EndOfAPI") ) {
			exit_loop = TRUE;
		}

	} while ( exit_loop == FALSE );

	MX_DEBUG(-2,("%s: received '%s'", fname, sReturnString));

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

	mx_status = mx_socket_close( client_socket );

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

