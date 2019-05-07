/*
 * Name: ms_socket_select.c
 *
 * Purpose: Process incoming MX socket events with select().
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014, 2016, 2019 Illinois Institute of Technology
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
mxsrv_update_select_fds( MX_LIST_HEAD *list_head,
			MX_SOCKET_HANDLER_LIST *socket_handler_list )
{
	static const char fname[] = "mxsrv_update_select_fds()";
	int i, handler_array_size;
	int highest_socket_in_use;
	MX_SOCKET *current_socket;
	fd_set select_readfds;

	if ( socket_handler_list == (MX_SOCKET_HANDLER_LIST *) NULL ) {
		mx_warning( "%s: socket_handler_list is NULL!", fname );

		mx_stack_traceback();

		mx_warning( "%s: continuing anyway.", fname );
	}

	handler_array_size = socket_handler_list->handler_array_size;

	FD_ZERO(&select_readfds);

	highest_socket_in_use = -1;

	for ( i = 0; i < handler_array_size; i++ ) {
		if ( socket_handler_list->array[i] != NULL ) {
			current_socket
				= socket_handler_list->array[i]->mx_socket;

			if ( current_socket->socket_fd < 0 ) {
			    MX_DEBUG(0,
("main #1: socket_handler_list->array[%d] = %p, current_socket fd = %d",
	i, socket_handler_list->array[i], (int) current_socket->socket_fd));
			}

			FD_SET( current_socket->socket_fd, &select_readfds );

			if ( highest_socket_in_use < 0 ) {
			    highest_socket_in_use = current_socket->socket_fd;

			} else
			if ( current_socket->socket_fd > highest_socket_in_use )
			{
			    highest_socket_in_use = current_socket->socket_fd;
			}
		}
	}

	socket_handler_list->highest_socket_in_use = highest_socket_in_use;

	/* Note: The following is an ANSI C structure copy. */

	socket_handler_list->select_readfds = select_readfds;

	return;
}

/*-------------------------------------------------------------------------*/

void
mxsrv_process_sockets_with_select( MX_RECORD *mx_record_list,
				MX_SOCKET_HANDLER_LIST *socket_handler_list )
{
	static const char fname[] = "mxsrv_process_sockets_with_select()";

	int i, handler_array_size;
	int num_fds_to_check, num_fds_with_activity;
	MX_SOCKET *current_socket;
	fd_set select_readfds;
	mx_bool_type socket_data_available;

	MX_EVENT_HANDLER *event_handler;
	struct timeval timeout;

#if !defined(OS_WIN32)
	int saved_errno;
#endif

	mx_status_type ( *process_event_fn ) ( MX_RECORD *,
					MX_SOCKET_HANDLER *,
					MX_SOCKET_HANDLER_LIST *,
					MX_EVENT_HANDLER * );

	if ( socket_handler_list->highest_socket_in_use < 0 ) {
		/* If no sockets are in use, then there is no point
		 * to calling select().
		 */

		MX_DEBUG(-2,
		("%s: NO SOCKETS AVAILABLE FOR SELECT()!!!", fname));

		return;
	}

	/* At least one socket is in use, so we will call select(). */

	handler_array_size = socket_handler_list->handler_array_size;

	/* Note: The following is an ANSI C structure copy.
	 * 
	 * We work here using a _copy_ of the master version of
	 * select_readfds, since select() actually changes the copy
	 * of select_readfds that is passed to it.
	 */

	select_readfds = socket_handler_list->select_readfds;

	/* Initialize the arguments to select(). */

#ifdef OS_WIN32
	num_fds_to_check = -1;
#else
	num_fds_to_check = socket_handler_list->highest_socket_in_use + 1;
#endif

	timeout.tv_sec = 0;
	timeout.tv_usec = 1;     /* Wait 1 microsecond. */

	socket_data_available = FALSE;
	
	/* Use select() to look for events. */

	num_fds_with_activity = select( num_fds_to_check,
					&select_readfds, NULL, NULL, &timeout );

#if defined(OS_WIN32)

	if ( num_fds_with_activity == SOCKET_ERROR ) {
		int last_error_code;

		last_error_code = WSAGetLastError();

		(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
		"select() generated a Windows Socket error code of %d",
			last_error_code );

		socket_data_available = FALSE;

	} else if ( num_fds_with_activity == 0 ) {

		/* Didn't get any events, so do nothing here. */

		socket_data_available = FALSE;
	} else {
		socket_data_available = TRUE;
	}

#else /* Not OS_WIN32 */

	saved_errno = errno;

	if ( num_fds_with_activity < 0 ) {
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
		socket_data_available = FALSE;

	} else if ( num_fds_with_activity == 0 ) {

		/* Didn't get any events, so do nothing here. */

		socket_data_available = FALSE;
	} else {
		socket_data_available = TRUE;
	}

#endif /* Not OS_WIN32 */

	if ( socket_data_available ) {
		MX_DEBUG(2,
		("%s: select() returned.  num_fds_with_activity = %d",
				fname, num_fds_with_activity));

		/* Figure out which sockets had events and
		 * then process the events.
		 */

		for ( i = 0; i < handler_array_size; i++ ) {
		    if ( socket_handler_list->array[i] != NULL ) {

			current_socket
				= socket_handler_list->array[i]->mx_socket;

			if ( current_socket->socket_fd < 0 ) {
				MX_DEBUG(0,
("main #2: socket_handler_list->array[%d] = %p, current_socket fd = %d",
	i, socket_handler_list->array[i], (int) current_socket->socket_fd));
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

