/*
 * Name:    mx_net_socket.c
 *
 * Purpose: Functions for managing MX protocol connections that run on
 *          top of TCP network sockets.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE	TRUE

#define MX_NET_SOCKET_DEBUG_IO_PERFORMANCE	FALSE

#include <stdio.h>

#include "mx_osdef.h"

#if HAVE_TCPIP

#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_clock.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_net_socket.h"

#if MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE
#include "mx_hrt_debug.h"
#endif

MX_EXPORT mx_status_type
mx_network_socket_receive_message( MX_SOCKET *mx_socket,
					double timeout,
					unsigned long buffer_length,
					void *buffer )
{
	const char fname[] = "mx_network_socket_receive_message()";

	mx_uint32_type *header;
	char *ptr;
	int saved_errno, use_non_blocking_mode, comparison;
	MX_CLOCK_TICK timeout_interval, current_time, timeout_time;
	long i, bytes_left, bytes_received, initial_recv_length;
	mx_uint32_type magic_value, header_length, message_length;

#if MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE
	MX_HRT_TIMING total_measurement;
#endif

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
	MX_HRT_TIMING io_measurement;
	long n;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}
	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	if ( buffer_length < MX_NETWORK_HEADER_LENGTH_VALUE + 1 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"TCP/IP receive buffer length (%ld) is too short.  Minimum allowed = %ld",
			buffer_length,
			(long) MX_NETWORK_HEADER_LENGTH_VALUE + 1L );
	}

	header = buffer;

	/* Overwrite the header, just to make sure nothing is left over
	 * from a previous call to mx_network_socket_receive_message().
	 */

	for ( i = 0; i < MX_NETWORK_HEADER_LENGTH_VALUE; i++ ) {
		header[i] = htonl( 0L );
	}

	/* If requested, compute the timeout time for this message. */

	if ( timeout < 0.0 ) {
		use_non_blocking_mode = FALSE;
	} else {
		use_non_blocking_mode = TRUE;

		timeout_interval = mx_convert_seconds_to_clock_ticks( timeout );

		current_time = mx_current_clock_tick();

		timeout_time = mx_add_clock_ticks( current_time,
							timeout_interval );
	}

	/* Read the first three network longs so that we can figure
	 * out how long the rest of the message is.
	 */

	ptr = buffer;

	initial_recv_length = 3 * sizeof( mx_uint32_type );

	bytes_left = initial_recv_length;

#if MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE
	MX_HRT_START( total_measurement );
#endif

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
	n = -1;
#endif
	while( bytes_left > 0 ) {

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
		n++;
		MX_HRT_START( io_measurement );
#endif
		bytes_received = recv(mx_socket->socket_fd, ptr, bytes_left, 0);

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
		MX_HRT_END( io_measurement );

		MX_HRT_RESULTS( io_measurement, fname,
	"First 3 longs: bytes_left = %ld, bytes_received = %ld, n = %ld",
			bytes_left, bytes_received, n );
#endif

		switch( bytes_received ) {
		case 0:
			return mx_error_quiet( MXE_NETWORK_IO_ERROR, fname,
				"Network connection closed unexpectedly." );
			break;
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			switch( saved_errno ) {
			case ECONNRESET:
			case ECONNABORTED:
				return mx_error_quiet(
					MXE_NETWORK_CONNECTION_LOST, fname,
			"Connection lost.  Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));

				break;
			case EAGAIN:

#if ( EAGAIN != EWOULDBLOCK )
			case EWOULDBLOCK:
#endif
				if ( use_non_blocking_mode == FALSE ) {
					return mx_error_quiet(
					MXE_NETWORK_IO_ERROR, fname,
				"A call to recv() for MX socket %d returned an "
				"error code warning that the call would block "
				"even though we are in blocking mode.  "
				"This should not happen.",
						mx_socket->socket_fd );

				} else {
					current_time = mx_current_clock_tick();

					comparison = mx_compare_clock_ticks(
					    	current_time, timeout_time );

					if ( comparison < 0 ) {

						/* Have not timed out yet, so
						 * go back to the top of the
						 * while() loop and try again.
						 */

						continue;
					} else {
						return mx_error_quiet(
						MXE_TIMED_OUT, fname,
    "Timed out after waiting %g seconds to read from the MX network socket.",
							timeout );
					}
				}
				break;
			default:
				return mx_error_quiet( MXE_NETWORK_IO_ERROR,
						fname,
"Error receiving header from remote host.  Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));

				break;
			}
			break;
		default:
			bytes_left -= bytes_received;
			ptr += bytes_received;
			break;
		}
	}

	magic_value    = ntohl( header[ MX_NETWORK_MAGIC ] );
	header_length  = ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );

#if 0
	MX_DEBUG(-2,("%s: magic_value    = %#lx", fname, magic_value));
	MX_DEBUG(-2,("%s: header_length  = %ld", fname, header_length));
	MX_DEBUG(-2,("%s: message_length = %ld", fname, message_length));
#endif

	if ( magic_value != MX_NETWORK_MAGIC_VALUE ) {
		return mx_error_quiet( MXE_NETWORK_IO_ERROR, fname,
		"Wrong magic number %#lx in received message.",
			(unsigned long) magic_value );
	}

	/* Receive the rest of the data. */

	bytes_left = (long)
		(header_length + message_length - initial_recv_length);

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
	n = -1;
#endif
	while( bytes_left > 0 ) {

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
		n++;
		MX_HRT_START( io_measurement );
#endif
		bytes_received = recv(mx_socket->socket_fd, ptr, bytes_left, 0);

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
		MX_HRT_END( io_measurement );

		MX_HRT_RESULTS( io_measurement, fname,
	"Rest of measurement: bytes_left = %ld, bytes_received = %ld, n = %ld",
			bytes_left, bytes_received, n );
#endif

		switch( bytes_received ) {
		case 0:
			return mx_error_quiet( MXE_NETWORK_IO_ERROR, fname,
				"Network connection closed unexpectedly." );
			break;
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			switch( saved_errno ) {
			case ECONNRESET:
			case ECONNABORTED:
				return mx_error_quiet(
					MXE_NETWORK_CONNECTION_LOST, fname,
			"Connection lost.  Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));

				break;
			case EAGAIN:

#if ( EAGAIN != EWOULDBLOCK )
			case EWOULDBLOCK:
#endif
				if ( use_non_blocking_mode == FALSE ) {
					return mx_error_quiet(
					MXE_NETWORK_IO_ERROR, fname,
				"A call to recv() for MX socket %d returned an "
				"error code warning that the call would block "
				"even though we are in blocking mode.  "
				"This should not happen.",
						mx_socket->socket_fd );

				} else {
					current_time = mx_current_clock_tick();

					comparison = mx_compare_clock_ticks(
					    	current_time, timeout_time );

					if ( comparison < 0 ) {

						/* Have not timed out yet, so
						 * go back to the top of the
						 * while() loop and try again.
						 */

						continue;
					} else {
						return mx_error_quiet(
						MXE_TIMED_OUT, fname,
    "Timed out after waiting %g seconds to read from the MX network socket.",
							timeout );
					}
				}
				break;
			default:
				return mx_error_quiet( MXE_NETWORK_IO_ERROR,
						fname,
			"Error receiving message body from remote host.  "
			"Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));

				break;
			}
			break;
		default:
			bytes_left -= bytes_received;
			ptr += bytes_received;
			break;
		}
	}

#if MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE
	MX_HRT_END( total_measurement );

	MX_HRT_RESULTS( total_measurement, fname, "Total duration" );
#endif

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mx_network_socket_send_message( MX_SOCKET *mx_socket,
				double timeout,
				void *buffer )
{
	const char fname[] = "mx_network_socket_send_message()";

	mx_uint32_type *header;
	char *ptr;
	int saved_errno, use_non_blocking_mode, comparison;
	MX_CLOCK_TICK timeout_interval, current_time, timeout_time;
	long bytes_left, bytes_sent;
	mx_uint32_type magic_value, header_length, message_length;

#if MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE
	MX_HRT_TIMING total_measurement;
#endif

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
	MX_HRT_TIMING io_measurement;
	long n;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}
	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );
	}

	header = buffer;
	ptr = buffer;

	magic_value    = ntohl( header[ MX_NETWORK_MAGIC ] );
	header_length  = ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );

#if 0
	MX_DEBUG(-2,("%s: magic_value    = %#lx", fname, magic_value));
	MX_DEBUG(-2,("%s: header_length  = %ld", fname, header_length));
	MX_DEBUG(-2,("%s: message_length = %ld", fname, message_length));

	MX_DEBUG(-2,("%s: message type   = %ld", fname, 
				ntohl( header[MX_NETWORK_MESSAGE_TYPE] ) ));
	MX_DEBUG(-2,("%s: status code    = %ld", fname,
				ntohl( header[MX_NETWORK_STATUS_CODE] ) ));
#endif

	bytes_left = (long) (header_length + message_length);

#if 0
        {
                long i;                                   
                unsigned char c;                                            
                        
                for ( i = 0; i < bytes_left; i++ ) {
                        c = ptr[i];
        
                        if ( isprint(c) ) {                  
                                MX_DEBUG(-2,("buffer[%ld] = %02x (%c)",
                                                i, c, c));
                        } else {
                                MX_DEBUG(-2,("buffer[%ld] = %02x", i, c));
                        }                                    
                }                                                            
        }
#endif   

	/* If requested, compute the timeout time for this message. */

	if ( timeout < 0.0 ) {
		use_non_blocking_mode = FALSE;
	} else {
		use_non_blocking_mode = TRUE;

		timeout_interval = mx_convert_seconds_to_clock_ticks( timeout );

		current_time = mx_current_clock_tick();

		timeout_time = mx_add_clock_ticks( current_time,
							timeout_interval );
	}

#if MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE
	MX_HRT_START( total_measurement );
#endif

	/* Send the message. */

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
	n = -1;
#endif
	while( bytes_left > 0 ) {

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
		MX_HRT_START( io_measurement );
#endif
		bytes_sent = send( mx_socket->socket_fd, ptr, bytes_left, 0 );

#if MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
		n++;
		MX_HRT_END( io_measurement );

		MX_HRT_RESULTS( io_measurement, fname,
			"Sending: bytes_left = %ld, bytes_sent = %ld, n = %ld",
			bytes_left, bytes_sent, n );
#endif

		switch( bytes_sent ) {
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			switch( saved_errno ) {
			case ECONNRESET:
			case ECONNABORTED:
				return mx_error_quiet(
					MXE_NETWORK_CONNECTION_LOST, fname,
			"Connection lost.  Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));
				break;
			case EAGAIN:

#if ( EAGAIN != EWOULDBLOCK )
			case EWOULDBLOCK:
#endif
				if ( use_non_blocking_mode == FALSE ) {
					return mx_error_quiet(
					MXE_NETWORK_IO_ERROR, fname,
				"A call to send() for MX socket %d returned an "
				"error code warning that the call would block "
				"even though we are in blocking mode.  "
				"This should not happen.",
						mx_socket->socket_fd );

				} else {
					current_time = mx_current_clock_tick();

					comparison = mx_compare_clock_ticks(
					    	current_time, timeout_time );

					if ( comparison < 0 ) {

						/* Have not timed out yet, so
						 * go back to the top of the
						 * while() loop and try again.
						 */

						continue;
					} else {
						return mx_error_quiet(
						MXE_TIMED_OUT, fname,
	"Timed out after waiting %g seconds to write to MX network socket %d.",
							timeout,
							mx_socket->socket_fd );
					}
				}
				break;
			default:
				return mx_error_quiet( MXE_NETWORK_IO_ERROR,
						fname,
	"Error sending to remote host.  Errno = %d, error text = '%s'",
				saved_errno, mx_socket_strerror(saved_errno));
				break;
			}
			break;
		default:
			bytes_left -= bytes_sent;
			ptr += bytes_sent;
			break;
		}
	}

#if MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE
	MX_HRT_END( total_measurement );

	MX_HRT_RESULTS( total_measurement, fname, "Total duration" );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_network_socket_send_error_message( MX_SOCKET *mx_socket,
			long return_message_type,
			mx_status_type error_message )
{
	const char fname[] = "mx_network_socket_send_error_message()";

	void *malloc_ptr;
	char *send_buffer, *message;
	mx_uint32_type *header;
	mx_uint32_type header_length, message_length;
	mx_status_type status;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}

	header_length = MX_NETWORK_HEADER_LENGTH_VALUE;
	message_length = 1 + strlen( error_message.message );

	/* The output of malloc() is assigned to a void pointer here
	 * because assigning it to a char pointer and then casting
	 * that to an mx_uint32_type pointer below results in a warning
	 * message under GCC 2.8.1.
	 */

	malloc_ptr = malloc( header_length + message_length );

	if ( malloc_ptr == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating send buffer for MX error message.");
	}

	send_buffer = (char *) malloc_ptr;

	header = (mx_uint32_type *) malloc_ptr;

	message = send_buffer + header_length;

	strcpy( message, error_message.message );

	header[ MX_NETWORK_MAGIC ] = htonl( MX_NETWORK_MAGIC_VALUE );
	header[ MX_NETWORK_HEADER_LENGTH ] = htonl( header_length );
	header[ MX_NETWORK_MESSAGE_LENGTH ] = htonl( message_length );
	header[ MX_NETWORK_STATUS_CODE ] = htonl( error_message.code );
	header[ MX_NETWORK_MESSAGE_TYPE ] = htonl( return_message_type );

	/* Send back the error message. */

	status = mx_network_socket_send_message( mx_socket, 0, send_buffer );

	free(send_buffer);

	return status;
}

#endif /* HAVE_TCPIP */
