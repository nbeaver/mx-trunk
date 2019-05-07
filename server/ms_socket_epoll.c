/*
 * Name: ms_socket_epoll.c
 *
 * Purpose: Process incoming MX socket events with epoll().
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* FIXME: This version of the code does not yet work (2019-03-29). */

#include <stdio.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_socket.h"
#include "mx_process.h"

#include "ms_mxserver.h"

#if HAVE_LINUX_EPOLL

#include <sys/epoll.h>

/*-------------------------------------------------------------------------*/

void
mxsrv_update_epoll_fds( MX_LIST_HEAD *list_head,
			MX_SOCKET_HANDLER_LIST *socket_handler_list )
{
	static const char fname[] = "mxsrv_update_epoll_fds()";

	int i, handler_array_size;
	int saved_errno, ctl_status;
	int highest_socket_in_use;
	MX_SOCKET *current_socket;
	struct epoll_event event;

	if ( socket_handler_list == (MX_SOCKET_HANDLER_LIST *) NULL ) {
		mx_warning( "%s: socket_handler_list is NULL!", fname );

		mx_stack_traceback();

		mx_warning( "%s: continuing anyway.", fname );
	}

	if ( socket_handler_list->epoll_fd < 0 ) {
		socket_handler_list->epoll_fd = epoll_create( 1 );

		if ( socket_handler_list->epoll_fd < 0 ) {
			saved_errno = errno;

			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The attempt to create an epoll instance failed, "
			"so we are exiting..."
			"Errno = %d, error_message = '%s'.",
				saved_errno, strerror(saved_errno) );

			mx_force_core_dump();

			/* Should not return. */
		}
	}

	handler_array_size = socket_handler_list->handler_array_size;

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

			event.events = EPOLLIN;
			event.data.fd = current_socket->socket_fd;

			ctl_status = epoll_ctl( socket_handler_list->epoll_fd,
					EPOLL_CTL_ADD,
					current_socket->socket_fd,
					&event );

			if ( ctl_status != 0 ) {
				saved_errno = errno;

				(void) mx_error( MXE_OPERATING_SYSTEM_ERROR,
				fname, "The attempt to add file descriptor %d "
				"to epoll descriptor %d failed.  "
				"Errno = %d, error message = '%s'.",
					current_socket->socket_fd,
					socket_handler_list->epoll_fd,
					saved_errno, strerror(saved_errno) );

				/* Continue on to the next fd. */

				continue;
			}

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

	return;
}

/*-------------------------------------------------------------------------*/

#define MXU_MAX_EPOLL_EVENTS 100

void
mxsrv_process_sockets_with_epoll( MX_RECORD *mx_record_list,
				MX_SOCKET_HANDLER_LIST *socket_handler_list )
{
	static const char fname[] = "mxsrv_process_sockets_with_epoll()";

	int i, handler_array_size;
	int saved_errno;
	MX_SOCKET *current_socket;
	int current_socket_fd;
	int num_epoll_events, timeout_milliseconds;
	struct epoll_event epoll_events[MXU_MAX_EPOLL_EVENTS];
	mx_bool_type socket_data_available;

	MX_EVENT_HANDLER *event_handler;

	mx_status_type ( *process_event_fn ) ( MX_RECORD *,
					MX_SOCKET_HANDLER *,
					MX_SOCKET_HANDLER_LIST *,
					MX_EVENT_HANDLER * );

	if ( socket_handler_list->highest_socket_in_use < 0 ) {
		/* If no sockets are in use, then there is no point
		 * to calling epoll().
		 */

		MX_DEBUG(-2,
		("%s: NO SOCKETS AVAILABLE FOR EPOLL!!!", fname));

		return;
	}

	handler_array_size = socket_handler_list->handler_array_size;

	/* Use epoll_wait() to look for events. */

	timeout_milliseconds = 1;

	num_epoll_events = epoll_wait( socket_handler_list->epoll_fd,
				epoll_events, MXU_MAX_EPOLL_EVENTS,
				timeout_milliseconds );

	if ( num_epoll_events < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Error in epoll() while waiting for events.  "
		"Errno = %d.  Error string = '%s'.",
		saved_errno, strerror( saved_errno ) );

		socket_data_available = FALSE;

	} else if ( num_epoll_events == 0 ) {

		/* Didn't get any events, so do nothing here. */

		socket_data_available = FALSE;
	} else {
		socket_data_available = TRUE;
	}

	if ( socket_data_available ) {
		MX_DEBUG(2,("%s: epoll() returned.  num_epoll_events = %d",
						fname, num_epoll_events));

		/* Figure out which sockets had events and
		 * then process the events.
		 */

		for ( i = 0; i < num_epoll_events; i++ ) {

			current_socket_fd = epoll_events[i].data.fd;

			/* FIXME!!! FIXME!!! FIXME!!! */

			/* The following _DOES_NOT_WORK_, since the contents
			 * of socket_handler_list->array[i] _DOES_NOT_
			 * correspond to socket descriptor i at this time.
			 * We need to evaluate the possibility of having
			 * the index of the array element match the file
			 * descriptor value.
			 */

			/* FIXME: Change things to make the following line
			 * true!
			 */

			current_socket =
		socket_handler_list->array[ current_socket_fd ]->mx_socket;

			event_handler =
				socket_handler_list->array[i]->event_handler;

			if ( event_handler == NULL ) {
				(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Event handler pointer for socket handler %d is NULL.", i);

				continue;
			}

			process_event_fn = event_handler->process_event;

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

	return;
}

#endif /* HAVE_LINUX_EPOLL */
