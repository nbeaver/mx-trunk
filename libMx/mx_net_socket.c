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
 * Copyright 1999-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE		FALSE

#define MX_NET_SOCKET_DEBUG_IO_PERFORMANCE		FALSE

/* Oct. 11, 2006 - For now it is best to leave the following define
 * set to TRUE, so that any such errors will not be hidden.  Currently,
 * this seems to only be an issue on Solaris.  (W. Lavender)
 */

#define MX_WARN_ABOUT_EWOULDBLOCK_IN_BLOCKING_MODE	FALSE

#include <stdio.h>

#include "mx_osdef.h"

#if HAVE_TCPIP

#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_clock.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_net_socket.h"

#if MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE || MX_NET_SOCKET_DEBUG_IO_PERFORMANCE
#include "mx_hrt_debug.h"
#endif

MX_EXPORT mx_status_type
mx_network_socket_receive_message( MX_SOCKET *mx_socket,
				double timeout,
				MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	static const char fname[] = "mx_network_socket_receive_message()";

	uint32_t *header;
	char *ptr;
	int saved_errno, comparison;
	mx_bool_type is_non_blocking, no_timeout;
	MX_CLOCK_TICK timeout_interval, current_time, timeout_time;
	int i, bytes_received, initial_recv_length;
	uint32_t bytes_left;
	uint32_t magic_value, header_length, message_length, total_length;
	mx_status_type mx_status;

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
	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );
	}

	if ( message_buffer->buffer_length
				< MXU_NETWORK_HEADER_LENGTH + 1 )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"TCP/IP receive buffer length (%lu) is too short.  Minimum allowed = %ld",
			(unsigned long) message_buffer->buffer_length,
			(long) MXU_NETWORK_HEADER_LENGTH + 1L );
	}

	header = message_buffer->u.uint32_buffer;

	/* Overwrite the header, just to make sure nothing is left over
	 * from a previous call to mx_network_socket_receive_message().
	 */

	for ( i = 0; i < MXU_NETWORK_NUM_HEADER_VALUES; i++ ) {
		header[i] = mx_htonl( 0L );
	}

	/* Is this socket in non-blocking mode? */

	mx_status = mx_socket_get_non_blocking_mode( mx_socket,
							&is_non_blocking );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, compute the timeout time for this message. */

	if ( timeout < 0.0 ) {
		no_timeout = TRUE;
	} else {
		no_timeout = FALSE;

		timeout_interval = mx_convert_seconds_to_clock_ticks( timeout );

		current_time = mx_current_clock_tick();

		timeout_time = mx_add_clock_ticks( current_time,
							timeout_interval );
	}

	/* Read the first three network longs so that we can figure
	 * out how long the rest of the message is.
	 */

	ptr = message_buffer->u.char_buffer;

	initial_recv_length = 3 * sizeof( uint32_t );

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
	"First 3 longs: bytes_left = %ld, bytes_received = %d, n = %ld",
			(long) bytes_left, bytes_received, n );
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

#if MX_WARN_ABOUT_EWOULDBLOCK_IN_BLOCKING_MODE
				if ( is_non_blocking == FALSE ) {
					return mx_error_quiet(
					MXE_NETWORK_IO_ERROR, fname,
				"A call to recv() for MX socket %d returned an "
				"error code warning that the call would block "
				"even though we are in blocking mode.  "
				"This should not happen.",
						mx_socket->socket_fd );
				}
#endif /* MX_WARN_ABOUT_EWOULDBLOCK_IN_BLOCKING_MODE */

				if ( no_timeout ) {
					/* In this case, we are never
					 * supposed to time out, so
					 * go back to the top of the
					 * while() loop and try again.
					 */

					continue;
				}

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

	magic_value    = mx_ntohl( header[ MX_NETWORK_MAGIC ] );
	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );

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

	/* If the message is too long to fit into the current buffer,
	 * increase the size of the buffer.
	 */

	total_length = header_length + message_length;

#if 0
	MX_DEBUG(-2,("%s: #1 message_buffer = %p, header = %p, ptr = %p",
		fname, message_buffer, header, ptr));
#endif

	if ( total_length > message_buffer->buffer_length ) {

		mx_status = mx_reallocate_network_buffer( message_buffer,
								total_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Update some pointers. */

		header = message_buffer->u.uint32_buffer;

		ptr = message_buffer->u.char_buffer;

		ptr += initial_recv_length;
	}

#if 0
	MX_DEBUG(-2,("%s: #2 message_buffer = %p, header = %p, ptr = %p",
		fname, message_buffer, header, ptr));
#endif

	/* Receive the rest of the data. */

	bytes_left = total_length - initial_recv_length;

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
	"Rest of measurement: bytes_left = %ld, bytes_received = %d, n = %ld",
			(long) bytes_left, bytes_received, n );
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

#if MX_WARN_ABOUT_EWOULDBLOCK_IN_BLOCKING_MODE
				if ( is_non_blocking == FALSE ) {
					return mx_error_quiet(
					MXE_NETWORK_IO_ERROR, fname,
				"A call to recv() for MX socket %d returned an "
				"error code warning that the call would block "
				"even though we are in blocking mode.  "
				"This should not happen.",
						mx_socket->socket_fd );

				}
#endif /* MX_WARN_ABOUT_EWOULDBLOCK_IN_BLOCKING_MODE */

				if ( no_timeout ) {
					/* In this case, we are never
					 * supposed to time out, so
					 * go back to the top of the
					 * while() loop and try again.
					 */

					continue;
				}

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
				MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	static const char fname[] = "mx_network_socket_send_message()";

	uint32_t *header;
	char *ptr;
	int saved_errno, comparison;
	mx_bool_type is_non_blocking, no_timeout;
	MX_CLOCK_TICK timeout_interval, current_time, timeout_time;
	int bytes_left, bytes_sent;
	uint32_t magic_value, header_length, message_length;
	mx_status_type mx_status;

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
	if ( message_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );
	}

	header = message_buffer->u.uint32_buffer;
	ptr    = message_buffer->u.char_buffer;

	magic_value    = mx_ntohl( header[ MX_NETWORK_MAGIC ] );
	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );

#if 0
	MX_DEBUG(-2,("%s: magic_value    = %#lx", fname, magic_value));
	MX_DEBUG(-2,("%s: header_length  = %ld", fname, header_length));
	MX_DEBUG(-2,("%s: message_length = %ld", fname, message_length));

	MX_DEBUG(-2,("%s: message type   = %ld", fname, 
				mx_ntohl( header[MX_NETWORK_MESSAGE_TYPE] ) ));
	MX_DEBUG(-2,("%s: status code    = %ld", fname,
				mx_ntohl( header[MX_NETWORK_STATUS_CODE] ) ));
#endif

	bytes_left = header_length + message_length;

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

	/* Is this socket in non-blocking mode? */

	mx_status = mx_socket_get_non_blocking_mode( mx_socket,
							&is_non_blocking );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, compute the timeout time for this message. */

	if ( timeout < 0.0 ) {
		no_timeout = TRUE;
	} else {
		no_timeout = FALSE;

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
			"Sending: bytes_left = %ld, bytes_sent = %d, n = %ld",
			(long) bytes_left, bytes_sent, n );
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
				if ( is_non_blocking == FALSE ) {

#if MX_WARN_ABOUT_EWOULDBLOCK_IN_BLOCKING_MODE

					return mx_error_quiet(
					MXE_NETWORK_IO_ERROR, fname,
				"A call to send() for MX socket %d returned an "
				"error code warning that the call would block "
				"even though we are in blocking mode.  "
				"This should not happen.",
						mx_socket->socket_fd );

#endif /* MX_WARN_ABOUT_EWOULDBLOCK_IN_BLOCKING_MODE */

				} else {
					if ( no_timeout ) {
						/* In this case, we are never
						 * supposed to time out, so
						 * go back to the top of the
						 * while() loop and try again.
						 */

						continue;
					}

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
	static const char fname[] = "mx_network_socket_send_error_message()";

	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	uint32_t *header;
	char     *ptr;
	uint32_t header_length, message_length, total_length;
	mx_status_type mx_status;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}

	header_length = MXU_NETWORK_HEADER_LENGTH;

	message_length = (uint32_t) ( 1 + strlen( error_message.message ) );

	total_length = header_length + message_length;

	mx_status = mx_allocate_network_buffer( &message_buffer, total_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	message_buffer->data_format = MX_NETWORK_DATAFMT_ASCII;

	header = message_buffer->u.uint32_buffer;
	ptr    = message_buffer->u.char_buffer;

	ptr   += header_length;

	strlcpy( ptr, error_message.message, message_length );

	header[ MX_NETWORK_MAGIC ] = mx_htonl( MX_NETWORK_MAGIC_VALUE );

	header[ MX_NETWORK_HEADER_LENGTH ] = mx_htonl( header_length );

	header[ MX_NETWORK_MESSAGE_LENGTH ] = mx_htonl( message_length );

	header[ MX_NETWORK_STATUS_CODE ] = mx_htonl( error_message.code );

	header[ MX_NETWORK_MESSAGE_TYPE ] = mx_htonl( return_message_type );

	header[ MX_NETWORK_MESSAGE_ID ] = mx_htonl( 0 );

	/* Send back the error message. */

	mx_status = mx_network_socket_send_message(
				mx_socket, 0, message_buffer );

	/* FIXME: When we start using message queues, we will need to
	 * change this so that it is no longer necessary to immediately
	 * free the buffer.
	 */

	mx_free_network_buffer( message_buffer );

	return mx_status;
}

#endif /* HAVE_TCPIP */
