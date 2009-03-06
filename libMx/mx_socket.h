/*
 * Name:    mx_socket.h 
 *
 * Purpose: BSD sockets and Winsock interface functions.
 *
 * Author:  William Lavender
 *
 * This header file includes all the defines and prototypes normally needed
 * by a socket-based program.  Thus, to use sockets, it is only necessary to
 * use #include "mx_socket.h" rather than including all the individual headers
 * separately.  This also promotes portability since different operating
 * systems have different sets of header files that they want to be included
 * in order to implement socket functionality.
 *
 *----------------------------------------------------------------------
 *
 * Copyright 1999-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SOCKET_H__
#define __MX_SOCKET_H__

#include "mx_osdef.h"

/* Include standard socket defines and prototypes. */

#if defined( OS_WIN32 )
#  include <winsock.h>

#elif defined( OS_VMS ) && defined( HAVE_WINTCP )

#  include "vms_twg.h"		/* For WIN/TCP from The Wollogong Group. */

#elif defined( OS_ECOS )

#  include <network.h>
#  include <arpa/inet.h>

#else

#  if defined( OS_VXWORKS )
#     include <sockLib.h>
#     include <ioLib.h>
#     include <hostLib.h>
#  else
#     include <sys/time.h>
#  endif

#  if defined( OS_DJGPP )
      extern int sock_init( void );
      extern int h_errno;
#  endif

#  include <time.h>
#  include <netdb.h>
#  include <fcntl.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <sys/ioctl.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>

#  if defined( OS_HPUX )
      extern int h_errno;
#  endif

#endif

#if HAVE_UNIX_DOMAIN_SOCKETS
#  include <sys/un.h>
#endif

#include "mx_stdint.h"

/* Make the rest of the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Define primary socket data structures. */

/* ..._FD means 'file descriptor' in case you were wondering. */

/* ===== Unix BSD sockets support ===== */

#if defined( OS_UNIX ) || defined( OS_CYGWIN ) || defined( OS_VMS ) \
 || defined( OS_DJGPP ) || defined( OS_RTEMS ) || defined( OS_VXWORKS ) \
 || defined( OS_ECOS )

#  define MX_SOCKET_FD			int
#  define MX_INVALID_SOCKET_FD		(-1)
#  define MX_SOCKET_ERROR		(-1)

#  define mx_socket_get_last_error()	(errno)

/* ===== Microsoft Win32 Winsock support ===== */

#elif defined( OS_WIN32 )

#  define MX_SOCKET_FD			SOCKET
#  define MX_INVALID_SOCKET_FD		INVALID_SOCKET
#  define MX_SOCKET_ERROR		SOCKET_ERROR

#  define mx_socket_get_last_error()	WSAGetLastError()

/* ============================ */

#else
#  error "Socket definitions not configured for this operating system."
#endif


typedef struct {
	MX_SOCKET_FD socket_fd;
	unsigned long socket_flags;
	mx_bool_type is_non_blocking;
} MX_SOCKET;

/* ***** Various define's ***** */

#define MX_SOCKET_DEFAULT_BUFFER_SIZE		0

/* Redefine some error codes for Win32. */

#if defined( OS_WIN32 )

#  define ECONNRESET	WSAECONNRESET
#  define ECONNABORTED  WSAECONNABORTED
#  define ENOTCONN	WSAENOTCONN
#  define EWOULDBLOCK	WSAEWOULDBLOCK

#endif

/* mx_socklen_t is used by the third argument of accept(). */

#if defined(OS_VMS)
#  define mx_socklen_t		unsigned int

#elif defined(__BORLANDC__)
#  if defined(OS_LINUX)
#    define mx_socklen_t	socklen_t
#  else
#    define mx_socklen_t	int
#  endif

#elif defined(__INTEL_COMPILER)
#  define mx_socklen_t		socklen_t

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_BSD) \
	|| defined(OS_HPUX)
#  define mx_socklen_t		socklen_t

#else
#  define mx_socklen_t		int
#endif

/* Macros for use with MX network headers. */

#if ( defined(OS_IRIX) && (MX_WORDSIZE == 64) )

#  define mx_htonl(x)	((uint32_t) htonl(x))
#  define mx_ntohl(x)	ntohl(x)

#else

#  define mx_htonl(x)	htonl(x)
#  define mx_ntohl(x)	ntohl(x)

#endif

/* The following are flags for the various mx_..._socket_open_as_...()
 * functions below.
 */

#define MXF_SOCKET_DISABLE_NAGLE_ALGORITHM	0x1
#define MXF_SOCKET_QUIET			0x2
#define MXF_SOCKET_QUIET_CONNECTION		0x4

/* The following defines are for the second argument to the function
 * mx_socket_check_error_status().
 */

#define MXF_SOCKCHK_INVALID	(-1)
#define MXF_SOCKCHK_ZERO	0
#define MXF_SOCKCHK_NULL	1

/* ***** General function definitions ***** */

MX_API mx_status_type mx_socket_initialize( void );

MX_API mx_status_type mx_gethostname( char *name, size_t maximum_length );

MX_API mx_status_type mx_tcp_socket_open_as_client( MX_SOCKET **client_socket,
						char *hostname,
						long port_number,
						unsigned long socket_flags,
						size_t buffer_size );

MX_API mx_status_type mx_tcp_socket_open_as_server( MX_SOCKET **server_socket,
						long port_number,
						unsigned long socket_flags,
						size_t buffer_size );

MX_API mx_status_type mx_unix_socket_open_as_client( MX_SOCKET **client_socket,
						char *pathname,
						unsigned long socket_flags,
						size_t buffer_size );

MX_API mx_status_type mx_unix_socket_open_as_server( MX_SOCKET **server_socket,
						char *pathname,
						unsigned long socket_flags,
						size_t buffer_size );

MX_API mx_status_type mx_socket_close( MX_SOCKET *mx_socket );

MX_API int mx_socket_ioctl( MX_SOCKET *mx_socket,
					int ioctl_type,
					void *ioctl_value );

MX_API mx_status_type mx_socket_get_non_blocking_mode( MX_SOCKET *mx_socket,
					mx_bool_type *non_blocking_flag );

MX_API mx_status_type mx_socket_set_non_blocking_mode( MX_SOCKET *mx_socket,
					mx_bool_type non_blocking_flag );

MX_API mx_bool_type mx_socket_is_open( MX_SOCKET *mx_socket );

MX_API void mx_socket_mark_as_closed( MX_SOCKET *mx_socket );

MX_API char *mx_socket_strerror( int saved_errno );

MX_API int  mx_socket_check_error_status( void *value_to_check,
						int type_of_check,
						char **error_string );

MX_API mx_status_type mx_socket_get_inet_address( const char *hostname,
						unsigned long *inet_address );

MX_API mx_status_type mx_socket_send( MX_SOCKET *mx_socket,
				void *message_buffer,
				size_t message_length_in_bytes );

MX_API mx_status_type mx_socket_receive( MX_SOCKET *mx_socket,
				void *message_buffer,
				size_t message_length_in_bytes,
				size_t *num_bytes_received,
				void *input_terminators,
				size_t input_terminators_length_in_bytes );

MX_API mx_status_type mx_socket_putline( MX_SOCKET *mx_socket,
					char *buffer,
					char *line_terminators );

MX_API mx_status_type mx_socket_getline( MX_SOCKET *mx_socket,
					char *buffer,
					size_t buffer_length,
					char *line_terminators );

MX_API mx_status_type mx_socket_num_input_bytes_available(
					MX_SOCKET *mx_socket,
					long *num_input_bytes_available );

MX_API mx_status_type mx_socket_discard_unread_input( MX_SOCKET *mx_socket );

MX_API mx_status_type mx_socket_wait_for_event( MX_SOCKET *mx_socket,
						double timeout_in_seconds );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SOCKET_H__ */

