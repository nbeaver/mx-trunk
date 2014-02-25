/*
 * Name: ms_socket_select.c
 *
 * Purpose: Process incoming MX socket events with select().
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_socket.h"
#include "mx_select.h"
#include "mx_process.h"

#include "ms_mxserver.h"

/*-------------------------------------------------------------------------*/

void
mxsrv_update_select_fds( fd_set *select_readfds,
			int *max_fd,
			int handler_array_size,
			MX_SOCKET_HANDLER_LIST *socket_handler_list )
{
	int i;
	MX_SOCKET *current_socket;

	FD_ZERO(select_readfds);

	*max_fd = -1;

	for ( i = 0; i < handler_array_size; i++ ) {
		if ( socket_handler_list->array[i] != NULL ) {
			current_socket
		    = socket_handler_list->array[i]->synchronous_socket;

			if ( current_socket->socket_fd < 0 ) {
				MX_DEBUG(0,
("main #1: socket_handler_list->array[%d] = %p, current_socket fd = %d",
	i, socket_handler_list->array[i], current_socket->socket_fd));
			}

			FD_SET( current_socket->socket_fd, select_readfds );

			if ( current_socket->socket_fd > (*max_fd) ) {
				*max_fd = current_socket->socket_fd;
			}
		}
	}

}

/*-------------------------------------------------------------------------*/

void
mxsrv_process_sockets_with_select( MX_RECORD *mx_record_list,
				int handler_array_size,
				MX_SOCKET_HANDLER_LIST *socket_handler_list )
{
	static const char fname[] = "mxsrv_process_sockets_with_select()";

	int i, saved_errno;
	int num_fds, max_fd;
	MX_SOCKET *current_socket;

	MX_EVENT_HANDLER *event_handler;
	fd_set select_readfds;
	struct timeval timeout;

	mx_status_type ( *process_event_fn ) ( MX_RECORD *,
					MX_SOCKET_HANDLER *,
					MX_SOCKET_HANDLER_LIST *,
					MX_EVENT_HANDLER * );


	/* Initialize the arguments to select(). */

	mxsrv_update_select_fds( &select_readfds,
				&max_fd,
				handler_array_size,
				socket_handler_list );

#ifdef OS_WIN32
	max_fd = -1;
#else
	max_fd++;
#endif

	timeout.tv_sec = 0;
	timeout.tv_usec = 1;     /* Wait 1 microsecond. */
	
	/* Use select() to look for events. */

	num_fds = select( max_fd, &select_readfds, NULL, NULL, &timeout );

	saved_errno = errno;

	if ( num_fds < 0 ) {
		if ( saved_errno == EINTR ) {

#if MS_PROCESS_SOCKETS_DEBUG_SIGNALS
			MX_DEBUG(-2,("%s: EINTR returned by select()",
				fname ));
#endif

			/* Receiving an EINTR errno from select()
			 * is normal.  It just means that a signal
			 * handler fired while we were blocked in
			 * the select() system call.
			 */
		} else {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Error in select() while waiting for events.  "
			"Errno = %d.  Error string = '%s'.",
			saved_errno, strerror( saved_errno ) );
		}
	} else if ( num_fds == 0 ) {

		/* Didn't get any events, so do nothing here. */

	} else {
		MX_DEBUG( 2,("%s: select() returned.  num_fds = %d",
				fname, num_fds));

		/* Figure out which sockets had events and
		 * then process the events.
		 */

		for ( i = 0; i < handler_array_size; i++ ) {
		    if ( socket_handler_list->array[i] != NULL ) {

			current_socket
		    = socket_handler_list->array[i]->synchronous_socket;

			if ( current_socket->socket_fd < 0 ) {
				MX_DEBUG(0,
("main #2: socket_handler_list->array[%d] = %p, current_socket fd = %d",
	i, socket_handler_list->array[i], current_socket->socket_fd));
			}

			if ( FD_ISSET(current_socket->socket_fd,
						&select_readfds) )
			{
				event_handler =
			socket_handler_list->array[i]->event_handler;

				if ( event_handler == NULL ) {
					(void) mx_error(
					MXE_NETWORK_IO_ERROR, fname,
	"Event handler pointer for socket handler %d is NULL.", i);

					continue;
				}

				process_event_fn
				    = event_handler->process_event;

				if ( process_event_fn == NULL ) {
					(void) mx_error(
					MXE_NETWORK_IO_ERROR, fname,
"process_event function pointer for socket handler %d is NULL.", i);

					continue;
				}

				/* Process the event. */

				(void) ( *process_event_fn )
					( mx_record_list,
					  socket_handler_list->array[i],
					  socket_handler_list,
					  event_handler );
			}
		    }
		}
	}

	return;
}

