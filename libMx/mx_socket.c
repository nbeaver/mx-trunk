/*
 * Name:    mx_socket.c
 *
 * Purpose: BSD sockets and Winsock interface functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SOCKET_DEBUG		TRUE

#include <stdio.h>

#include "mx_osdef.h"

#if HAVE_TCPIP

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>

#ifdef OS_SOLARIS
# include <sys/filio.h>
#endif

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"
#include "mx_record.h"
#include "mx_ascii.h"
#include "mx_socket.h"
#include "mx_select.h"
#include "mx_net.h"

#if ( HAVE_UNIX_DOMAIN_SOCKETS && !defined(AF_LOCAL) )
#define AF_LOCAL	AF_UNIX
#endif

#ifndef INADDR_NONE
#define INADDR_NONE	0xffffffff
#endif

/********************************************************************/

static int mx_sockets_are_initialized = FALSE;

#if defined( OS_WIN32 )
   static mx_status_type mx_winsock_initialize( void );
   static char * mx_winsock_strerror( int winsock_errno );

#elif defined( OS_DJGPP )
   static mx_status_type mx_watt32_initialize( void );

#elif defined( OS_ECOS )
   static mx_status_type mx_ecos_net_initialize( void );
#endif

MX_EXPORT mx_status_type
mx_socket_initialize( void )
{
	mx_status_type mx_status;

	if ( mx_sockets_are_initialized )
		return MX_SUCCESSFUL_RESULT;

#if defined( OS_WIN32 )
	mx_status = mx_winsock_initialize();

#elif defined( OS_DJGPP )
	mx_status = mx_watt32_initialize();

#elif defined( OS_ECOS )
	mx_status = mx_ecos_net_initialize();
#else
	mx_sockets_are_initialized = TRUE;

	mx_status = MX_SUCCESSFUL_RESULT;
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mx_socket_wait_for_event( MX_SOCKET *mx_socket, double timeout_in_seconds )
{
	static const char fname[] = "mx_socket_wait_for_event()";

	int socket_fd, fd_count, num_fds;
	int saved_errno;
	fd_set read_fds;

	struct timeval timeout;
	unsigned long tv_seconds;
	unsigned long tv_microseconds;
	double timeout_remainder;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}

	socket_fd = mx_socket->socket_fd;

	tv_seconds = (unsigned long) timeout_in_seconds;

	timeout_remainder = timeout_in_seconds - (double) tv_seconds;

	tv_microseconds = (unsigned long) (1.0e6 * timeout_remainder);

#if MX_SOCKET_DEBUG
	MX_DEBUG(-2,("%s: socket_fd = %d, timeout = %g seconds",
		fname, socket_fd, timeout_in_seconds));
#endif

	while(1) {
		FD_ZERO( &read_fds );
		FD_SET( socket_fd, &read_fds );

		timeout.tv_sec = (long) tv_seconds;
		timeout.tv_usec = (long) tv_microseconds;

#if defined(OS_WIN32)
		fd_count = -1;
#else
		fd_count = socket_fd + 1;
#endif

#if MX_SOCKET_DEBUG
		MX_DEBUG(-2,
		("%s: About to call select() for manager socket fd = %d",
			fname, socket_fd));
#endif
		num_fds = select( fd_count, &read_fds, NULL, NULL, &timeout );

		saved_errno = errno;

#if MX_SOCKET_DEBUG
		MX_DEBUG(-2,("%s: select() for socket %d, fd_count = %d, "
			"num_fds = %d, errno = %d",
			fname, socket_fd, fd_count,
			num_fds, saved_errno));
#endif
		if ( num_fds < 0 ) {
			if ( saved_errno == EINTR ) {
#if MX_SOCKET_DEBUG
				MX_DEBUG(-2,
				("%s: EINTR returned by select()", fname));
#endif

				/* Receiving an EINTR errno from select()
				 * is normal.  It just means that a signal
				 * handler fired while we were blocked in
				 * the select() system call.
				 */
			} else {
				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"Error in select() while waiting for events.  "
				"Errno = %d.  Error string = '%s'.",
					saved_errno, strerror( saved_errno ) );
			}

		} else if ( num_fds == 0 ) {

			/* Did not get any events, so we return
			 * a quiet timeout error.
			 */

			return mx_error( MXE_TIMED_OUT | MXE_QUIET, fname,
			"Timed out after waiting %g seconds for socket %d "
			"to become readable.", timeout_in_seconds, socket_fd );
		} else {
			/* Check to verify that FD_ISSET() is indeed set.*/

			if ( FD_ISSET( socket_fd, &read_fds ) == 0 ) {
				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"select() said that %d fds were ready to be "
				"read, but socket %d says that it is _not_ "
				"ready to be read.",
					num_fds, socket_fd );
			}

			/* If we get here, then the socket is ready to be read,
			 * so return a success status.
			 */

#if MX_SOCKET_DEBUG
			MX_DEBUG(-2,("%s: socket %d is ready to be read.",
				fname, socket_fd));
#endif
			return MX_SUCCESSFUL_RESULT;
		}
	}
}

/********************************************************************/

#if defined( OS_UNIX ) || defined( OS_WIN32 ) || defined( OS_CYGWIN ) \
	|| defined( OS_RTEMS ) || defined( OS_VXWORKS ) \
	|| defined( OS_VMS ) || defined( OS_DJGPP )

MX_EXPORT mx_status_type
mx_gethostname( char *name, size_t maximum_length )
{
	static const char fname[] = "mx_gethostname()";

	int status, saved_errno;

#if defined( OS_WIN32 )
	if ( mx_sockets_are_initialized == FALSE ) {
		mx_status_type mx_status;

		mx_status = mx_socket_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_sockets_are_initialized = TRUE;
	}
#endif

	status = gethostname( name, maximum_length );

	if ( status != 0 ) {
		saved_errno = mx_socket_get_last_error();

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
	"Unable to get the hostname for the computer we are running on.  "
	"errno = %d, error message = '%s'",
			saved_errno, mx_socket_strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#elif defined( OS_ECOS )

MX_EXPORT mx_status_type
mx_gethostname( char *name, size_t maximum_length )
{
	strlcpy( name, "eCos", maximum_length );

	return MX_SUCCESSFUL_RESULT;
}

#else

#error mx_gethostname() is not yet implemented for this operating system.

#endif

/************************* TCP/IP sockets ***************************/

MX_EXPORT mx_status_type
mx_tcp_socket_open_as_client( MX_SOCKET **client_socket,
		char *hostname, long port_number,
		unsigned long socket_flags,
		size_t buffer_size )
{
	static char fname[] = "mx_tcp_socket_open_as_client()";

	struct sockaddr_in destination_address;
	unsigned long inet_address;

	int saved_errno, status;
	long mask, error_code;
	char *error_string;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( client_socket == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"client_socket pointer passed is NULL." );
	}
	if ( hostname == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null hostname string pointer given as an argument." );
	}
	if ( port_number <= 0 || port_number > 65535 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal port number %ld given as an argument.", port_number );
	}

	if ( mx_sockets_are_initialized == FALSE ) {

		mx_status = mx_socket_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_sockets_are_initialized = TRUE;
	}

	mx_status = mx_socket_get_inet_address( hostname, &inet_address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	memset((char *) &destination_address, 0, sizeof(destination_address));

	destination_address.sin_family = AF_INET;
	destination_address.sin_addr.s_addr = (uint32_t) inet_address;
	destination_address.sin_port = htons( (unsigned short) port_number );

	/* Create a socket. */

	*client_socket = (MX_SOCKET *) malloc( sizeof(MX_SOCKET) );

	if ( (*client_socket) == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory creating a TCP client socket." );
	}

	(*client_socket)->socket_fd = socket( AF_INET, SOCK_STREAM, 0 );

	saved_errno = mx_socket_check_error_status(
			&((*client_socket)->socket_fd),
			MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *client_socket );

		*client_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Attempt to create a TCP socket failed.  "
			"Errno = %d.  Error string = '%s'.",
			saved_errno, error_string );
	}

	/* Sockets start out in blocking mode. */

	(*client_socket)->is_non_blocking = FALSE;

	/* Save the socket flags argument. */

	(*client_socket)->socket_flags = socket_flags;

	/* Open the socket. */

	MX_DEBUG( 2,("%s: About to invoke connect().",fname));

	status = connect( (*client_socket)->socket_fd,
			(struct sockaddr *) &destination_address,
			sizeof( destination_address ) );

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mask = MXF_SOCKET_QUIET | MXF_SOCKET_QUIET_CONNECTION;

		if ( socket_flags & mask ) {
			error_code = (MXE_NETWORK_IO_ERROR | MXE_QUIET);
		} else {
			error_code = MXE_NETWORK_IO_ERROR;
		}

		mx_socket_close( *client_socket );

		*client_socket = NULL;

		return mx_error( error_code, fname,
			"connect() to host '%s', port %ld failed.  "
			"Errno = %d.  Error string = '%s'.",
				hostname, port_number,
				saved_errno, error_string );
	}

	if ( socket_flags & MXF_SOCKET_DISABLE_NAGLE_ALGORITHM ) {

#if defined(OS_ECOS)
		mx_warning(
		"Disabling the Nagle algorithm on eCos is not supported." );
#else

		int flag = 1;

		status = setsockopt( (*client_socket)->socket_fd,
				IPPROTO_TCP, TCP_NODELAY,
				( char * ) &flag,
				sizeof(int) );

		saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

		if ( saved_errno != 0 ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
"An attempt to disable the Nagle algorithm for host '%s', port %ld failed.  "
"Errno = %d.  Error string = '%s'.",
				hostname, port_number,
				saved_errno, error_string );
		}
#endif
	}

	MX_DEBUG( 2,("Leaving %s.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_tcp_socket_open_as_server( MX_SOCKET **server_socket,
				long port_number,
				unsigned long socket_flags,
				size_t buffer_size )
{
	static char fname[] = "mx_tcp_socket_open_as_server()";

	struct sockaddr_in server_address;
	int saved_errno, status, reuseaddr;
	char *error_string;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s: port_number = %ld, socket_flags = %#lx",
			fname, port_number, socket_flags ));

	if ( server_socket == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_socket pointer passed is NULL." );
	}
	if ( port_number <= 0 || port_number > 65535 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal port number %ld given as an argument.", port_number );
	}

	if ( mx_sockets_are_initialized == FALSE ) {

		mx_status = mx_socket_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_sockets_are_initialized = TRUE;
	}

	/* Create a socket. */

	*server_socket = (MX_SOCKET *) malloc( sizeof(MX_SOCKET) );

	if ( (*server_socket) == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory creating a TCP server socket." );
	}

	(*server_socket)->socket_fd = socket( AF_INET, SOCK_STREAM, 0 );

	saved_errno = mx_socket_check_error_status(
			&((*server_socket)->socket_fd),
			MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Attempt to create a TCP socket failed.  "
			"Errno = %d.  Error string = '%s'.",
			saved_errno, error_string );
	}

	/* Sockets start out in blocking mode. */

	(*server_socket)->is_non_blocking = FALSE;

	/* Save the socket flags argument. */

	(*server_socket)->socket_flags = socket_flags;

	/* Allow address reuse. */

	reuseaddr = 1;

	status = setsockopt( (*server_socket)->socket_fd, SOL_SOCKET,
		SO_REUSEADDR, (char *) &reuseaddr, sizeof(reuseaddr) );

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Attempt to set SO_REUSEADDR option for server socket for port %ld failed.  "
			"Errno = %d.  Error string = '%s'.",
			port_number, saved_errno, error_string );
	}

	/* Bind to the local port so that clients can connect to us. */

	memset((char *) &server_address, 0, sizeof( server_address ));

	server_address.sin_family      = AF_INET;
	server_address.sin_addr.s_addr = htonl( INADDR_ANY );
	server_address.sin_port        = htons( (unsigned short) port_number );

	status = bind( (*server_socket)->socket_fd,
				(struct sockaddr *) &server_address,
				sizeof( server_address ) );

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Attempt to bind server socket to port %ld failed.  "
			"Errno = %d.  Error string = '%s'.",
			port_number, saved_errno, error_string );
	}

	/* Indicate our willingness to accept connections. */

	status = listen((*server_socket)->socket_fd, 5); /* 5 is traditional. */

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Attempt to listen for connections to port %ld failed.  "
			"Errno = %d.  Error string = '%s'.",
			port_number, saved_errno, error_string );
	}

	/* Set the socket to non-blocking mode. */

	mx_status = mx_socket_set_non_blocking_mode( *server_socket, TRUE );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_status;
	}

	/* If requested disable the Nagle algorithm.  The Nagle algorithm
	 * tries to coalesce multiple small socket sends into a bigger
	 * send in order to increase network I/O efficiency.  However,
	 * some kinds of operations are incompatible with the Nagle
	 * algorithm, so we give a way of turning it off.
	 */

	if ( socket_flags & MXF_SOCKET_DISABLE_NAGLE_ALGORITHM ) {

#if defined(OS_ECOS)
		mx_warning(
		"Disabling the Nagle algorithm on eCos is not supported." );
#else

		int flag = 1;

		status = setsockopt( (*server_socket)->socket_fd,
				IPPROTO_TCP, TCP_NODELAY,
				( char * ) &flag,
				sizeof(int) );

		saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

		if ( saved_errno != 0 ) {
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Attempt to disable the Nagle algorithm for port %ld failed.  "
		"Errno = %d.  Error string = '%s'.",
				port_number, saved_errno, error_string );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/************************* Unix domain sockets ***************************/

#if ( HAVE_UNIX_DOMAIN_SOCKETS == 0 )

MX_EXPORT mx_status_type
mx_unix_socket_open_as_client( MX_SOCKET **client_socket,
				char *pathname,
				unsigned long socket_flags,
				size_t buffer_size )
{
	static char fname[] = "mx_unix_socket_open_as_client()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Unix domain sockets are not supported on this system." );
}

MX_EXPORT mx_status_type
mx_unix_socket_open_as_server( MX_SOCKET **server_socket,
				char *pathname,
				unsigned long socket_flags,
				size_t buffer_size )
{
	static char fname[] = "mx_unix_socket_open_as_server()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Unix domain sockets are not supported on this system." );
}

#else /**** HAVE_UNIX_DOMAIN_SOCKETS ****/

MX_EXPORT mx_status_type
mx_unix_socket_open_as_client( MX_SOCKET **client_socket,
				char *pathname,
				unsigned long socket_flags,
				size_t buffer_size )
{
	static char fname[] = "mx_unix_socket_open_as_client()";

	struct sockaddr_un destination_address;
	void *sockaddr_ptr;

	int saved_errno, status;
	long mask, error_code;
	char *error_string;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( client_socket == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"client_socket pointer passed is NULL." );
	}
	if ( pathname == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null pathname string pointer given as an argument." );
	}

	if ( mx_sockets_are_initialized == FALSE ) {

		mx_status = mx_socket_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_sockets_are_initialized = TRUE;
	}

	memset((char *) &destination_address, 0, sizeof(destination_address));

	destination_address.sun_family = AF_LOCAL;

	strlcpy( destination_address.sun_path, pathname, 
				sizeof( destination_address.sun_path ) );

	/* Create a socket. */

	*client_socket = (MX_SOCKET *) malloc( sizeof(MX_SOCKET) );

	if ( (*client_socket) == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory creating a Unix domain client socket." );
	}

	(*client_socket)->socket_fd = socket( AF_LOCAL, SOCK_STREAM, 0 );

	saved_errno = mx_socket_check_error_status(
			&((*client_socket)->socket_fd),
			MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *client_socket );

		*client_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Attempt to create a Unix domain socket failed.  "
			"Errno = %d.  Error string = '%s'.",
			saved_errno, error_string );
	}

	/* Sockets start out in blocking mode. */

	(*client_socket)->is_non_blocking = FALSE;

	/* Save the socket flags argument. */

	(*client_socket)->socket_flags = socket_flags;

	/* Open the socket. */

	MX_DEBUG( 2,("%s: About to invoke connect().",fname));

	/* Assignment to a void pointer avoids warning messages like
	 * 'cast increases required alignment of target type' on Irix.
	 */

	sockaddr_ptr = &destination_address;

	status = connect( (*client_socket)->socket_fd, sockaddr_ptr,
			sizeof( destination_address ) );

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mask = MXF_SOCKET_QUIET | MXF_SOCKET_QUIET_CONNECTION;

		if ( socket_flags & mask ) {
			error_code = (MXE_NETWORK_IO_ERROR | MXE_QUIET);
		} else {
			error_code = MXE_NETWORK_IO_ERROR;
		}

		mx_socket_close( *client_socket );

		*client_socket = NULL;

		return mx_error( error_code, fname,
			"connect() to Unix domain socket '%s' failed.  "
			"Errno = %d.  Error string = '%s'.",
				pathname, saved_errno, error_string );
	}

	MX_DEBUG( 2,("Leaving %s.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_unix_socket_open_as_server( MX_SOCKET **server_socket,
				char *pathname,
				unsigned long socket_flags,
				size_t buffer_size )
{
	static char fname[] = "mx_unix_socket_open_as_server()";

	struct sockaddr_un server_address;
	void *sockaddr_ptr;
	int saved_errno, status, reuseaddr;
	char *error_string;
	mx_status_type mx_status;

	if ( server_socket == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_socket pointer passed is NULL." );
	}
	if ( pathname == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null pathname string pointer given as an argument." );
	}

	MX_DEBUG( 2,("%s: pathname = '%s', socket_flags = %#lx",
			fname, pathname, socket_flags ));

	if ( mx_sockets_are_initialized == FALSE ) {

		mx_status = mx_socket_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_sockets_are_initialized = TRUE;
	}

	/* If a file with the same name as our new socket already exists,
	 * try to delete it.  It is not an error if the file does not
	 * already exist.
	 */

	status = remove( pathname );

	if ( ( status != 0 ) && ( errno != ENOENT ) ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Attempt to delete file '%s' failed.  "
			"Errno = %d.  Error string = '%s'.",
			pathname, saved_errno, strerror( saved_errno ) );
	}

	/* Create a socket. */

	*server_socket = (MX_SOCKET *) malloc( sizeof(MX_SOCKET) );

	if ( (*server_socket) == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory creating a Unix domain server socket." );
	}

	(*server_socket)->socket_fd = socket( AF_LOCAL, SOCK_STREAM, 0 );

	saved_errno = mx_socket_check_error_status(
			&((*server_socket)->socket_fd),
			MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Attempt to create a Unix domain socket failed.  "
			"Errno = %d.  Error string = '%s'.",
			saved_errno, error_string );
	}

	/* Sockets start out in blocking mode. */

	(*server_socket)->is_non_blocking = FALSE;

	/* Save the socket flags argument. */

	(*server_socket)->socket_flags = socket_flags;

	/* Allow address reuse. */

	reuseaddr = 1;

	status = setsockopt( (*server_socket)->socket_fd, SOL_SOCKET,
		SO_REUSEADDR, (char *) &reuseaddr, sizeof(reuseaddr) );

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Attempt to set SO_REUSEADDR option for server socket '%s' failed.  "
			"Errno = %d.  Error string = '%s'.",
			pathname, saved_errno, error_string );
	}

	/* Bind to the local port so that clients can connect to us. */

	memset((char *) &server_address, 0, sizeof( server_address ));

	server_address.sun_family = AF_LOCAL;

	strlcpy( server_address.sun_path, pathname, 
				sizeof( server_address.sun_path ) );

	/* Assignment to a void pointer avoids warning messages like
	 * 'cast increases required alignment of target type' on Irix.
	 */

	sockaddr_ptr = &server_address;

	status = bind( (*server_socket)->socket_fd, sockaddr_ptr,
			sizeof( server_address ) );

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Attempt to bind server socket to pathname '%s' failed."
			"  Errno = %d.  Error string = '%s'.",
			pathname, saved_errno, error_string );
	}

	/* Indicate our willingness to accept connections. */

	status = listen((*server_socket)->socket_fd, 5); /* 5 is traditional. */

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	if ( saved_errno != 0 ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Attempt to listen for connections on socket '%s' failed.  "
			"Errno = %d.  Error string = '%s'.",
			pathname, saved_errno, error_string );
	}

	/* Set the socket to non-blocking mode. */

	mx_status = mx_socket_set_non_blocking_mode( *server_socket, TRUE );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_socket_close( *server_socket );

		*server_socket = NULL;

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /**** HAVE_UNIX_DOMAIN_SOCKETS ****/

/************************* All sockets ***************************/

#if defined( OS_WIN32 ) || defined( OS_DJGPP )
#  define mx_closesocket(fd)	closesocket(fd)

#elif (defined( OS_VMS ) && defined( HAVE_WINTCP ))
#  define mx_closesocket(fd)	vms_close(fd)

#else
#  define mx_closesocket(fd)	close(fd)
#endif

/*---------*/

MX_EXPORT mx_status_type
mx_socket_close( MX_SOCKET *mx_socket )
{
	static char fname[] = "mx_socket_close()";

	MX_SOCKET_FD socket_fd;
	int status, saved_errno;
	char c, *error_string;
	mx_status_type mx_status;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}

	socket_fd = mx_socket->socket_fd;

	MX_DEBUG( 2,("%s invoked for socket %d.", fname, socket_fd));

	/* Set the socket to non-blocking mode.
	 *
	 * Note: This is the last point in the function where we still
	 * need the MX_SOCKET structure, so we free that structure
	 * immediately after the call.
	 */

	mx_status = mx_socket_set_non_blocking_mode( mx_socket, TRUE );

	mx_free(mx_socket);

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_closesocket( socket_fd );
		return mx_status;
	}

	/* "Half-close" the socket by disallowing any further writes
	 * from our end.
	 */

	status = shutdown( socket_fd, 1 );

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	switch ( saved_errno ) {
	case 0:
		break;		/* Everything is fine. */

	case ENOTCONN:		/* The server has already disconnected. */
	case EINVAL:
		/* Return now since there is nothing further we can do. */

		mx_closesocket( socket_fd );

		return MX_SUCCESSFUL_RESULT;
	default:
		mx_closesocket( socket_fd );

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Error while executing shutdown( mx_socket, 1 ).  "
		"Errno = %d.  Error string = '%s'.",
		saved_errno, error_string );
	}

	/* Read from the socket until there is no more data. */

	for(;;) {
		status = recv( socket_fd, &c, 1, 0 );

		if ( status == 0 ) {
			/* The connection is closed, so we're done. */
			break;		/* exit the for() loop. */

		} else if ( status == MX_SOCKET_ERROR ) {
			saved_errno = mx_socket_get_last_error();

			if ( saved_errno == EWOULDBLOCK ) {

				/* Nothing more to read, so we're done. */

				break;	/* exit the for() loop. */
			} else
			if ( saved_errno == ECONNRESET ) {

				/* The remote process reset the connection,
				 * so we're done.
				 */

				break;	/* exit the for() loop. */
			} else {
				mx_closesocket( socket_fd );

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Error during socket close procedure while executing recv().  "
	"Errno = %d.  Error string = '%s'.", saved_errno,
					mx_socket_strerror( saved_errno ) );
			}
		}
	}

	/* Now finish shutting down the connection. */

	status = shutdown( socket_fd, 2 );

	saved_errno = mx_socket_check_error_status(
			&status, MXF_SOCKCHK_INVALID, &error_string );

	switch ( saved_errno ) {
	case 0:

	case ENOTCONN:			/* The server didn't wait for us. */
	case EINVAL:
	case ECONNRESET:

		/* Nothing to do. */

		break;
	default:
		mx_closesocket( socket_fd );

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Error while executing shutdown( mx_socket, 2 ).  "
			"Errno = %d.  Error string = '%s'.",
			saved_errno, error_string );
	}

	mx_closesocket( socket_fd );

	return MX_SUCCESSFUL_RESULT;
}

#undef mx_closesocket

/*---------*/

MX_EXPORT int
mx_socket_ioctl( MX_SOCKET *mx_socket,
		int ioctl_type,
		void *ioctl_value )
{
	static const char fname[] = "mx_socket_ioctl()";

	int ioctl_status, socket_errno;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );

		errno = EINVAL;

		return -1;
	}

#if defined( OS_WIN32 )
#   if defined(__BORLANDC__) || defined(__GNUC__)
	ioctl_status = ioctlsocket( mx_socket->socket_fd,
					ioctl_type, ioctl_value );
#   else
	ioctl_status = ioctlsocket( mx_socket->socket_fd,
					ioctl_type, ioctl_value );
#   endif
#elif defined( OS_DJGPP )
	ioctl_status = ioctlsocket( mx_socket->socket_fd,
					ioctl_type, (char *) ioctl_value );
#elif defined( OS_VXWORKS )
	ioctl_status = ioctl( mx_socket->socket_fd,
					ioctl_type, (int) ioctl_value );
#else
	ioctl_status = ioctl( mx_socket->socket_fd,
					ioctl_type, ioctl_value );
#endif

	socket_errno = mx_socket_check_error_status(
			&ioctl_status, MXF_SOCKCHK_INVALID, NULL );

	return socket_errno;
}

#if defined(OS_SOLARIS)
#  include <sys/utsname.h>
#endif

#if defined(OS_WIN32) \
	|| ( defined(OS_VMS) && (__VMS_VER < 80200000) )

/* Win32 does not support F_GETFL for fcntl(), so we manually save a flag
 * in mx_socket_set_non_blocking_mode() that we report back here.
 */

MX_EXPORT mx_status_type
mx_socket_get_non_blocking_mode( MX_SOCKET *mx_socket,
				mx_bool_type *non_blocking_flag )
{
	static const char fname[] = "mx_socket_get_non_blocking_flag()";

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}
	if ( non_blocking_flag == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The non_blocking_flag pointer passed was NULL." );
	}

	*non_blocking_flag = mx_socket->is_non_blocking;

	return MX_SUCCESSFUL_RESULT;
}

#else /* not OS_WIN32 */

MX_EXPORT mx_status_type
mx_socket_get_non_blocking_mode( MX_SOCKET *mx_socket,
				mx_bool_type *non_blocking_flag )
{
	static const char fname[] = "mx_socket_get_non_blocking_flag()";

	int status, fd_flags, saved_errno;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}
	if ( non_blocking_flag == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The non_blocking_flag pointer passed was NULL." );
	}

#if defined(OS_VXWORKS)
	status = ioctl( mx_socket->socket_fd, FIOGETFL, (int) &fd_flags );

	MX_DEBUG(-10,
	("%s: FIXME! Verify that the fd_flags (%#x) returned by VxWorks "
	"actually contains the non-blocking socket info that we need!",
		fname, fd_flags ));
#else
	fd_flags = fcntl( mx_socket->socket_fd, F_GETFL, 0 );

	status = fd_flags;
#endif

	if ( status == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The attempt to get the socket flags failed for socket %d.  "
		"Errno = %d.  Error string = '%s'.", mx_socket->socket_fd,
			saved_errno, strerror( saved_errno ) );
	}

	if ( fd_flags & O_NONBLOCK ) {
		mx_socket->is_non_blocking = TRUE;
	} else {
		mx_socket->is_non_blocking = FALSE;
	}

#if defined(OS_VXWORKS)
	MX_DEBUG(-10,("%s: mx_socket->is_non_blocking = %d",
		fname, (int) mx_socket->is_non_blocking));
#endif

	*non_blocking_flag = mx_socket->is_non_blocking;

	return MX_SUCCESSFUL_RESULT;
}

#endif /* not OS_WIN32 */


MX_EXPORT mx_status_type
mx_socket_set_non_blocking_mode( MX_SOCKET *mx_socket,
				mx_bool_type non_blocking_flag )
{
	static const char fname[] = "mx_socket_set_non_blocking_flag()";

	int socket_errno, non_blocking;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}

#if defined(OS_VMS)

	/* FIXME: (WML - Jan. 12. 2006)
	 *        For now we have disabled non-blocking mode on these platforms
	 *        due to unfixed issues with their behavior.
	 */

	return MX_SUCCESSFUL_RESULT;
#endif

#if defined(OS_SOLARIS)
	{
		/* (WML - Oct. 11, 2006) - As of today, this problem
		 * is still here.  In one test, using non-blocking I/O
		 * caused the data transfer to be around 15 times slower.
		 */

		/* FIXME - FIXME - FIXME  (WML - Dec. 5, 2005)
		 * For some reason, on Solaris 10, non-blocking socket I/O
		 * seems to be very slow compared to blocking I/O.  However,
		 * Solaris 8 does not seem to have this problem.
		 *
		 * The socket code is scheduled to be rewritten for MX 2.0,
		 * so for now we put in a "temporary" patch that disables
		 * non-blocking I/O for Solaris 9 and above.  I do not
		 * currently have a copy of Solaris 9 to test with, so
		 * disabling it on Solaris 9 is just being careful.
		 */

		int os_major, os_minor, os_update;
		mx_status_type mx_status;

		mx_status = mx_get_os_version(&os_major, &os_minor, &os_update);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: os_major = %d, os_minor = %d",
			fname, os_major, os_minor ));

		/* If this is Solaris 9 or newer, return
		 * without doing anything.
		 */

		if ( (os_major >= 5) && (os_minor >= 9) ) {
			return MX_SUCCESSFUL_RESULT;
		}
	}
#endif

	if ( non_blocking_flag ) {
		non_blocking = 1;

		mx_socket->is_non_blocking = TRUE;
	} else {
		non_blocking = 0;

		mx_socket->is_non_blocking = FALSE;
	}

	socket_errno = mx_socket_ioctl( mx_socket, FIONBIO, &non_blocking );

	if ( socket_errno != 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Error while trying to set socket %d to non-blocking mode.  "
		"Errno = %d.  Error string = '%s'.",
			mx_socket->socket_fd,
			socket_errno,
			mx_socket_strerror( socket_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_bool_type
mx_socket_is_open( MX_SOCKET *mx_socket )
{
	/* Check for simple error cases. */

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return FALSE;
	}

#ifdef OS_WIN32
	if ( mx_socket->socket_fd == INVALID_SOCKET ) {
		return FALSE;
	}
#else
	if ( mx_socket->socket_fd < 0 ) {
		return FALSE;
	}
#endif

	return TRUE;
}

MX_EXPORT void
mx_socket_mark_as_closed( MX_SOCKET *mx_socket )
{
	if ( mx_socket != (MX_SOCKET *) NULL ) {
		mx_socket->socket_fd = MX_INVALID_SOCKET_FD;
	}
	return;
}

MX_EXPORT char *
mx_socket_strerror( int saved_errno )
{
#ifdef OS_WIN32
	return mx_winsock_strerror( saved_errno );
#else
	return strerror( saved_errno );
#endif
}


MX_EXPORT int
mx_socket_check_error_status( void *value_to_check,
				int type_of_check, char **error_string )
{
	static char fname[] = "mx_socket_check_error_status()";

	int saved_errno, int_value;

	switch( type_of_check ) {
	case MXF_SOCKCHK_INVALID:
		int_value = *( (int *) value_to_check );

		if ( int_value != MX_INVALID_SOCKET_FD ) {
			if ( error_string != NULL ) {
				*error_string = NULL;
			}
			return 0;
		}
		break;
	case MXF_SOCKCHK_ZERO:
		int_value = *( (int *) value_to_check );

		if ( int_value != 0 ) {
			if ( error_string != NULL ) {
				*error_string = NULL;
			}
			return 0;
		}
		break;
	case MXF_SOCKCHK_NULL:
		if ( value_to_check != NULL ) {
			if ( error_string != NULL ) {
				*error_string = NULL;
			}
			return 0;
		}
		break;
	default:
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown type_of_check = %d.", type_of_check );

		if ( error_string != NULL ) {
			*error_string = NULL;
		}

		return -1;
	}

	saved_errno = mx_socket_get_last_error();

	if ( error_string != NULL ) {
		*error_string = mx_socket_strerror( saved_errno );
	}

	return saved_errno;
}

MX_EXPORT mx_status_type
mx_socket_get_inet_address( const char *hostname, unsigned long *inet_address )
{
	static const char fname[] = "mx_socket_get_inet_address()";

	MX_DEBUG( 2,("%s invoked for hostname '%s'.", fname, hostname));

	/* First test to see if hostname is a dotted decimal number.  Note
	 * that 255.255.255.255 will fail this test.
	 */

	*inet_address = inet_addr( hostname );

	if ( *inet_address == INADDR_NONE ) {

		struct hostent *host_entry;
		struct in_addr *in_addr_struct;
		int error_value;
		void *void_ptr;

		/* The hostname is not dotted decimal, so try using
		 * gethostbyname() to look the name up.  Note that
		 * gethostbyname() blocks while looking the address up.
		 */

#if defined(OS_VXWORKS)
		host_entry = NULL;	/* FIXME */
#else
		host_entry = gethostbyname( hostname );
#endif

		if ( host_entry != NULL ) {

			/* Assign h_addr to a void pointer first in order
			 * to avoid the warning 'cast increases required
			 * alignment of target type' from GCC 2.8.1 under
			 * Solaris 2.6.
			 */

			void_ptr = host_entry->h_addr;

			in_addr_struct = (struct in_addr *) void_ptr;

			*inet_address = in_addr_struct->s_addr;
		} else {
#if defined(OS_WIN32)
			error_value = WSAGetLastError();
#elif defined(OS_VXWORKS)
			error_value = HOST_NOT_FOUND;	/* FIXME */
#else
			error_value = h_errno;
#endif
			switch( error_value ) {
			case HOST_NOT_FOUND:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
					"The host name '%s' was not found.",
					hostname );

#ifdef NO_ADDRESS
			case NO_ADDRESS:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The requested domain name '%s' is valid, but does not have an IP address.",
					hostname );
#endif

			case NO_RECOVERY:
				return mx_error( MXE_FUNCTION_FAILED, fname,
				"The domain name '%s' does not exist.",
					hostname );

			case TRY_AGAIN:
				return mx_error( MXE_TRY_AGAIN, fname,
		"The domain name '%s' does not currently seem to exist, "
		"but this is likely to be a temporary condition, "
		"so try again later.", hostname );

			default:
				return mx_error( MXE_FUNCTION_FAILED, fname,
"An unrecognized error code %d was returned by the call gethostbyname('%s')",
					error_value, hostname );
			}
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_socket_send( MX_SOCKET *mx_socket,
		void *message_buffer,
		size_t message_length_in_bytes )
{
	static const char fname[] = "mx_socket_send()";

	long bytes_left, bytes_sent, error_code;
	int saved_errno;
	char *ptr;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SOCKET pointer passed was NULL." );
	}

	bytes_left = (long) message_length_in_bytes;

	ptr = (char *) message_buffer;

	while( bytes_left > 0 ) {
		bytes_sent = send( mx_socket->socket_fd,
					ptr, (int) bytes_left, 0 );

		switch( bytes_sent ) {
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			switch( saved_errno ) {
			case ECONNRESET:
			case ECONNABORTED:
			case EPIPE:
				if (mx_socket->socket_flags & MXF_SOCKET_QUIET)
				{
				    error_code =
				      (MXE_NETWORK_CONNECTION_LOST | MXE_QUIET);
				} else {
				    error_code = MXE_NETWORK_CONNECTION_LOST;
				}

				return mx_error( error_code, fname,
				"Network connection lost.  "
				"Errno = %d, error text = '%s'",
					saved_errno,
					mx_socket_strerror(saved_errno));
				break;
			default:
				return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Error sending to remote host.  Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));
				break;
			}
			break;
		default:
			bytes_left -= bytes_sent;

#if MX_SOCKET_DEBUG
			{
				long n;

				fprintf( stderr, "%s:", fname );

				for ( n = 0; n < bytes_sent; n++ ) {
					fprintf( stderr, " %#02x",
						ptr[n] & 0xff );
				}

				fprintf( stderr, "   bytes_left = %ld\n",
						bytes_left );
			}
#endif

			ptr += bytes_sent;
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_socket_receive( MX_SOCKET *mx_socket,
		void *message_buffer,
		size_t message_length_in_bytes,
		size_t *num_bytes_received,
		void *input_terminators,
		size_t input_terminators_length_in_bytes )
{
	static const char fname[] = "mx_socket_receive()";

	long bytes_left, bytes_received, total_bytes_received, error_code;
	int i, saved_errno, num_terminators_seen;
	char *ptr, *terminators;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SOCKET pointer passed was NULL." );
	}

	bytes_left = (long) message_length_in_bytes;

	ptr = (char *) message_buffer;

	terminators = (char *) input_terminators;

	total_bytes_received = 0;
	num_terminators_seen = 0;

	while( bytes_left > 0 ) {
		bytes_received = recv( mx_socket->socket_fd,
					ptr, (int) bytes_left, 0 );

#if MX_SOCKET_DEBUG
		if ( bytes_received < bytes_left ) {
		    MX_DEBUG(-2,("%s: bytes_left = %ld, bytes_received = %ld",
			fname, bytes_left, bytes_received));
		}
#endif

		switch( bytes_received ) {
		case 0:
			*ptr = '\0';

			if ( num_bytes_received != NULL ) {
				*num_bytes_received = total_bytes_received;
			}

			if ( mx_socket->socket_flags & MXF_SOCKET_QUIET ) {
				error_code =
				    (MXE_NETWORK_CONNECTION_LOST | MXE_QUIET);
			} else {
				error_code = MXE_NETWORK_CONNECTION_LOST;
			}
			return mx_error( error_code, fname,
			    "Network connection closed unexpectedly." );
			break;
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			if ( num_bytes_received != NULL ) {
				*num_bytes_received = total_bytes_received;
			}

			switch( saved_errno ) {
			case ECONNRESET:
			case ECONNABORTED:
				return mx_error(
					MXE_NETWORK_CONNECTION_LOST, fname,
		"Network connection lost.  Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));

				break;
			default:
				return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Error receiving message body from remote host.  "
			"Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));

				break;
			}
			break;
		default:
			bytes_left -= bytes_received;

#if MX_SOCKET_DEBUG
			{
				long n;

				fprintf( stderr, "%s:", fname );

				for ( n = 0; n < bytes_received; n++ ) {
					fprintf( stderr, " %#02x",
						ptr[n] & 0xff );
				}

				fprintf( stderr, "   bytes_left = %ld\n",
						bytes_left );
			}
#endif

			if ( input_terminators == NULL ) {
				total_bytes_received += bytes_received;
			} else {
				/* Input terminators are _not_ NULL. */

				for ( i = 0; i < bytes_received; i++ ) {
				    total_bytes_received++;

				    if ( ptr[i] == 
					     terminators[num_terminators_seen] )
				    {
					num_terminators_seen++;
				    } else {
					num_terminators_seen = 0;
				    }
				    if ( num_terminators_seen >=
					     input_terminators_length_in_bytes )
				    {
					if ( num_bytes_received != NULL ) {
					    *num_bytes_received =
							total_bytes_received;
					}

					if ( i >= ( bytes_received - 1 ) ) {
					    return MX_SUCCESSFUL_RESULT;
					} else {
					    return mx_error(
						MXE_LIMIT_WAS_EXCEEDED, fname,
		"%ld unread bytes were lost after the input terminators.",
						bytes_received - 1 - i );
					}
				    }
				}
			}

			ptr += bytes_received;
			break;
		}
	}

	if ( num_bytes_received != NULL ) {
		*num_bytes_received = total_bytes_received;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_socket_putline( MX_SOCKET *mx_socket, char *buffer, char *line_terminators )
{
	mx_status_type mx_status;

	/* Send the buffer. */

	mx_status = mx_socket_send( mx_socket, buffer, strlen( buffer ) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the line terminators. */

	if ( line_terminators != NULL ) {
		mx_status = mx_socket_send( mx_socket, line_terminators,
						strlen(line_terminators) );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_socket_getline( MX_SOCKET *mx_socket, char *buffer, size_t buffer_length,
					char *line_terminators )
{
	mx_status_type mx_status;
	size_t i, string_length, line_terminator_length, num_bytes_received;

	*buffer = '\0';

	if ( line_terminators == NULL ) {
		line_terminator_length = 0;
	} else {
		line_terminator_length = strlen( line_terminators );
	}

	mx_status = mx_socket_receive( mx_socket,
					buffer, buffer_length,
					&num_bytes_received,
					line_terminators,
					line_terminator_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Strip off the line terminators. */

	string_length = num_bytes_received;

	for (i = string_length - line_terminator_length; i < string_length; i++)
	{
		buffer[i] = '\0';
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_socket_num_input_bytes_available( MX_SOCKET *mx_socket,
				long *num_input_bytes_available )
{
	static const char fname[] = "mx_socket_num_input_bytes_available()";

	int num_fds, select_status, socket_errno;
	struct timeval timeout;
	char *error_string;

#if HAVE_FD_SET
	fd_set read_mask;
#else
	long read_mask;
#endif

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}
	if ( num_input_bytes_available == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_input_bytes_available pointer passed was NULL." );
	}

	/* First, we check with select().  select() is capable of detecting
	 * whether or not the remote server connection has gone away.
	 */

#if defined( OS_WIN32 )
	num_fds = -1;		/* Win32 does not really use this argument. */
#else
	num_fds = 1 + mx_socket->socket_fd;
#endif

#if HAVE_FD_SET
	FD_ZERO( &read_mask );
	FD_SET( (mx_socket->socket_fd), &read_mask );
#else
	read_mask = 1 << (mx_socket->socket_fd);
#endif
	/* Set the timeout to zero so that we do not block. */

	timeout.tv_usec = 0;
	timeout.tv_sec = 0;

	/* Do the test. */

	select_status = select( num_fds, &read_mask, NULL, NULL, &timeout );

	socket_errno = mx_socket_check_error_status(
		&select_status, MXF_SOCKCHK_INVALID, &error_string );

	if ( socket_errno != 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"An error occurred while checking socket %d for input.  "
		"Errno = %d, error_message = '%s'",
			mx_socket->socket_fd, socket_errno,
			mx_socket_strerror( socket_errno ) );
	}

	if ( select_status == 0 ) {
		*num_input_bytes_available = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	/* select() can only tell you whether or not at least one character
	 * is available to be read.  It cannot tell you how many characters
	 * are waiting to be read.
	 */

#if HAVE_FIONREAD_FOR_SOCKETS
	/* If FIONREAD is available, we can use that to determine how
	 * many bytes are available.
	 */

	socket_errno = mx_socket_ioctl( mx_socket, FIONREAD,
					num_input_bytes_available );

	if ( socket_errno != 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"FIONREAD ioctl failed for socket %d.  "
			"Errno = %d, error message = '%s'",
			mx_socket->socket_fd, socket_errno,
			mx_socket_strerror( socket_errno ) );
	}

	/* If select() returned a non-zero status, but FIONREAD said that
	 * zero bytes were available, then we assume that the socket has
	 * had an error.  The most likely error is that the remote server
	 * has disconnected.
	 */

	if ( *num_input_bytes_available == 0 ) {
		long mask, error_code;

		mask = MXF_SOCKET_QUIET | MXF_SOCKET_QUIET_CONNECTION;

		if ( mx_socket->socket_flags & mask ) {
			error_code = MXE_NETWORK_CONNECTION_LOST | MXE_QUIET;
		} else {
			error_code = MXE_NETWORK_CONNECTION_LOST;
		}

		return mx_error( error_code, fname,
		"An error occurred while checking socket %d for input.",
			mx_socket->socket_fd );
	}
#else
	/* If FIONREAD is not available, then we can only safely claim
	 * that 1 input byte is available.  This is inefficient, but safe.
	 */

	*num_input_bytes_available = 1;
#endif

	return MX_SUCCESSFUL_RESULT;
}

#define MXU_SOCKET_DISCARD_BUFFER_LENGTH	1000

MX_EXPORT mx_status_type
mx_socket_discard_unread_input( MX_SOCKET *mx_socket )
{
	static const char fname[] = "mx_socket_discard_unread_input()";

	char discard_buffer[ MXU_SOCKET_DISCARD_BUFFER_LENGTH ];
	unsigned long i, max_attempts, wait_ms, num_chars;
	unsigned long j, k, num_blocks, remainder;
	long num_input_bytes_available;
	mx_status_type mx_status;

	k = 0;	/* To avoid an unused variable warning. */

	/* If input is available, read until there is no more input.
	 * If we do this to a socket that is constantly generating
	 * output, then we will never get to the end.  Thus, we have
	 * to build a timeout into the function.
	 */

	max_attempts = 10000;
	wait_ms = 5;

	num_chars = 0;

	for ( i = 0; i < max_attempts; i++ ) {
		mx_status = mx_socket_num_input_bytes_available( mx_socket,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_input_bytes_available == 0 ) {
			break;    /* No input available, so the for() loop. */
		}

		num_chars += num_input_bytes_available;

		/* Discard the available characters by reading them. */

		num_blocks = num_input_bytes_available 
					/ MXU_SOCKET_DISCARD_BUFFER_LENGTH;

		remainder = num_input_bytes_available
					% MXU_SOCKET_DISCARD_BUFFER_LENGTH;

		for ( j = 0; j < num_blocks; j++ ) {
			mx_status = mx_socket_receive( mx_socket,
					discard_buffer,
					MXU_SOCKET_DISCARD_BUFFER_LENGTH,
					NULL, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
#if MX_SOCKET_DEBUG
		fprintf( stderr, "%s:", fname );

		for ( k = 0; k < MXU_SOCKET_DISCARD_BUFFER_LENGTH; k++ ) {
			fprintf( stderr, " %#02x", discard_buffer[k] &0xff );
		}

		fprintf( stderr, "\n" );
#endif
		if ( remainder > 0 ) {
			mx_status = mx_socket_receive( mx_socket,
					discard_buffer,
					remainder,
					NULL, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
#if MX_SOCKET_DEBUG
			fprintf( stderr, "%s:", fname );

			for ( k = 0; k < remainder; k++ ) {
				fprintf( stderr,
					" %#02x", discard_buffer[k] &0xff );
			}

			fprintf( stderr, "\n" );
#endif

		}

		mx_msleep( wait_ms );
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"%lu characters were read while waiting %g seconds for "
		"the input buffer of socket %d to empty.  "
		"Perhaps this device continuously generates output?",
			num_chars, 0.001 * (double) ( max_attempts * wait_ms ),
			mx_socket->socket_fd );
	}

	return MX_SUCCESSFUL_RESULT;
}

/********** Microsoft Win32 specific support **********/

#if defined( OS_WIN32 )

static void
mx_winsock_shutdown( void )
{
	int status, saved_errno;

	status = WSACleanup();

	if ( status != 0 ) {
		saved_errno = WSAGetLastError();

		fprintf(stderr,
		"WSACleanup() failed with error code %d, reason = '%s'\n",
			saved_errno, mx_socket_strerror( saved_errno ) );
	}
	return;
}

static mx_status_type
mx_winsock_initialize( void )
{
	static const char fname[] = "mx_winsock_initialize()";

	WORD version_requested;
	WSADATA winsock_data;
	int status;

#if defined(__BORLANDC__)
	/* Borland C emits a warning for the MAKEWORD() macro for some reason.*/

	version_requested = 0x0101;
#else
	version_requested = MAKEWORD( 1, 1 );
#endif

	status = WSAStartup( version_requested, &winsock_data );

	if ( status != 0 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Winsock initialization failed." );
	}

	if ( atexit( mx_winsock_shutdown ) != 0 ) {
		(void) WSACleanup();

		return mx_error( MXE_FUNCTION_FAILED, fname,
"Attempt to set up Winsock atexit() handler failed.  Winsock disabled." );
	}
	return MX_SUCCESSFUL_RESULT;
}

static char *
mx_winsock_strerror( int winsock_errno )
{
	int i;

	static struct {
		int error_code;
		char error_string[60];
	} winsock_error_messages[] = {

	{ WSAEINTR,		"Interrupted system call" },
	{ WSAEBADF,		"Bad file number" },
	{ WSAEACCES,		"Permission denied" },
	{ WSAEFAULT,		"Bad address" },
	{ WSAEINVAL,		"Invalid argument" },
	{ WSAEMFILE,		"Too many open files" },
	{ WSAEWOULDBLOCK,	"Operation would block" },
	{ WSAEINPROGRESS,	"Operation now in progress" },
	{ WSAEALREADY,		"Operation already in progress" },
	{ WSAENOTSOCK,		"Socket operation on nonsocket" },
	{ WSAEDESTADDRREQ,	"Destination address required" },
	{ WSAEMSGSIZE,		"Message too long" },
	{ WSAEPROTOTYPE,	"Protocol wrong type for socket" },
	{ WSAENOPROTOOPT,	"Bad protocol option" },
	{ WSAEPROTONOSUPPORT,	"Protocol not supported" },
	{ WSAESOCKTNOSUPPORT,	"Socket type not supported" },
	{ WSAEOPNOTSUPP,	"Operation not supported on socket" },
	{ WSAEPFNOSUPPORT,	"Protocol family not supported" },
	{ WSAEAFNOSUPPORT, "Address family not supported by protocol family" },
	{ WSAEADDRINUSE,	"Address already in use" },
	{ WSAEADDRNOTAVAIL,	"Cannot assign requested address" },
	{ WSAENETDOWN,		"Network is down" },
	{ WSAENETUNREACH,	"Network is unreachable" },
	{ WSAENETRESET,		"Net dropped connection or reset" },
	{ WSAECONNABORTED,	"Software caused connection abort" },
	{ WSAECONNRESET,	"Connection reset by peer" },
	{ WSAENOBUFS,		"No buffer space available" },
	{ WSAEISCONN,		"Socket is already connected" },
	{ WSAENOTCONN,		"Socket is not connected" },
	{ WSAESHUTDOWN,		"Cannot send after socket shutdown" },
	{ WSAETOOMANYREFS,	"Too many references, cannot splice" },
	{ WSAETIMEDOUT,		"Connection timed out" },
	{ WSAECONNREFUSED,	"Connection refused" },
	{ WSAELOOP,		"Too many levels of symbolic link" },
	{ WSAENAMETOOLONG,	"File name too long" },
	{ WSAEHOSTDOWN,		"Host is down" },
	{ WSAEHOSTUNREACH,	"No route to host" },
	{ WSAENOTEMPTY,		"Directory is not empty" },
	{ WSAEPROCLIM,		"Too many processes" },
	{ WSAEUSERS,		"Too many users" },
	{ WSAEDQUOT,		"Disk quota exceeded" },
	{ WSAESTALE,		"Stale NFS file handle" },
	{ WSASYSNOTREADY,	"Network subsystem is unavailable" },
	{ WSAVERNOTSUPPORTED,	"Winsock DLL version out of range" },
	{ WSANOTINITIALISED,	"Successful WSAStartup() not yet performed" },
	{ WSAEREMOTE,		"Too many levels of remote in path" },
	{ WSAHOST_NOT_FOUND,	"Host not found" },
	{ WSATRY_AGAIN,		"Nonauthoritative host not found" },
	{ WSANO_RECOVERY,	"Nonrecoverable error" },
	{ WSANO_DATA,	    "Valid name, no data record of requested type" },
	{ WSANO_ADDRESS,	"No address, look for MX record" }
	};

	static int num_winsock_error_messages = sizeof(winsock_error_messages)
				/ sizeof(winsock_error_messages[0]);

	static char unrecognized[] = "Unrecognized Winsock error code";

	for ( i = 0; i < num_winsock_error_messages; i++ ) {

		if ( winsock_errno == winsock_error_messages[i].error_code ) {

			return &(winsock_error_messages[i].error_string[0]);
		}
	}
	return &(unrecognized[0]);
}

#endif /* OS_WIN32 */

/********** DJGPP specific support **********/

#if defined( OS_DJGPP )

static mx_status_type
mx_watt32_initialize( void )
{
#if 0
	dbug_init();
#endif

	sock_init();

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_DJGPP */

/********** eCos specific support **********/

#if defined( OS_ECOS )

static mx_status_type
mx_ecos_net_initialize( void )
{
	init_all_network_interfaces();

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_ECOS */

#endif /* HAVE_TCPIP */

