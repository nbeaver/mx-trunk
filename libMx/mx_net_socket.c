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
 * Copyright 1999-2008, 2015-2016, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_NET_SOCKET_DEBUG_TOTAL_PERFORMANCE		FALSE

#define MX_NET_SOCKET_DEBUG_IO_PERFORMANCE		FALSE

#define MX_NET_SOCKET_DEBUG_MESSAGES			FALSE

#include <stdio.h>

#include "mx_osdef.h"

#if HAVE_TCPIP

#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_clock_tick.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_net_socket.h"
#include "mx_process.h"
#include "mx_debugger.h"

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
	int bytes_received, initial_recv_length;
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

	memset( header, 0, MXU_NETWORK_HEADER_LENGTH );

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
			return mx_error(
			(MXE_NETWORK_IO_ERROR | MXE_QUIET), fname,
				"Network connection closed unexpectedly." );
			break;
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			switch( saved_errno ) {
			case ECONNRESET:
			case ECONNABORTED:
				return mx_error(
				(MXE_NETWORK_CONNECTION_LOST | MXE_QUIET),fname,
					"Connection lost.  "
					"Errno = %d, error text = '%s'",
					saved_errno,
					mx_socket_strerror(saved_errno) );

				break;
			case EAGAIN:

#if ( EAGAIN != EWOULDBLOCK )
			case EWOULDBLOCK:
#endif
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
					return mx_error(
					(MXE_TIMED_OUT | MXE_QUIET), fname,
					"Timed out after waiting %g seconds to "
					"read from the MX network socket.",
						timeout );
				}
				break;
			default:
				return mx_error(
				(MXE_NETWORK_IO_ERROR | MXE_QUIET), fname,
				"Error receiving header from remote host.  "
				"Errno = %d, error text = '%s'",
					saved_errno,
					mx_socket_strerror(saved_errno) );

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
		return mx_error( (MXE_NETWORK_IO_ERROR | MXE_QUIET), fname,
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
			return mx_error(
				(MXE_NETWORK_IO_ERROR | MXE_QUIET), fname,
				"Network connection closed unexpectedly." );
			break;
		case MX_SOCKET_ERROR:
			saved_errno = mx_socket_get_last_error();

			switch( saved_errno ) {
			case ECONNRESET:
			case ECONNABORTED:
				return mx_error(
				(MXE_NETWORK_CONNECTION_LOST | MXE_QUIET),fname,
				"Connection lost.  "
				"Errno = %d, error text = '%s'",
					saved_errno,
					mx_socket_strerror(saved_errno) );
				break;
			case EAGAIN:

#if ( EAGAIN != EWOULDBLOCK )
			case EWOULDBLOCK:
#endif
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
					return mx_error(
					(MXE_TIMED_OUT | MXE_QUIET), fname,
					"Timed out after waiting %g seconds to "
					"read from the MX network socket.",
						timeout );
				}
				break;
			default:
				return mx_error(
				(MXE_NETWORK_IO_ERROR | MXE_QUIET), fname,
				"Error receiving message body from "
				"remote host.  Errno = %d, error text = '%s'",
					saved_errno,
					mx_socket_strerror(saved_errno) );

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

#if MX_NET_SOCKET_DEBUG_MESSAGES
	MX_DEBUG(-2,("%s: Received message from socket %d <--",
		fname, mx_socket->socket_fd ));

	mx_network_display_message( message_buffer, NULL );
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

	MXW_UNUSED( magic_value );

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

#if MX_NET_SOCKET_DEBUG_MESSAGES
	MX_DEBUG(-2,("%s: Sending message to socket %d -->",
		fname, mx_socket->socket_fd ));

	mx_network_display_message( message_buffer, NULL );
#endif

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
				return mx_error(
				(MXE_NETWORK_CONNECTION_LOST| MXE_QUIET), fname,
				"Connection lost.  "
				"Errno = %d, error text = '%s'",
					saved_errno,
					mx_socket_strerror(saved_errno) );
				break;

			case EFAULT:
				/* If we get here the pointer for the message
				 * was invalid.  This should never happen.
				 * If it does happen, then the data structures
				 * for the connections are sufficiently 
				 * messed up that all we can do is to 
				 * unilaterally close the connection.
				 */

				mx_wait_for_debugger();

				mx_status = mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The data structures for the send buffer "
				"for socket %d are corrupted.  All we can "
				"do at this point is close the connection, "
				"since we cannot even send an error to the "
				"remote process.", mx_socket->socket_fd );

				return mx_status;
				break;

			case EAGAIN:

#if ( EAGAIN != EWOULDBLOCK )
			case EWOULDBLOCK:
#endif
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
					return mx_error(
					(MXE_TIMED_OUT | MXE_QUIET), fname,
					"Timed out after waiting %g seconds to "
					"write to MX network socket %d.",
						timeout,
						(int) mx_socket->socket_fd );
				}
				break;
			default:
				return mx_error(
				(MXE_NETWORK_IO_ERROR | MXE_QUIET), fname,
				"Error sending to remote host.  "
				"Errno = %d, error text = '%s'",
					saved_errno,
					mx_socket_strerror(saved_errno) );
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
			uint32_t message_id,
			unsigned long header_length,
			mx_bool_type debug_flag,
			long return_message_type,
			mx_status_type error_message )
{
	static const char fname[] = "mx_network_socket_send_error_message()";

	MX_SOCKET_HANDLER *socket_handler = NULL;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer = NULL;
	uint32_t *header = NULL;
	char     *ptr = NULL;
	uint32_t message_length, total_length;
	mx_status_type mx_status;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}

	socket_handler = mx_socket->socket_handler;

	if ( socket_handler == (MX_SOCKET_HANDLER *) NULL ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"MX_SOCKET structure %p (socket %ld) does not have an "
		"associated MX_SOCKET_HANDLER structure.  "
		"This function is only intended for use with sockets in "
		"MX network servers which have MX_SOCKET HANDLER structures.  "
		"Perhaps you are mistakenly trying to call it from "
		"an MX client?", mx_socket, (long) mx_socket->socket_fd );
	}

	if ( header_length == 0 ) {
		header_length = MXU_NETWORK_HEADER_LENGTH;
	}

	message_buffer = socket_handler->message_buffer;

	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {

		/* If the MX_SOCKET_HANDLER does not already have an
		 * MX_NETWORK_MESSAGE_BUFFER, then we allocate one
		 * right here.
		 */

		message_length = (uint32_t)
			( 1 + strlen( error_message.message ) );

		total_length = header_length + message_length;

		/* MX_NETWORK_MESSAGE_BUFFER objects are expected to have
		 * an internal message buffer array that is at least
		 * MXU_NETWORK_INITIAL_MESSAGE_BUFFER_LENGTH bytes in length.
		 */

		if ( total_length < MXU_NETWORK_INITIAL_MESSAGE_BUFFER_LENGTH )
		{
		    total_length = MXU_NETWORK_INITIAL_MESSAGE_BUFFER_LENGTH;
		}

		mx_status = mx_allocate_network_buffer( &message_buffer,
							NULL, socket_handler,
							total_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		socket_handler->message_buffer = message_buffer;
	} else {
		message_length = (uint32_t)
			( 1 + strlen( error_message.message ) );
	}

	/* FIXME: Why in the world were we setting the message buffer
	 * data format unconditionally to ASCII here?  (WML 2022-01-26)
	 */
#if 0
	message_buffer->data_format = MX_NETWORK_DATAFMT_ASCII;
#endif

	header = message_buffer->u.uint32_buffer;
	ptr    = message_buffer->u.char_buffer;

	ptr   += header_length;

	strlcpy( ptr, error_message.message, message_length );

	header[ MX_NETWORK_MAGIC ] = mx_htonl( MX_NETWORK_MAGIC_VALUE );

	header[ MX_NETWORK_HEADER_LENGTH ] = mx_htonl( header_length );

	header[ MX_NETWORK_MESSAGE_LENGTH ] = mx_htonl( message_length );

	header[ MX_NETWORK_MESSAGE_TYPE ] = mx_htonl( return_message_type );

	header[ MX_NETWORK_STATUS_CODE ] = mx_htonl( error_message.code );

	/* If the header is long enough, include the message id. */

	if ( header_length >= ((MX_NETWORK_MESSAGE_ID+1) * sizeof(uint32_t)) ) {
		header[ MX_NETWORK_DATA_TYPE ] = mx_htonl( MXFT_STRING );

		header[ MX_NETWORK_MESSAGE_ID ] = mx_htonl( message_id );
	}

#if MX_NET_SOCKET_DEBUG_MESSAGES
	if ( debug_flag ) {
		fprintf( stderr,
		"\nMX NET: Sending error code %s (%ld) to socket %d\n",
			mx_status_code_string( error_message.code ),
			error_message.code,
			mx_socket->socket_fd );

		mx_network_display_message( message_buffer, NULL,
				socket_handler->use_64bit_network_longs );
	}
#endif

	/* Send back the error message. */

	mx_status = mx_network_socket_send_message(
				mx_socket, 0, message_buffer );

	/* FIXME: I would imagine that freeing this network buffer here
	 * would not be a good idea in current versions of MX.  Perhaps
	 * this was a leftover from the distant, misty beginnings of time"
	 */
#if 0
	/* FIXME: When we start using message queues, we will need to
	 * change this so that it is no longer necessary to immediately
	 * free the buffer.
	 */

	mx_free_network_buffer( message_buffer );
#endif

	return mx_status;
}

#endif /* HAVE_TCPIP */
