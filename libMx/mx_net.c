/*
 * Name:    mx_net.c
 *
 * Purpose: Client side part of MX network protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define NETWORK_DEBUG		TRUE	/* You should normally leave this on. */

#define NETWORK_DEBUG_TIMING			FALSE

#define NETWORK_DEBUG_HEADER_LENGTH		FALSE

#define NETWORK_DEBUG_BUFFER_ALLOCATION		FALSE

#define NETWORK_DEBUG_MESSAGE_IDS		FALSE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <stdarg.h>
#include <math.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_inttypes.h"
#include "mx_array.h"
#include "mx_bit.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_handle.h"
#include "mx_callback.h"
#include "mx_driver.h"

#include "n_tcpip.h"
#include "n_unix.h"

#if ( HAVE_TCPIP == 0 )
#     define htonl(x) (x)
#     define ntohl(x) (x)
#endif

#if HAVE_XDR
#  include "mx_xdr.h"
#endif

#if NETWORK_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_allocate_network_buffer( MX_NETWORK_MESSAGE_BUFFER **message_buffer,
				size_t initial_length )
{
	static const char fname[] = "mx_allocate_network_buffer()";

#if NETWORK_DEBUG_BUFFER_ALLOCATION
	MX_DEBUG(-2,("%s invoked for an initial length of %lu",
		fname, (unsigned long) initial_length ));
#endif

	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );
	}

	if ( initial_length < MXU_NETWORK_MINIMUM_MESSAGE_BUFFER_LENGTH ) {
		initial_length = MXU_NETWORK_MINIMUM_MESSAGE_BUFFER_LENGTH;
	}

	*message_buffer = malloc( sizeof(MX_NETWORK_MESSAGE_BUFFER) );

	if ( (*message_buffer) == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_NETWORK_MESSAGE_BUFFER structure." );
	}

	/* We use calloc() rather than malloc() here to make Valgrind happy. */

	(*message_buffer)->u.uint32_buffer = calloc( 1, initial_length + 1 );

	if ( (*message_buffer)->u.uint32_buffer == (uint32_t *) NULL ) {

		free( *message_buffer );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte "
		"network message buffer.", (unsigned long) initial_length );
	}

	(*message_buffer)->buffer_length = initial_length;

	(*message_buffer)->data_format = MX_NETWORK_DATAFMT_ASCII;

#if NETWORK_DEBUG_BUFFER_ALLOCATION
	MX_DEBUG(-2,
("%s: *message_buffer = %p, uint32_buffer = %p, char_buffer = %p, length = %lu",
	 	fname, *message_buffer,
		(*message_buffer)->u.uint32_buffer,
		(*message_buffer)->u.char_buffer,
		(unsigned long) (*message_buffer)->buffer_length));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_reallocate_network_buffer( MX_NETWORK_MESSAGE_BUFFER *message_buffer,
					size_t new_length )
{
	static const char fname[] = "mx_reallocate_network_buffer()";

	size_t old_length;
	uint32_t *header;
	char *new_ptr;
#if NETWORK_DEBUG_BUFFER_ALLOCATION
	int i;
#endif

	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );
	}

	if ( new_length < MXU_NETWORK_MINIMUM_MESSAGE_BUFFER_LENGTH ) {
		new_length = MXU_NETWORK_MINIMUM_MESSAGE_BUFFER_LENGTH;
	}

	if ( message_buffer->u.uint32_buffer == (uint32_t *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The buffer array pointer for network message buffer %p is NULL.",
	    		message_buffer );
	}

	old_length = message_buffer->buffer_length;

#if NETWORK_DEBUG_BUFFER_ALLOCATION
	MX_DEBUG(-2,("%s will change the length of network message buffer %p "
		"from %lu bytes to %lu bytes.", fname, message_buffer,
			(unsigned long) old_length,
			(unsigned long) new_length ));
#endif

	header = message_buffer->u.uint32_buffer;

	MXW_UNUSED( header );

#if NETWORK_DEBUG_BUFFER_ALLOCATION
	MX_DEBUG(-2,("%s: #1 header = %p", fname, header));

	for ( i = 0; i < 5; i++ ) {
		MX_DEBUG(-2,("%s: header[%d] = %lu",
			fname, i, (unsigned long) header[i]));
	}

	MX_DEBUG(-2,("%s: BEFORE realloc() end of buffer test.", fname));

	message_buffer->u.char_buffer[ old_length - 1 ] = 0;

	MX_DEBUG(-2,("%s: BEFORE realloc(), uint32_buffer = %p", fname,
				message_buffer->u.uint32_buffer));
#endif

	message_buffer->u.uint32_buffer =
		realloc( message_buffer->u.uint32_buffer, new_length + 1 );

#if NETWORK_DEBUG_BUFFER_ALLOCATION
	MX_DEBUG(-2,("%s: AFTER realloc(), uint32_buffer = %p", fname,
				message_buffer->u.uint32_buffer));
#endif

	if ( message_buffer->u.uint32_buffer == (uint32_t *) NULL ) {

		message_buffer->buffer_length = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to change network message buffer %p "
		"from %lu bytes long to %lu bytes long.  The length of the "
		"buffer has been set to zero.", message_buffer,
			(unsigned long) old_length,
			(unsigned long) new_length );
	}

#if NETWORK_DEBUG_BUFFER_ALLOCATION
	MX_DEBUG(-2,("%s: AFTER realloc() end of buffer test.", fname));

	message_buffer->u.char_buffer[ new_length - 1 ] = 0;

	MX_DEBUG(-2,
	("%s: AFTER realloc() end of buffer test succeeded", fname));
#endif
	message_buffer->buffer_length = new_length;

	header = message_buffer->u.uint32_buffer;

#if NETWORK_DEBUG_BUFFER_ALLOCATION
	MX_DEBUG(-2,("%s: #2 header = %p", fname, header));

	for ( i = 0; i < 5; i++ ) {
		MX_DEBUG(-2,("%s: header[%d] = %lu",
			fname, i, (unsigned long) header[i]));
	}

	MX_DEBUG(-2,
	("%s: message_buffer = %p, u.uint32_buffer = %p, length = %lu",
	 	fname, message_buffer, message_buffer->u.uint32_buffer,
		(unsigned long) message_buffer->buffer_length));
#endif
	/* Zero out the new part of the buffer so that Valgrind does
	 * not get upset.
	 */

	new_ptr = message_buffer->u.char_buffer + old_length;

	memset( new_ptr, 0, new_length - old_length );

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT void
mx_free_network_buffer( MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return;
	}

#if NETWORK_DEBUG_BUFFER_ALLOCATION
	MX_DEBUG(-2,("mx_free_network_buffer(): message_buffer = %p, "
		"u.uint32_buffer = %p, length = %lu",
	 	message_buffer, message_buffer->u.uint32_buffer,
		(unsigned long) message_buffer->buffer_length));
#endif

	if ( message_buffer->u.uint32_buffer != NULL ) {
		free( message_buffer->u.uint32_buffer );
	}

	free( message_buffer );

	return;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_receive_message( MX_RECORD *server_record,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	static const char fname[] = "mx_network_receive_message()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER *,
				MX_NETWORK_MESSAGE_BUFFER * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Record pointer passed was NULL." );
	}
	if ( message_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
    "MX_NETWORK_MESSAGE_BUFFER pointer passed for record '%s' was NULL.",
			server_record->name );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	if ( server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		return mx_error( MXE_RECORD_DISABLED_BY_USER, fname,
		"Connections to MX server '%s' are disabled.",
			server_record->name );
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->receive_message;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"receive_message function pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = ( *fptr ) ( server, message_buffer );

#if NETWORK_DEBUG
	if ( server->server_flags & MXF_NETWORK_SERVER_DEBUG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: SERVER (%s) -> CLIENT\n",
				server_record->name );

		mx_network_display_message( message_buffer, NULL,
					server->use_64bit_network_longs );
	}
#endif

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_send_message( MX_RECORD *server_record,
			MX_NETWORK_MESSAGE_BUFFER *message_buffer )
{
	static const char fname[] = "mx_network_send_message()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER *,
				MX_NETWORK_MESSAGE_BUFFER * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Record pointer passed was NULL." );
	}
	if ( message_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
    "MX_NETWORK_MESSAGE_BUFFER pointer passed for record '%s' was NULL.",
			server_record->name );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	if ( server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		return mx_error( MXE_RECORD_DISABLED_BY_USER, fname,
		"Connections to MX server '%s' are disabled.",
			server_record->name );
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->send_message;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"send_message function pointer for server record '%s' is NULL.",
			server_record->name );
	}

#if NETWORK_DEBUG
	if ( server->server_flags & MXF_NETWORK_SERVER_DEBUG_VERBOSE ) {
		fprintf( stderr, "\nMX NET: CLIENT -> SERVER (%s)\n",
					server_record->name );

		mx_network_display_message( message_buffer, NULL,
					server->use_64bit_network_longs );
	}
#endif
	mx_status = ( *fptr ) ( server, message_buffer );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_server_discover_header_length( MX_RECORD *server_record )
{
	static const char fname[] =
		"mx_network_server_discover_header_length()";

	MX_NETWORK_SERVER *server;
	long datatype, num_dimensions, dimension_array[1];
	uint32_t *header;
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The server_record pointer passed was NULL." );
	}

	server = server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for record '%s' is NULL.",
			server_record->name );
	}

	if ( server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		return mx_error( MXE_RECORD_DISABLED_BY_USER, fname,
		"Connections to MX server '%s' are disabled.",
			server_record->name );
	}

	/* A safe way to find out the length of headers returned by
	 * a remote MX server is to ask for the field type of the
	 * 'mx_database.list_is_active' field.
	 */

	mx_status = mx_get_field_type( server_record,
				"mx_database.list_is_active", 0, &datatype,
				&num_dimensions, dimension_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We do not actually care about the value returned by the server
	 * in the variable 'active'.  The value may actually be corrupt if
	 * there is a mismatch in the header lengths.  Instead, we want to
	 * examine the contents of the header of the message just returned.
	 */

	if ( server->message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The message_buffer pointer for server '%s' is NULL "
		"at a time when the buffer should already have been allocated.",
			server_record->name );
	}

	header = server->message_buffer->u.uint32_buffer;

	if ( header == (uint32_t *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The message_buffer->u.uint32_buffer pointer for server '%s' "
		"is NULL at a time when the buffer should already have "
		"been allocated.", server_record->name );
	}

	server->remote_header_length = 
		mx_ntohl( header[MX_NETWORK_HEADER_LENGTH] );

#if NETWORK_DEBUG_HEADER_LENGTH
	MX_DEBUG(-2,("%s: server '%s' remote_header_length = %lu",
		fname, server_record->name, server->remote_header_length));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

/* Each time we send a new RPC message, we increment the message id.
 * However, we must arrange for the message id to wrap back to 1, 
 * whenever it goes outside the allowed range (currently 1 to 0x7fffffff).
 */

MX_EXPORT void
mx_network_update_message_id( unsigned long *message_id )
{
	if ( message_id == NULL )
		return;

	(*message_id)++;

	/* We must prevent two things here:
	 * 
	 * 1.  We must not allow the message id to increment past the largest
	 *     allowed RPC message id and on into the range where the callback
	 *     bit is set.
	 *
	 * 2.  We must not allow the message id to have a value of 0.
	 *
	 * For both cases, we set the message id to 1.
	 */

	if ( (*message_id) >= MX_NETWORK_MESSAGE_IS_CALLBACK ) {
		*message_id = 1;
	} else
	if ( (*message_id) == 0 ) {
		*message_id = 1;
	}

	return;
}

/* ====================================================================== */

#define RETURN_IF_TIMED_OUT \
	do {                                                                  \
		current_tick = mx_current_clock_tick();                       \
                                                                              \
		comparison = mx_compare_clock_ticks( current_tick, end_tick );\
                                                                              \
		if ( comparison >= 0 ) {                                      \
			return mx_error( (MXE_TIMED_OUT | MXE_QUIET), fname,  \
				"Timed out after waiting %g seconds for "     \
				"message ID %#lx from MX server '%s'.",       \
					timeout_in_seconds,                   \
					(unsigned long) message_id,           \
					server_record->name );                \
		}                                                             \
	} while (0)

/* ----- */

#define MX_NETWORK_MAX_ID_MISMATCH    10

MX_EXPORT mx_status_type
mx_network_wait_for_message_id( MX_RECORD *server_record,
			MX_NETWORK_MESSAGE_BUFFER *buffer,
			uint32_t message_id,
			double timeout_in_seconds )
{
	static const char fname[] = "mx_network_wait_for_message_id()";

	MX_NETWORK_SERVER *server;
	MX_CLOCK_TICK current_tick, end_tick, timeout_in_ticks;
	MX_LIST_ENTRY *list_start, *list_entry;
	MX_CALLBACK *callback;
	MX_LIST_HEAD *record_list_head;
	mx_bool_type message_is_available, timeout_enabled;
	mx_bool_type debug_enabled, callback_found;
	int comparison;
	uint32_t *header;
	uint32_t received_message_id, difference;
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	if ( server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		/* If an MX server is disabled in the database, then no
		 * messages will ever arrive from it.
		 */

		return mx_error( (MXE_NOT_AVAILABLE | MXE_QUIET), fname,
		"MX server '%s' is disabled in the MX database.",
			server_record->name );
	}

	record_list_head = mx_get_record_list_head_struct( server_record );

	if ( record_list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer is NULL." );
	}

	if ( buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		buffer = server->message_buffer;
	}

	/* Figure out when the timeout period will end. */

	if ( timeout_in_seconds < 0.0 ) {
		timeout_enabled = FALSE;
	} else {
		timeout_enabled = TRUE;
	}

	if ( timeout_enabled ) {
		timeout_in_ticks =
		    mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

		current_tick = mx_current_clock_tick();

		end_tick = mx_add_clock_ticks( current_tick, timeout_in_ticks );
	}

	if ( server->server_flags & MXF_NETWORK_SERVER_DEBUG_ANY ) {
		debug_enabled = TRUE;
	} else {
		debug_enabled = FALSE;
	}

	MXW_UNUSED( debug_enabled );

#if NETWORK_DEBUG_MESSAGE_IDS
	if ( debug_enabled ) {
		fprintf( stderr,
		"\nMX NET: Waiting for message ID %#lx, timeout = %g sec\n",
	    			(unsigned long) message_id,
				timeout_in_seconds );
	}
#endif
	/* Wait for messages.  We always go through the loop at least once. */

	do {
		/* Sleep for a moment to make sure that we do not
		 * use up all available cpu time.
		 */

		mx_msleep(1);

		/* Are any network messages available? */

		mx_status = mx_network_message_is_available( server_record,
							&message_is_available );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;
		case MXE_NETWORK_CONNECTION_LOST:
			/* If we get here, then the remote server has
			 * disconnected or crashed.  If so, then we
			 * close the network connection.
			 */

			(void) mx_close_hardware( server_record );

			return mx_status;
			break;
		default:
			return mx_status;
		}

		/* If no messages are available, see if we have timed out. */

		if ( message_is_available == FALSE ) {
			if ( timeout_enabled ) {
				RETURN_IF_TIMED_OUT;
			}

			/* Go back to the top of the loop and try again. */

			continue;
		}

		/* If we get here, then a message is available to be read
		 * from the server.  Thus, we now try to read the message.
		 */

		mx_status = mx_network_receive_message( server_record, buffer );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If we already know that the server does not support 
		 * message IDs, then return here.
		 */

		if ( mx_server_supports_message_ids(server) == FALSE ) {
			return MX_SUCCESSFUL_RESULT;
		}

		/* If the message ID, matches the number that we are
		 * looking for, then we can return to our caller now.
		 */

		header = buffer->u.uint32_buffer;

		if ( header == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The uint32_buffer pointer for message buffer %p "
			"used for server '%s' is NULL.",
		    		buffer, server_record->name );
		}

		received_message_id = 
			mx_ntohl( header[ MX_NETWORK_MESSAGE_ID ] );

		if ( received_message_id == 0 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Received a message ID of 0 from server '%s'.  "
			"Message ID 0 is illegal, so this should not happen.",
				server_record->name );
		}

		if ( received_message_id == message_id ) {
#if NETWORK_DEBUG_MESSAGE_IDS
			if ( debug_enabled ) {
				fprintf( stderr,
	"\nMX NET: Message ID %#lx has finally arrived from server '%s'.\n",
		    			(unsigned long) message_id,
					server_record->name );
			}
#endif
			return MX_SUCCESSFUL_RESULT;
		}

		/* If we get here, this message is not the one that we
		 * were waiting for.
		 */

		/* Is the message a callback message? */

		if ( received_message_id & MX_NETWORK_MESSAGE_IS_CALLBACK ) {

		    	/* The message is a callback message, so we
			 * must invoke the callback.
			 */

#if NETWORK_DEBUG_MESSAGE_IDS
			MX_DEBUG(-2,
			("%s: Handling callback for message ID %#lx",
				fname, (unsigned long) received_message_id ));
#endif

			if ( server->callback_list == NULL ) {
				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"Received callback %#lx from server '%s', "
				"but no callbacks are registered on "
				"the client side.",
					(unsigned long) received_message_id,
					server_record->name );
			}

			/* Look for the callback in the server record's
			 * callback list.
			 */

			list_start = server->callback_list->list_start;

			if ( list_start == (MX_LIST_ENTRY *) NULL ) {
				return mx_error(
				MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The list_start pointer for the callback list "
				"belonging to server record '%s' is NULL.",
					server_record->name );
			}

			list_entry = list_start;

			callback_found = FALSE;

			do {
			    callback = list_entry->list_entry_data;

			    if ( callback == (MX_CALLBACK *) NULL ) {
				return mx_error(
				MXE_CORRUPT_DATA_STRUCTURE, fname,
				"Null MX_CALLBACK pointer found in "
				"callback list for server '%s'.",
						server_record->name );
			    }

			    if ( callback->callback_id == received_message_id )
			    {
			    	/* This is the correct callback. */

				callback_found = TRUE;

				break;	/* Exit the do...while() loop. */
			    }

			    list_entry = list_entry->next_list_entry;
				
			} while( list_entry != list_start );

			MXW_UNUSED( callback_found );

#if NETWORK_DEBUG
			if ( record_list_head->network_debug_flags
				& MXF_NETDBG_SUMMARY )
			{
				MX_NETWORK_FIELD *nf;
				unsigned long data_type, message_type;
				unsigned long header_length, message_length;
				char nf_label[ 100 ];

				data_type = mx_ntohl(
					header[ MX_NETWORK_DATA_TYPE ] );
				message_type = mx_ntohl(
					header[ MX_NETWORK_MESSAGE_TYPE ] );
				header_length = mx_ntohl(
					header[ MX_NETWORK_HEADER_LENGTH ] );
				message_length = mx_ntohl(
					header[ MX_NETWORK_MESSAGE_LENGTH ] );

				nf = callback->u.network_field;

				mx_network_get_nf_label(
					server_record, nf->nfname,
					nf_label, sizeof(nf_label) );

				fprintf( stderr,
				"MX CALLBACK %#lx from '%s', value = ",
					(unsigned long) received_message_id,
					nf_label );

				mx_network_buffer_show_value(
					buffer->u.char_buffer + header_length,
					server->data_format,
					data_type,
					message_type,
					message_length,
					server->use_64bit_network_longs );

				fprintf( stderr, "\n" );
			}
#endif /* NETWORK_DEBUG */

			/* If we have found the correct callback, then
			 * invoke the callback.
			 */

			mx_status = mx_invoke_callback( callback,
						MXCBT_VALUE_CHANGED, FALSE ); 

			/* If the timeout time has arrived, then return
			 * to our caller.
			 */

			if ( timeout_enabled ) {
				RETURN_IF_TIMED_OUT;
			}

			/* Go back to the top of the loop and look again
			 * for the message ID that we are waiting for.
			 */

			continue;
		}

		/* If we get here, then we have received an RPC message that
		 * does not match the message ID that we were looking for.
		 */

#if NETWORK_DEBUG_MESSAGE_IDS
		MX_DEBUG(-2,
		("%s: Received %#lx when expecting %#lx from '%s'.",
			fname, (unsigned long) received_message_id,
			(unsigned long) message_id,
			server_record->name ));
#endif

		/* Check for illegal message IDs. */

		if ( received_message_id >= MX_NETWORK_MESSAGE_IS_CALLBACK ) {

			/* Note: We should only get here if the received
			 * message id is greater than or equal to
			 * 0x100000000 (8 zeros) and the 0x80000000 (7 zeros)
			 * bit is not set.  This should only be possible
			 * if uint32_t integers actually have more than
			 * 32 bits in them.  This may happen at some point
			 * in the future if 64-bit or 128-bit architectures
			 * come into being which do not directly support
			 * 32 bit integer types in hardware.  It might
			 * happen someday.
			 *
			 * A much less likely possibility is if the code is
			 * run on a machine where the word sizes are not a
			 * power of 2.  There were ancient systems with
			 * 36-bit integers like the DECsystem 10 or 20,
			 * but those are dead and gone.
			 *
			 * The only modern system I know of where this might
			 * come up is a 48-bit IBM AS/400.  Now, it is very
			 * unlikely that MX will ever be ported to an AS/400.
			 * However, if you are successfully running MX on
			 * an AS/400, I salute you.  Also, I would love to
			 * hear about it if you were to do something that
			 * unusual.
			 * 
			 * Anyway, this will probably never happen, but
			 * we check for it anyway.
			 */

			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Illegal RPC message id %#lx received from server '%s'."
			"  The maximum legal RPC message id is %#x.",
				(unsigned long) received_message_id,
				server_record->name,
				MX_NETWORK_MESSAGE_IS_CALLBACK - 1 );
		}

		/*
		 * If the received message id is less than, but close to
		 * the expected message id, we make the hopeful assumption
		 * that the client timed out waiting for an old message
		 * and now that old message has arrived.  However, we are
		 * no longer in a position to deal with the old message,
		 * so we discard it and go back to look for the correct
		 * message.
		 *
		 * In making this decision, we must take into account the
		 * possibility that the message id recently wrapped around
		 * to 1.
		 */ 

		/* Handle wraparound case first. */

		if ( (MX_NETWORK_MESSAGE_IS_CALLBACK - received_message_id)
			< MX_NETWORK_MAX_ID_MISMATCH )
		{
			difference = MX_NETWORK_MESSAGE_IS_CALLBACK
					+ message_id - received_message_id;
		} else {
			difference = message_id - received_message_id;
		}

		if ( difference <= MX_NETWORK_MAX_ID_MISMATCH ) {
			/* If the difference is small enough, we generate a
			 * warning and go back to look again for the message
			 * that we actually want.
			 */

			mx_warning( "Received old RPC message ID %#lx when we "
			"were waiting for message ID %#lx from server '%s'.  "
			"Perhaps a previous transaction with the server "
			"timed out?  RPC message %#lx will be discarded.",
				(unsigned long) received_message_id,
				(unsigned long) message_id,
				server_record->name,
				(unsigned long) received_message_id );
		} else {
			/* If we have a large difference, we bomb out
			 * with an error message.
			 */

			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Received unexpected RPC message ID %#lx when we "
			"were waiting for message ID %#lx from server '%s'.", 
				(unsigned long) received_message_id,
				(unsigned long) message_id,
				server_record->name );
		}

		/* We have finished processing this message, so check
		 * to see if we have timed out.
		 */

		if ( timeout_enabled ) {
			RETURN_IF_TIMED_OUT;
		}

	} while (1);
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_wait_for_messages_from_server(
			MX_RECORD *server_record,
			double timeout_in_seconds )
{
	static const char fname[] =
		"mx_network_wait_for_messages_from_server()";

	MX_NETWORK_SERVER *network_server;
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	network_server = server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for record '%s' is NULL.",
			server_record->name );
	}

	mx_status = mx_network_reconnect_if_down( server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( network_server->connection_status & MXCS_RECONNECTED ) {
		(void) mx_network_restore_callbacks( server_record );

		network_server->connection_status &= (~MXCS_RECONNECTED);
	}

	mx_status = mx_network_wait_for_message_id( server_record,
			NULL, 0, timeout_in_seconds );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_wait_for_messages( MX_RECORD *record,
			double timeout_in_seconds )
{
	static const char fname[] = "mx_network_wait_for_messages()";

	MX_RECORD *record_list, *current_record;
	MX_NETWORK_SERVER *network_server;
	MX_CLOCK_TICK start_tick, end_tick, timeout_in_ticks, current_tick;
	int comparison;
	unsigned long cs;
	mx_bool_type timeout_enabled;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	/* Jump to the list head record. */

	record_list = record->list_head;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The list head pointer for record '%s' is NULL.",
			record->name );
	}

	/* Compute the timeout time. */

	if ( timeout_in_seconds < 0.0 ) {
		timeout_enabled = FALSE;
	} else {
		timeout_enabled = TRUE;

		timeout_in_ticks =
			mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

		start_tick = mx_current_clock_tick();

		end_tick = mx_add_clock_ticks(start_tick, timeout_in_ticks);
	}

	/* Check for server callbacks until the timeout expires.  If there
	 * is no timeout.
	 */

	while (1) {

		/* Walk through the record list looking for MX server records.*/

		current_record = record_list->next_record;

		while ( current_record != record_list ) {
			if ( (current_record->mx_superclass == MXR_SERVER)
			  && (current_record->mx_class == MXN_NETWORK_SERVER) )
			{
			    mx_status = mx_network_reconnect_if_down(
			    				current_record );

			    if ( mx_status.code == MXE_SUCCESS ) {
			        network_server =
					current_record->record_class_struct;

				cs = network_server->connection_status;

				if ( cs & MXCS_RECONNECTED ) {
			        	(void) mx_network_restore_callbacks(
							current_record );

					network_server->connection_status
						&= (~MXCS_RECONNECTED);
				}

			        (void) mx_network_wait_for_message_id(
				    	current_record, NULL, 0, 0.0 );
			    }
			}

			current_record = current_record->next_record;
		}

		if ( timeout_enabled ) {
			current_tick = mx_current_clock_tick();

			comparison =
			    mx_compare_clock_ticks( current_tick, end_tick );

			if ( comparison >= 0 ) {
				/* The timeout period has expired,
				 * so return now.
				 */

				return MX_SUCCESSFUL_RESULT;
			}
		} else {
			/* If no timeout interval was specified, then
			 * return after the first pass.
			 */

			return MX_SUCCESSFUL_RESULT;
		}
	}

#if !defined(OS_SOLARIS)
	return MX_SUCCESSFUL_RESULT;
#endif
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_connection_is_up( MX_RECORD *server_record,
				mx_bool_type *connection_is_up )
{
	static const char fname[] = "mx_network_connection_is_up()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER *, mx_bool_type * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}
	if ( connection_is_up == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"connection_is_up pointer is NULL." );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	if ( server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		/* A disabled server is never up. */

		*connection_is_up = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->connection_is_up;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"connection_is_up function pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = ( *fptr ) ( server, connection_is_up );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_reconnect_if_down( MX_RECORD *server_record )
{
	static const char fname[] = "mx_network_reconnect_if_down()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Record pointer passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	if ( server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		/* You cannot reconnect to a disabled server. */

		return MX_SUCCESSFUL_RESULT;
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->reconnect_if_down;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"reconnect_if_down function pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = ( *fptr ) ( server );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_message_is_available( MX_RECORD *server_record,
				mx_bool_type *message_is_available )
{
	static const char fname[] = "mx_network_message_is_available()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	mx_status_type ( *fptr ) ( MX_NETWORK_SERVER *, mx_bool_type * );
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}
	if ( message_is_available == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"message_is_available pointer is NULL." );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	if ( server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		/* Disabled servers _never_ have messages available. */

		*message_is_available = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
			(server_record->class_specific_function_list);

	if ( function_list == (MX_NETWORK_SERVER_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SERVER_FUNCTION_LIST pointer for server record '%s' is NULL.",
			server_record->name );
	}

	fptr = function_list->message_is_available;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"message_is_available function pointer for server record '%s' is NULL.",
			server_record->name );
	}

	mx_status = ( *fptr ) ( server, message_is_available );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_mark_handles_as_invalid( MX_RECORD *server_record )
{
	static const char fname[] = "mx_network_mark_handles_as_invalid()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_FIELD *nf;
	unsigned long i;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	/* If the server is disabled, then it will not have any handles. */

	if ( server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		server->network_handles_are_valid = FALSE;
		return MX_SUCCESSFUL_RESULT;
	}

	/* See if the handles have already been marked invalid. */

	if ( server->network_handles_are_valid == FALSE )
		return MX_SUCCESSFUL_RESULT;

	/* Otherwise, walk through the list and mark the handles as invalid. */

	for ( i = 0; i < server->num_network_fields; i++ ) {
		nf = server->network_field_array[i];

		if ( nf != (MX_NETWORK_FIELD *) NULL ) {
			MX_DEBUG( 2,
		("%s: Marking handle as invalid, network_field '%s' (%ld,%ld)",
				fname, nf->nfname,
				nf->record_handle, nf->field_handle));

			nf->record_handle = MX_ILLEGAL_HANDLE;
			nf->field_handle = MX_ILLEGAL_HANDLE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

static mx_status_type
mxp_restore_network_callback( MX_RECORD *server_record,
				MX_CALLBACK *callback )
{
	static const char fname[] = "mxp_restore_network_callback()";

	MX_NETWORK_SERVER *network_server;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	MX_NETWORK_FIELD *nf;
	uint32_t *header, *uint32_message;
	char *char_message;
	unsigned long header_length, message_length, message_type;
	long status_code;
	unsigned long data_type, message_id;
	mx_bool_type connected;
	mx_status_type mx_status;

	network_server = server_record->record_class_struct;

	nf = callback->u.network_field;

#if 0
	MX_DEBUG(-2,("%s invoked for server '%s', nf '%s'.",
		fname, server_record->name, nf->nfname));
#endif

	/* Make sure that the network field is connected. */

	mx_status = mx_network_field_is_connected( nf, &connected );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( connected == FALSE ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Find the message buffer for this server. */

	message_buffer = network_server->message_buffer;

	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_MESSAGE_BUFFER for server '%s' is NULL.",
			server_record->name );
	}

	/* Construct a message to the server asking it to add a callback. */

	/*--- First, construct the header. ---*/

	header = message_buffer->u.uint32_buffer;
	header_length = network_server->remote_header_length;

	header[MX_NETWORK_MAGIC]         = mx_htonl(MX_NETWORK_MAGIC_VALUE);
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl(header_length);
	header[MX_NETWORK_MESSAGE_TYPE]  = mx_htonl(MX_NETMSG_ADD_CALLBACK);
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl(MXE_SUCCESS);
	header[MX_NETWORK_DATA_TYPE]     = mx_htonl(MXFT_ULONG);

	mx_network_update_message_id( &(network_server->last_rpc_message_id) );

	header[MX_NETWORK_MESSAGE_ID] =
			mx_htonl(network_server->last_rpc_message_id);

	/*--- Then, fill in the message body. ---*/

	uint32_message = header
	    + ( mx_remote_header_length(network_server) / sizeof(uint32_t) );

	uint32_message[0] = mx_htonl( nf->record_handle );
	uint32_message[1] = mx_htonl( nf->field_handle );
	uint32_message[2] = mx_htonl( callback->supported_callback_types );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( 3 * sizeof(uint32_t) );

	/******** Now send the message. ********/

	mx_status = mx_network_send_message( server_record, message_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/******** Wait for the response. ********/

	mx_status = mx_network_wait_for_message_id(
					server_record,
					message_buffer,
					network_server->last_rpc_message_id,
					network_server->timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* These pointers may have been changed by buffer reallocation
	 * in mx_network_wait_for_message_id(), so we read them again.
	 */

	header = message_buffer->u.uint32_buffer;

	uint32_message = header
	    + ( mx_remote_header_length(network_server) / sizeof(uint32_t) );

	char_message = message_buffer->u.char_buffer
				+ mx_remote_header_length(network_server);

	/* Read the header of the returned message. */

	header_length  = mx_ntohl( header[MX_NETWORK_HEADER_LENGTH] );
	message_length = mx_ntohl( header[MX_NETWORK_MESSAGE_LENGTH] );
	message_type   = mx_ntohl( header[MX_NETWORK_MESSAGE_TYPE] );
	status_code = (long) mx_ntohl( header[MX_NETWORK_STATUS_CODE] );

	MXW_UNUSED( message_length );

	if ( message_type != mx_server_response(MX_NETMSG_ADD_CALLBACK) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Message type for response was not %#lx.  "
		"Instead it was of type = %#lx.",
		    (unsigned long) mx_server_response(MX_NETMSG_ADD_CALLBACK),
		    (unsigned long) message_type );
	}

	/* If we got an error status code from the server, return the
	 * error message to the caller.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( status_code, fname, "%s", char_message );
	}

	/* If the header is not at least 28 bytes long, then the server
	 * is too old to support callbacks.
	 */

	if ( header_length < 28 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"Server '%s' does not support callbacks.",
			server_record->name );
	}

	data_type  = mx_ntohl( header[MX_NETWORK_DATA_TYPE] );
	message_id = mx_ntohl( header[MX_NETWORK_MESSAGE_ID] );

	MXW_UNUSED( data_type );
	MXW_UNUSED( message_id );

	/* Get the new callback ID from the returned message. */

	callback->callback_id = mx_ntohl( uint32_message[0] );

#if 0
	MX_DEBUG(-2,("%s: callback_id = %#lx",
		fname, (unsigned long) callback->callback_id));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* ---------------------------------------------------------------------- */

MX_EXPORT mx_status_type
mx_network_restore_callbacks( MX_RECORD *server_record )
{
	static const char fname[] = "mx_network_restore_callbacks()";

	MX_NETWORK_SERVER *network_server;
	MX_LIST *callback_list;
	MX_LIST_ENTRY *list_start, *list_entry;
	MX_CALLBACK *callback;
	MX_RECORD_FIELD *rf;
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	network_server = server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	if ( network_server->server_flags & MXF_NETWORK_SERVER_DISABLED ) {
		/* You cannot restore callbacks for a disabled server. */

		return MX_SUCCESSFUL_RESULT;
	}

	callback_list = network_server->callback_list;

	if ( callback_list == (MX_LIST *) NULL ) {
		/* There are no callbacks to restore. */

		return MX_SUCCESSFUL_RESULT;
	}

	list_start = callback_list->list_start;

	if ( list_start == (MX_LIST_ENTRY *) NULL ) {
		/* There are no callbacks to restore. */

		return MX_SUCCESSFUL_RESULT;
	}

	list_entry = list_start;

	do {
		callback = list_entry->list_entry_data;

		switch( callback->callback_class ) {
		case MXCBC_NETWORK:
			mx_status = mxp_restore_network_callback(
						server_record, callback );

			MXW_UNUSED( mx_status );
			break;
		case MXCBC_FIELD:
			rf = callback->u.record_field;

			MX_DEBUG(-2,("%s: rf = '%s'", fname, rf->name));
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported callback class %lu for callback %p "
			"used by server record '%s'.",
				callback->callback_class, callback,
				server_record->name );
			break;
		}

		list_entry = list_entry->next_list_entry;

	} while ( list_entry != list_start );

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

#define MXP_MAX_DISPLAY_VALUES    20

MX_EXPORT void
mx_network_buffer_show_value( void *buffer,
				unsigned long data_format,
				uint32_t data_type,
				uint32_t message_type,
				uint32_t message_length,
				mx_bool_type use_64bit_network_longs )
{
	static const char fname[] = "mx_network_buffer_show_value()";

	long i, raw_display_values, max_display_values;
	size_t scalar_element_size;
	void *raw_buffer;
	unsigned long raw_buffer_length;
	long num_dimensions;
	long dimension[1];
	size_t data_element_size[1];
	size_t xdr_scalar_element_size, xdr_transfer_length;
	uint32_t raw_xdr_array_length, xdr_array_length;
	uint32_t modified_xdr_array_length;
	uint32_t *uint32_buffer;
	mx_bool_type xdr_buffer_is_array, xdr_array_length_was_patched;
	mx_bool_type little_endian_byteorder;
	mx_status_type mx_status;

	/* Initialize some variables. */

	raw_xdr_array_length = 0;
	xdr_scalar_element_size = 0;
	xdr_buffer_is_array = FALSE;
	uint32_buffer = NULL;

	/* For GET_ARRAY and PUT_ARRAY in ASCII format, we treat the body
	 * of the message as a single string.
	 */

	if ( data_format == MX_NETWORK_DATAFMT_ASCII ) {
		switch( message_type ) {
		case MX_NETMSG_GET_ARRAY_BY_NAME:
		case MX_NETMSG_GET_ARRAY_BY_HANDLE:
		case MX_NETMSG_PUT_ARRAY_BY_NAME:
		case MX_NETMSG_PUT_ARRAY_BY_HANDLE:
		case mx_server_response(MX_NETMSG_GET_ARRAY_BY_NAME):
		case mx_server_response(MX_NETMSG_GET_ARRAY_BY_HANDLE):
		case mx_server_response(MX_NETMSG_PUT_ARRAY_BY_NAME):
		case mx_server_response(MX_NETMSG_PUT_ARRAY_BY_HANDLE):
			fprintf( stderr, "%s\n", (char *) buffer );

			/* At this point, we are done.  For ASCII format,
			 * message types not mentioned above will be
			 * handled together with RAW format.
			 */

			return;
		}
	}

	scalar_element_size = mx_get_scalar_element_size( (long) data_type,
						use_64bit_network_longs );

	if ( scalar_element_size == 0 ) {
		fprintf( stderr, "*** Unknown type %lu ***\n",
			(unsigned long) data_type );

		return;
	}

#if 0
	MX_DEBUG(-2,
		("%s: scalar_element_size = %d, use_64bit_network_longs = %d",
		fname, (int) scalar_element_size, use_64bit_network_longs ));
#endif

	/* Figure out how many values will be displayed. */

	if ( data_format != MX_NETWORK_DATAFMT_XDR ) {
		if ( use_64bit_network_longs ) {
			raw_display_values =
				(long) (message_length / scalar_element_size);
		} else {
			switch( data_type ) {
			case MXFT_LONG:
			case MXFT_ULONG:
			case MXFT_HEX:
				raw_display_values =
					(long) (message_length / 4L);
				break;
			default:
				raw_display_values =
				  (long) (message_length / scalar_element_size);
				break;
			}
		}
	} else {
		/* For XDR, figuring out the number of values to display
		 * takes a little bit of work.
		 */

		xdr_scalar_element_size =
			mx_xdr_get_scalar_element_size( (long) data_type );

		if ( message_length < xdr_scalar_element_size ) {
			(void) mx_error( MXE_NETWORK_IO_ERROR, fname,
			"The length (%ld) of the XDR message is less "
			"than the minimum allowed length (%ld) for "
			"MX data type %ld.",
				(long) message_length,
				(long) xdr_scalar_element_size,
				(long) data_type );
			return;
		} else
		if ( message_length == xdr_scalar_element_size ) {
			xdr_buffer_is_array = FALSE;
			raw_display_values = 1;
		} else {
			xdr_buffer_is_array = TRUE;

			/* The first 4 bytes of the message contain the
			 * array length, so we must skip over them.
			 */

			raw_display_values = (long)
			    ((message_length - 4) / xdr_scalar_element_size);
		}
	}

	if ( data_type == MXFT_STRING ) {
		max_display_values = raw_display_values;
	} else {
		if ( raw_display_values > MXP_MAX_DISPLAY_VALUES ) {
			max_display_values = MXP_MAX_DISPLAY_VALUES;
		} else {
			max_display_values = raw_display_values;
		}
	}

	i = -1;

	switch( data_format ) {
	case MX_NETWORK_DATAFMT_ASCII:
	case MX_NETWORK_DATAFMT_RAW:
	case MX_NETWORK_DATAFMT_XDR:

		/* XDR data must be converted to raw data
		 * before it can be displayed.
		 */

		if ( data_format == MX_NETWORK_DATAFMT_XDR ) {
#if 0
			unsigned char u;

			fprintf( stderr, "XDR buffer = " );

			for ( i = 0; i < message_length; i++ ) {
				u = ((char *)buffer)[i];

				fprintf( stderr, "%#x ", u );
			}

			fprintf( stderr, "\n" );
#endif

			if ( mx_native_byteorder() == MX_DATAFMT_LITTLE_ENDIAN )
			{
				little_endian_byteorder = TRUE;
			} else {
				little_endian_byteorder = FALSE;
			}

			/* Create a raw buffer which will be used as the
			 * destination of a call to mx_xdr_data_transfer().
			 */

			raw_buffer_length =
				scalar_element_size * max_display_values;

			raw_buffer = malloc( raw_buffer_length );

			if ( raw_buffer == NULL ) {
				(void) mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to allocate "
				"a %lu byte XDR conversion buffer.",
					raw_buffer_length );	
				return;
			}

			data_element_size[0] = scalar_element_size;

			xdr_array_length_was_patched = FALSE;

			if ( xdr_buffer_is_array == FALSE ) {
				num_dimensions = 0;
				dimension[0] = 0;

				xdr_transfer_length = xdr_scalar_element_size;
			} else {
				num_dimensions = 1;
				dimension[0] = max_display_values;

				xdr_transfer_length = 4 +
				  max_display_values * xdr_scalar_element_size;
				
				/* If the XDR data is an array, then the first
				 * 4-bytes of the XDR data contains the length
				 * of the array in bigendian format.
				 */

				uint32_buffer = buffer;

				raw_xdr_array_length = *uint32_buffer;

				if ( little_endian_byteorder ) {
					xdr_array_length =
				    mx_32bit_byteswap( raw_xdr_array_length );
				} else {
					xdr_array_length = raw_xdr_array_length;
				}

				/* WARNING WARNING WARNING WARNING WARNING
				 *
				 * If the XDR array length is longer than the
				 * number of values that we plan to display,
				 * then we must patch the XDR data buffer's
				 * array length value to match the number
				 * of values that we plan to display.
				 *
				 * We _MUST_ restore the original value before
				 * returning from this routine.
				 */

				if ( xdr_array_length > max_display_values ) {
					xdr_array_length_was_patched = TRUE;

					modified_xdr_array_length =
						max_display_values;

					if ( little_endian_byteorder ) {
						modified_xdr_array_length =
						  mx_32bit_byteswap(
						    modified_xdr_array_length );
					}

					/* Patch the array length. */

					*uint32_buffer =
						modified_xdr_array_length;
				}
			}

			/* Perform the XDR data conversion. */

			mx_status = mx_xdr_data_transfer( MX_XDR_DECODE,
					raw_buffer, FALSE,
					(long) data_type, num_dimensions,
					dimension, data_element_size,
					buffer, xdr_transfer_length, NULL );

			if ( xdr_array_length_was_patched ) {
				/* Restore the array length. */

				*uint32_buffer = raw_xdr_array_length;
			}

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_free( raw_buffer );
				return;
			}
		} else {
			raw_buffer = buffer;
		}

		switch( data_type ) {
		case MXFT_STRING:
			fprintf( stderr, "'%s' ", ((char *) raw_buffer) );
			break;
		case MXFT_CHAR:
		case MXFT_UCHAR:
			for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%#x ",
					((unsigned char *) raw_buffer)[i] );
			}
			break;
		case MXFT_SHORT:
			for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%hd ",
					((short *) raw_buffer)[i] );
			}
			break;
		case MXFT_USHORT:
			for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%hu ",
					((unsigned short *) raw_buffer)[i] );
			}
			break;
		case MXFT_BOOL:
			for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%d ",
				(int)(((mx_bool_type *) raw_buffer)[i]) );
			}
			break;
		case MXFT_LONG:
			if ( use_64bit_network_longs ) {
			    for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%" PRId64 " ",
					((int64_t *) raw_buffer)[i] );
			    }
			} else {
#if ( MX_WORDSIZE == 32 )
			    for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%ld ",
				    (0xffffffff & ((long *) raw_buffer)[i]) );
			    }
#elif ( MX_WORDSIZE == 64 )
			    for ( i = 0; i < max_display_values; i++ ) {
				int32_t int32_value;

				int32_value = 
				    (0xffffffff & ((long *) raw_buffer)[i]);

				fprintf( stderr, "%ld ", (long) int32_value );
			    }
#else
#  error Unsupported MX_WORDSIZE value.
#endif
			}
			break;
		case MXFT_ULONG:
			if ( use_64bit_network_longs ) {
			    for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%" PRIu64 " ",
					((uint64_t *) raw_buffer)[i] );
			    }
			} else {
			    for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%lu ",
			    (0xffffffff & ((unsigned long *) raw_buffer)[i]) );
			    }
			}
			break;
		case MXFT_FLOAT:
			for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%g ",
					((float *) raw_buffer)[i] );
			}
			break;
		case MXFT_DOUBLE:
			for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%g ",
					((double *) raw_buffer)[i] );
			}
			break;
		case MXFT_HEX:
			if ( use_64bit_network_longs ) {
			    for ( i = 0; i < max_display_values; i++ ) {
				uint64_t hex_value;

				hex_value = ((uint64_t *) raw_buffer)[i];

#if ( defined(OS_VMS) && defined(__VAX) )
				hex_value &= 0xffffffff;
#else
				hex_value &= UINT64_C( 0xffffffffffffffff );
#endif

				fprintf( stderr, "%#" PRIx64 " ", hex_value );
			    }
			} else {
			    for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%#lx ",
			    (0xffffffff & ((unsigned long *) raw_buffer)[i]) );
			    }
			}
			break;
		case MXFT_INT64:
			for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%" PRId64 " ",
					((int64_t *) raw_buffer)[i] );
			}
			break;
		case MXFT_UINT64:
			for ( i = 0; i < max_display_values; i++ ) {
				fprintf( stderr, "%" PRIu64 " ",
					((uint64_t *) raw_buffer)[i] );
			}
			break;
		case MXFT_RECORD:
		case MXFT_RECORDTYPE:
		case MXFT_INTERFACE:
		case MXFT_RECORD_FIELD:
#if 0
			(void) mx_error( MXE_UNSUPPORTED, fname,
				"Unsupported data type %lu requested.",
				(unsigned long) data_type );
			return;
#else
			fprintf( stderr, "'%s' ", ((char *) raw_buffer) );
#endif
			break;
		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Unrecognized data type %lu requested.",
				(unsigned long) data_type );
			return;
		}

		if ( data_format == MX_NETWORK_DATAFMT_XDR ) {
			mx_free( raw_buffer );
		}
		break;

	default:
		(void) mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported data format %ld", data_format );
		return;
	}

	if ( raw_display_values > max_display_values ) {
		fprintf( stderr, "... " );
	}

	return;
}

/*------------------------------------------------------------------------*/

MX_EXPORT void
mx_network_display_message( MX_NETWORK_MESSAGE_BUFFER *message_buffer,
				MX_RECORD_FIELD *record_field,
				mx_bool_type use_64bit_network_longs )
{
	static const char fname[] = "mx_network_display_message()";

	uint32_t *header, *uint32_message;
	char *buffer, *char_message;

	union {
		double double_value;
		uint32_t uint32_value[2];
	} u;

	uint32_t magic_number, header_length, message_length;
	uint32_t message_type, status_code, message_id;
	uint32_t record_handle, field_handle;
	long i, data_type, field_type, num_dimensions, dimension_size;
	unsigned long option_number, option_value;
	unsigned long attribute_number;
	double attribute_value;
	unsigned long callback_type, callback_id;
	const char *data_type_name;

	if ( message_buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		    "MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );
		return;
	}

	buffer = message_buffer->u.char_buffer;
	header = message_buffer->u.uint32_buffer;

	magic_number   = mx_ntohl( header[ MX_NETWORK_MAGIC ] );
	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code    = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	MXW_UNUSED( magic_number );

	if ( header_length < 28 ) {
		/* Handle servers and clients from MX 1.4 and before. */

		data_type  = 0;
		message_id = 0;
	} else {
		data_type  = (long) mx_ntohl( header[ MX_NETWORK_DATA_TYPE ] );
		message_id = mx_ntohl( header[ MX_NETWORK_MESSAGE_ID ] );
	}

	uint32_message = header + header_length / 4;
	char_message   = buffer + header_length;

	fprintf( stderr,
"  header length = %ld, message_length = %ld, message type = %#lx,\n",
		(long) header_length, (long) message_length,
		(long) message_type );

	data_type_name = mx_get_field_type_string( data_type );

	if ( strcmp( data_type_name, "MXFT_UNKNOWN" ) == 0 ) {
		fprintf( stderr,
"  status code = %ld, data type = 'Unknown type %ld', message id = %#lx\n",
			(long) status_code, data_type,
			(unsigned long) message_id );
	} else {
		fprintf( stderr,
"  status code = %ld, data type = %s, message id = %#lx\n",
			(long) status_code, data_type_name,
			(unsigned long) message_id );
	}

	switch( message_type ) {
	case MX_NETMSG_UNEXPECTED_ERROR:
		fprintf( stderr, "  UNEXPECTED_ERROR: '%s'\n", char_message );
		break;

	case MX_NETMSG_GET_ARRAY_BY_NAME:
		fprintf( stderr, "  GET_ARRAY_BY_NAME: '%s'\n", char_message );
		break;

	case MX_NETMSG_GET_ARRAY_BY_HANDLE:
		record_handle = mx_ntohl( uint32_message[0] );
		field_handle  = mx_ntohl( uint32_message[1] );

		fprintf( stderr, "  GET_ARRAY_BY_HANDLE: (%lu,%lu)\n",
				(unsigned long) record_handle,
				(unsigned long) field_handle );
		break;

	case mx_server_response(MX_NETMSG_GET_ARRAY_BY_NAME):
		fprintf( stderr, "  GET_ARRAY_BY_NAME response = " );

		mx_network_buffer_show_value( uint32_message,
					message_buffer->data_format,
					data_type,
					message_type,
					message_length,
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	case mx_server_response(MX_NETMSG_GET_ARRAY_BY_HANDLE):

		if ( ( record_field != NULL )
		  && ( record_field->record != NULL ) )
		{
			fprintf( stderr,
			"  GET_ARRAY_BY_HANDLE '%s.%s' response = ",
				record_field->record->name,
				record_field->name );
		} else {
			fprintf( stderr, "  GET_ARRAY_BY_HANDLE response = " );
		}

		mx_network_buffer_show_value( uint32_message,
					message_buffer->data_format,
					data_type,
					message_type,
					message_length,
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_PUT_ARRAY_BY_NAME:
		fprintf( stderr, "  PUT_ARRAY_BY_NAME: '%s' value = ",
				char_message );

		mx_network_buffer_show_value(
				char_message + MXU_RECORD_FIELD_NAME_LENGTH,
					message_buffer->data_format,
					data_type,
					message_type,
					message_length,
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	case MX_NETMSG_PUT_ARRAY_BY_HANDLE:
		record_handle = mx_ntohl( uint32_message[0] );
		field_handle  = mx_ntohl( uint32_message[1] );

		fprintf( stderr, "  PUT_ARRAY_BY_HANDLE: (%lu,%lu) value = ",
				(unsigned long) record_handle,
				(unsigned long) field_handle );

		mx_network_buffer_show_value( &(uint32_message[2]),
					message_buffer->data_format,
					data_type,
					message_type,
					message_length - 2 * sizeof(uint32_t),
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	case mx_server_response(MX_NETMSG_PUT_ARRAY_BY_NAME):
		fprintf( stderr, "  PUT_ARRAY_BY_NAME response\n" );
		break;

	case mx_server_response(MX_NETMSG_PUT_ARRAY_BY_HANDLE):

		if ( ( record_field != NULL )
		  && ( record_field->record != NULL ) )
		{
			fprintf( stderr,
			"  PUT_ARRAY_BY_HANDLE '%s.%s' response\n",
				record_field->record->name,
				record_field->name );
		} else {
			fprintf( stderr, "  PUT_ARRAY_BY_HANDLE response\n" );
		}
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_GET_NETWORK_HANDLE:
		fprintf( stderr, "  GET_NETWORK_HANDLE: '%s'\n", char_message );
		break;

	case mx_server_response(MX_NETMSG_GET_NETWORK_HANDLE):
		record_handle = mx_ntohl( uint32_message[0] );
		field_handle  = mx_ntohl( uint32_message[1] );

		fprintf( stderr, "  GET_NETWORK_HANDLE response: (%lu,%lu)\n",
				(unsigned long) record_handle,
				(unsigned long) field_handle );
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_GET_FIELD_TYPE:
		fprintf( stderr, "  GET_FIELD_TYPE: '%s'\n", char_message );
		break;

	case mx_server_response(MX_NETMSG_GET_FIELD_TYPE):
		field_type     = (long) mx_ntohl( uint32_message[0] );
		num_dimensions = (long) mx_ntohl( uint32_message[1] );

		fprintf( stderr,
	"  GET_FIELD_TYPE response: field type = %ld, num dimensions = %ld",
			field_type, num_dimensions );

		for ( i = 0; i < num_dimensions; i++ ) {
			dimension_size = (long) mx_ntohl( uint32_message[i+2] );

			fprintf( stderr, ", %ld", dimension_size );
		}
		fprintf( stderr, "\n" );
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_SET_CLIENT_INFO:
		fprintf( stderr, "  SET_CLIENT_INFO: '%s'\n", char_message );
		break;

	case mx_server_response(MX_NETMSG_SET_CLIENT_INFO):
		fprintf( stderr, "  SET_CLIENT_INFO response\n" );
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_GET_OPTION:
		option_number = mx_ntohl( uint32_message[0] );

		fprintf( stderr, "  GET_OPTION: option number = %lu\n",
				option_number );
		break;

	case mx_server_response(MX_NETMSG_GET_OPTION):
		option_value = mx_ntohl( uint32_message[0] );

		fprintf( stderr, "  GET_OPTION: returned option value = %lu\n",
							option_value );
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_SET_OPTION:
		option_number = mx_ntohl( uint32_message[0] );
		option_value  = mx_ntohl( uint32_message[1] );

		fprintf( stderr,
		"  SET_OPTION: option number = %lu, option value = %lu\n",
			option_number, option_value );
		break;

	case mx_server_response(MX_NETMSG_SET_OPTION):
		if ( char_message[0] == '\0' ) {
			fprintf( stderr,
			"  SET_OPTION response: Option change accepted\n" );
		} else {
			fprintf( stderr,
			"  SET_OPTION response: %s\n", char_message );
		}
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_GET_ATTRIBUTE:
		record_handle    = mx_ntohl( uint32_message[0] );
		field_handle     = mx_ntohl( uint32_message[1] );
		attribute_number = mx_ntohl( uint32_message[2] );

		fprintf( stderr,
		"  GET_ATTRIBUTE: (%lu,%lu) attribute number = %lu\n",
				(unsigned long) record_handle,
				(unsigned long) field_handle,
				attribute_number );
		break;

	case mx_server_response(MX_NETMSG_GET_ATTRIBUTE):
		/* Do _not_ use mx_ntohl() for the attribute value, since
		 * the raw value is actually an IEEE 754 double.
		 */

		u.uint32_value[0] = uint32_message[0];
		u.uint32_value[1] = uint32_message[1];

		attribute_value = u.double_value;

		fprintf( stderr,
		"  GET_ATTRIBUTE: returned attribute value = %g\n",
							attribute_value );
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_SET_ATTRIBUTE:
		record_handle    = mx_ntohl( uint32_message[0] );
		field_handle     = mx_ntohl( uint32_message[1] );
		attribute_number = mx_ntohl( uint32_message[2] );

		/* Do _not_ use mx_ntohl() for the attribute value, since
		 * the raw value is actually an IEEE 754 double.
		 */

		u.uint32_value[0] = uint32_message[3];
		u.uint32_value[1] = uint32_message[4];

		attribute_value = u.double_value;

		fprintf( stderr,
    "  SET_ATTRIBUTE: (%lu,%lu) attribute number = %lu, attribute value = %g\n",
			(unsigned long) record_handle,
			(unsigned long) field_handle,
			attribute_number, attribute_value );
		break;

	case mx_server_response(MX_NETMSG_SET_ATTRIBUTE):
		if ( char_message[0] == '\0' ) {
			fprintf( stderr,
		"  SET_ATTRIBUTE response: Attribute change accepted\n" );
		} else {
			fprintf( stderr,
			"  SET_ATTRIBUTE response: %s\n", char_message );
		}
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_ADD_CALLBACK:
		record_handle = mx_ntohl( uint32_message[0] );
		field_handle  = mx_ntohl( uint32_message[1] );
		callback_type = mx_ntohl( uint32_message[2] );

		fprintf( stderr,
		"  ADD_CALLBACK: (%lu,%lu) callback type = ",
			(unsigned long) record_handle,
			(unsigned long) field_handle );

		switch ( callback_type ) {
		case MXCBT_VALUE_CHANGED:
			fprintf( stderr, "MXCBT_VALUE_CHANGED\n" );
			break;
		case MXCBT_POLL:
			fprintf( stderr, "MXCBT_POLL\n" );
			break;
		default:
			fprintf( stderr, "%lu\n", callback_type );
			break;
		}

		break;

	case mx_server_response(MX_NETMSG_ADD_CALLBACK):
		callback_id = mx_ntohl( uint32_message[0] );

		fprintf( stderr,
		"  ADD_CALLBACK response: callback id = %#lx\n", callback_id );
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_DELETE_CALLBACK:
		callback_id = mx_ntohl( uint32_message[0] );

		fprintf( stderr,
		"  DELETE_CALLBACK: callback_id = %#lx\n", callback_id );
		break;

	case mx_server_response(MX_NETMSG_DELETE_CALLBACK):
		fprintf( stderr,
		"  DELETE_CALLBACK response.\n" );
		break;

	/*-------------------------------------------------------------------*/

	case mx_server_response(MX_NETMSG_CALLBACK):
		fprintf( stderr,
		"  *** CALLBACK *** from server: callback id = %#lx, value = ",
			(unsigned long) message_id );

		mx_network_buffer_show_value( uint32_message,
					message_buffer->data_format,
					data_type,
					message_type,
					message_length,
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	/*-------------------------------------------------------------------*/

	default:
		mx_warning( "%s: Unrecognized message type %#lx",
			fname, (unsigned long) message_type );
	}

	return;
}

/*------------------------------------------------------------------------*/

#define NF_LABEL_LENGTH \
	  ( MXU_HOSTNAME_LENGTH + MXU_RECORD_FIELD_NAME_LENGTH + 8 )

#if 1

MX_EXPORT void
mx_network_display_summary( MX_NETWORK_MESSAGE_BUFFER *message_buffer,
				MX_NETWORK_FIELD *network_field,
				MX_RECORD *server_record,
				char *remote_record_field_name,
		 		MX_RECORD_FIELD *record_field,
				mx_bool_type use_64bit_network_longs )
{
	static const char fname[] = "mx_network_display_summary()";

	uint32_t *header, *uint32_message;
	char *buffer, *char_message;

	uint32_t magic_number, header_length, message_length;
	uint32_t message_type, status_code, message_id;
	uint32_t record_handle, field_handle;
	long data_type;
	char nf_label[ NF_LABEL_LENGTH ];

#if 0
	union {
		double double_value;
		uint32_t uint32_value[2];
	} u;

	long i, data_type, field_type, num_dimensions, dimension_size;
	unsigned long option_number, option_value;
	unsigned long attribute_number;
	double attribute_value;
	unsigned long callback_type, callback_id;
#endif

	if ( message_buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		    "MX_NETWORK_MESSAGE_BUFFER pointer passed was NULL." );
		return;
	}

	mx_network_get_remote_field_label( network_field,
					server_record,
					remote_record_field_name,
					record_field,
					nf_label, sizeof(nf_label) );

	buffer = message_buffer->u.char_buffer;
	header = message_buffer->u.uint32_buffer;

	magic_number   = mx_ntohl( header[ MX_NETWORK_MAGIC ] );
	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code    = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	MXW_UNUSED( magic_number );
	MXW_UNUSED( status_code );

	if ( header_length < 28 ) {
		/* Handle servers and clients from MX 1.4 and before. */

		data_type  = 0;
		message_id = 0;
	} else {
		data_type  = (long) mx_ntohl( header[ MX_NETWORK_DATA_TYPE ] );
		message_id = mx_ntohl( header[ MX_NETWORK_MESSAGE_ID ] );
	}

	MXW_UNUSED( message_id );

	uint32_message = header + header_length / 4;
	char_message   = buffer + header_length;

	switch( message_type ) {
	case MX_NETMSG_GET_ARRAY_BY_NAME:
	case MX_NETMSG_GET_ARRAY_BY_HANDLE:
		/* Display nothing. */
		break;

	case mx_server_response(MX_NETMSG_GET_ARRAY_BY_NAME):
		if ( ( record_field != NULL )
		  && ( record_field->record != NULL ) )
		{
			fprintf( stderr, "MX get_array_by_name('%s.%s') = ",
				record_field->record->name,
				record_field->name );
		} else {
			fprintf( stderr, "MX get_array_by_name('\?\?\?') = " );
		}

		mx_network_buffer_show_value( uint32_message,
					message_buffer->data_format,
					data_type,
					message_type,
					message_length,
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	case mx_server_response(MX_NETMSG_GET_ARRAY_BY_HANDLE):

		if ( ( record_field != NULL )
		  && ( record_field->record != NULL ) )
		{
			fprintf( stderr,
			"MX get_array('%s.%s') = ",
				record_field->record->name,
				record_field->name );
		} else {
			fprintf( stderr, "MX get_array('\?\?\?') = " );
		}

		mx_network_buffer_show_value( uint32_message,
					message_buffer->data_format,
					data_type,
					message_type,
					message_length,
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	/*-------------------------------------------------------------------*/

	case MX_NETMSG_PUT_ARRAY_BY_NAME:
		fprintf( stderr, "MX put_array_by_name('%s') = ",
				char_message );

		mx_network_buffer_show_value(
				char_message + MXU_RECORD_FIELD_NAME_LENGTH,
					message_buffer->data_format,
					data_type,
					message_type,
					message_length,
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	case MX_NETMSG_PUT_ARRAY_BY_HANDLE:
		record_handle = mx_ntohl( uint32_message[0] );
		field_handle  = mx_ntohl( uint32_message[1] );

		if ( ( record_field != NULL )
		  && ( record_field->record != NULL ) )
		{
			fprintf( stderr, "MX put_array('%s.%s') = ",
				record_field->record->name,
				record_field->name );
		} else {
			fprintf( stderr, "MX put_array(%lu,%lu) = ",
				(unsigned long) record_handle,
				(unsigned long) field_handle );
		}

		mx_network_buffer_show_value( &(uint32_message[2]),
					message_buffer->data_format,
					data_type,
					message_type,
					message_length - 2 * sizeof(uint32_t),
					use_64bit_network_longs );
		fprintf( stderr, "\n" );
		break;

	case mx_server_response(MX_NETMSG_PUT_ARRAY_BY_NAME):
	case mx_server_response(MX_NETMSG_PUT_ARRAY_BY_HANDLE):
		/* Display nothing. */
		break;

	/*-------------------------------------------------------------------*/

	default:
		mx_warning(
		"%s: Message type %#lx is not supported for summary.",
			fname, (unsigned long) message_type );

	}

	return;
}
#endif

/* ====================================================================== */

static mx_status_type
mx_network_field_get_parameters( MX_RECORD *server_record,
			MX_RECORD_FIELD *local_field,
			long *datatype,
			long *num_dimensions,
			long **dimension_array,
			size_t **data_element_size_array,
			MX_NETWORK_SERVER **server,
			MX_NETWORK_SERVER_FUNCTION_LIST **function_list,
			const char *calling_fname )
{
	static const char fname[] = "mx_network_field_get_parameters()";

	if ( server_record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record pointer passed to '%s' is NULL.",
			calling_fname );
	}
	if ( local_field == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"local_field pointer passed to '%s' is NULL.",
			calling_fname );
	}
	*datatype = local_field->datatype;

	switch( *datatype ) {
	case MXFT_STRING:
	case MXFT_CHAR:
	case MXFT_UCHAR:
	case MXFT_SHORT:
	case MXFT_USHORT:
	case MXFT_BOOL:
	case MXFT_LONG:
	case MXFT_ULONG:
	case MXFT_INT64:
	case MXFT_UINT64:
	case MXFT_FLOAT:
	case MXFT_DOUBLE:
	case MXFT_HEX:
	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
	case MXFT_RECORD_FIELD:
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Data type %ld passed to '%s' is not a legal network datatype.",
			*datatype, calling_fname );
	}

	*num_dimensions = local_field->num_dimensions;
	*dimension_array = local_field->dimension;
	*data_element_size_array = local_field->data_element_size;

	*server = (MX_NETWORK_SERVER *) (server_record->record_class_struct);
	*function_list = (MX_NETWORK_SERVER_FUNCTION_LIST *)
				(server_record->class_specific_function_list);

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_field_is_connected( MX_NETWORK_FIELD *nf,
				mx_bool_type *connected )
{
	static const char fname[] = "mx_network_field_is_connected()";

	MX_NETWORK_SERVER *network_server;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}
	if ( connected == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The connected pointer passed was NULL." );
	}
	if ( nf->server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The server_record pointer for the MX_NETWORK_FIELD "
		"pointer passed is NULL." );
	}

	network_server = (MX_NETWORK_SERVER *)
			nf->server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			nf->server_record->name );
	}

	if ( network_server->server_supports_network_handles == FALSE ) {
		*connected = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( ( nf->record_handle == MX_ILLEGAL_HANDLE )
	  || ( nf->field_handle == MX_ILLEGAL_HANDLE ) )
	{
		*connected = FALSE;
	} else {
		*connected = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_field_init( MX_NETWORK_FIELD *nf,
			MX_RECORD *server_record,
			char *name_format, ... )
{
	static const char fname[] = "mx_network_field_init()";

	va_list args;
	char buffer[1000];

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}
#if 0
	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The server_record pointer passed was NULL." );
	}
#endif
	if ( name_format == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The network field name format string pointer passed was NULL." );
	}

	va_start(args, name_format);
	vsnprintf(buffer, sizeof(buffer), name_format, args);
	va_end(args);

	/* Save the network field name. */

	strlcpy( nf->nfname, buffer, MXU_RECORD_FIELD_NAME_LENGTH );

	MX_DEBUG( 2,("%s invoked for network field '%s:%s'",
		fname, server_record->name, nf->nfname));

	/* Initialize the data structures. */

	nf->server_record = server_record;

	nf->record_handle = MX_ILLEGAL_HANDLE;
	nf->field_handle = MX_ILLEGAL_HANDLE;

	nf->local_field = NULL;
	nf->must_free_local_field_on_delete = FALSE;
	nf->do_not_copy_buffer_on_callback = FALSE;

	nf->application_ptr = NULL;

	if ( nf->server_record != (MX_RECORD *) NULL ) {
		/* Add this network field to the server's list of fields. */

		MX_NETWORK_SERVER *network_server;
		unsigned long num_fields, block_size, new_array_size;

		network_server = (MX_NETWORK_SERVER *)
					nf->server_record->record_class_struct;

		if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_NETWORK_SERVER pointer for record '%s' is NULL.",
				nf->server_record->name );
		}

		num_fields = network_server->num_network_fields;

		if ( (num_fields != 0 )
		  && (network_server->network_field_array == NULL) )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Network server record '%s' says that it has %lu "
			"network record fields, but the network_field_array "
			"pointer is NULL.", nf->server_record->name,
				num_fields );
		}

		block_size = network_server->network_field_array_block_size;

		if ( block_size == 0 ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The network_field_array_block_size for record '%s' "
			"is zero.  This should not happen.",
				nf->server_record->name );
		}

		if ( num_fields == 0 ) {
			network_server->network_field_array =
				(MX_NETWORK_FIELD **)
				malloc( block_size * sizeof(MX_NETWORK_FIELD));

			if ( network_server->network_field_array == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to allocate "
				"an array of %lu MX_NETWORK_FIELD pointers.",
					block_size );
			}
		} else
		if (( num_fields % block_size ) == 0) {
			new_array_size = num_fields + block_size;

			network_server->network_field_array =
				(MX_NETWORK_FIELD **)
				realloc( network_server->network_field_array,
				    new_array_size * sizeof(MX_NETWORK_FIELD));

			if ( network_server->network_field_array == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to resize "
				"an array of %lu MX_NETWORK_FIELD pointers.",
					new_array_size );
			}
		}

		network_server->network_field_array[ num_fields ] = nf;

		network_server->num_network_fields++;

		MX_DEBUG( 2,("%s: Added network field (%lu) '%s,%s'",
			fname, num_fields, nf->server_record->name,
			nf->nfname ));
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_get_array( MX_NETWORK_FIELD *nf,
		long datatype,
		long num_dimensions,
		long *dimension,
		void *value_ptr )
{
	static const char fname[] = "mx_get_array()";

	mx_bool_type connected;
	mx_status_type mx_status;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	mx_status = mx_network_field_is_connected( nf, &connected );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( connected == FALSE ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_internal_get_array( NULL, NULL, nf,
					datatype, num_dimensions, dimension,
					value_ptr );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_put_array( MX_NETWORK_FIELD *nf,
		long datatype,
		long num_dimensions,
		long *dimension,
		void *value_ptr )
{
	static const char fname[] = "mx_put_array()";

	mx_bool_type connected;
	mx_status_type mx_status;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	mx_status = mx_network_field_is_connected( nf, &connected );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( connected == FALSE ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_internal_put_array( NULL, NULL, nf,
					datatype, num_dimensions, dimension,
					value_ptr );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_internal_get_array( MX_RECORD *server_record,
		char *remote_record_field_name,
		MX_NETWORK_FIELD *nf,
		long datatype,
		long num_dimensions,
		long *dimension,
		void *value_ptr )
{
	static const char fname[] = "mx_internal_get_array()";

	MX_RECORD_FIELD local_temp_record_field;
	long *dimension_array;
	long local_dimension_array[MXU_FIELD_MAX_DIMENSIONS];
	size_t data_element_size[MXU_FIELD_MAX_DIMENSIONS];
	mx_status_type mx_status;

	if ( dimension != NULL ) {
		dimension_array = dimension;

	} else if ( num_dimensions == 0 ) {
		dimension_array = local_dimension_array;
		dimension_array[0] = 0;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"dimension array pointer is NULL, but num_dimensions (%ld) is greater than 0",
			num_dimensions );
	}

	/* Setting the first element of data_element_size to be zero causes
	 * mx_construct_temp_record_field() to use a default data element
	 * size array.
	 */

	data_element_size[0] = 0L;

	/* Setup a temporary record field to be used by
	 * mx_get_field_array().
	 */

	mx_status = mx_initialize_temp_record_field( &local_temp_record_field,
			datatype, num_dimensions, dimension_array,
			data_element_size, value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the request to the server. */

	mx_status = mx_get_field_array( server_record,
			remote_record_field_name,
			nf,
			&local_temp_record_field,
			value_ptr );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_internal_put_array( MX_RECORD *server_record,
		char *remote_record_field_name,
		MX_NETWORK_FIELD *nf,
		long datatype,
		long num_dimensions,
		long *dimension,
		void *value_ptr )
{
	static const char fname[] = "mx_internal_put_array()";

	MX_RECORD_FIELD local_temp_record_field;
	long *dimension_array;
	long local_dimension_array[MXU_FIELD_MAX_DIMENSIONS];
	size_t data_element_size[MXU_FIELD_MAX_DIMENSIONS];
	mx_status_type mx_status;

	if ( dimension != NULL ) {
		dimension_array = dimension;

	} else if ( num_dimensions == 0 ) {
		dimension_array = local_dimension_array;
		dimension_array[0] = 0;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"dimension array pointer is NULL, but num_dimensions (%ld) is greater than 0",
			num_dimensions );
	}

	/* Setting the first element of data_element_size to be zero causes
	 * mx_construct_temp_record_field() to use a default data element
	 * size array.
	 */

	data_element_size[0] = 0L;

	/* Setup a temporary record field to be used by
	 * mx_put_field_array().
	 */

	mx_status = mx_initialize_temp_record_field( &local_temp_record_field,
			datatype, num_dimensions, dimension_array,
			data_element_size, value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the request to the server. */

	mx_status = mx_put_field_array( server_record,
			remote_record_field_name,
			nf,
			&local_temp_record_field,
			value_ptr );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT char *
mx_network_get_nf_label( MX_RECORD *server_record,
			char *remote_record_field_name,
			char *label, size_t max_label_length )
{
	MX_TCPIP_SERVER *tcpip_server;
	MX_UNIX_SERVER *unix_server;

	if ( label == NULL )
		return NULL;

	if ( server_record == NULL ) {
		if ( remote_record_field_name == NULL ) {
			strlcpy( label, "", max_label_length );
		} else {
			strlcpy( label, remote_record_field_name,
				max_label_length );
		}

		return label;
	}

	switch( server_record->mx_type ) {
	case MXN_NET_TCPIP:
		tcpip_server = server_record->record_type_struct;

		if ( remote_record_field_name == NULL ) {
			snprintf( label, max_label_length,
				"%s@%ld",
				tcpip_server->hostname,
				tcpip_server->port );
		} else {
			snprintf( label, max_label_length,
				"%s@%ld:%s",
				tcpip_server->hostname,
				tcpip_server->port,
				remote_record_field_name );
		}
		break;
	case MXN_NET_UNIX:
		unix_server = server_record->record_type_struct;

		if ( remote_record_field_name == NULL ) {
			snprintf( label, max_label_length,
				"%s",
				unix_server->pathname );
		} else {
			snprintf( label, max_label_length,
				"%s:%s",
				unix_server->pathname,
				remote_record_field_name );
		}
		break;
	default:
		if ( remote_record_field_name == NULL ) {
			snprintf( label, max_label_length,
				"unknown server type %ld",
				server_record->mx_type );
		} else {
			snprintf( label, max_label_length,
				"unknown server type %ld:%s",
				server_record->mx_type,
				remote_record_field_name );
		}
		break;
	}

	return label;
}

/* ====================================================================== */

MX_EXPORT char *
mx_network_get_remote_field_label( MX_NETWORK_FIELD *nf,
				MX_RECORD *server_record,
				char *remote_record_field_name,
				MX_RECORD_FIELD *record_field,
				char *nf_label, size_t max_nf_label_length )
{
	if ( nf_label == NULL ) {
		return NULL;
	}

	if ( nf != NULL ) {
		mx_network_get_nf_label( nf->server_record, nf->nfname,
					nf_label, max_nf_label_length );
	} else {
		if ( remote_record_field_name == NULL ) {
			if ( ( record_field == NULL )
			  || ( record_field->record == NULL ) )
			{
				char unknown_field_name[] = "\?\?\?";

				mx_network_get_nf_label( server_record,
							unknown_field_name,
						nf_label, max_nf_label_length );
			} else {
				char rf_name[MXU_RECORD_FIELD_NAME_LENGTH+1];

				snprintf( rf_name, sizeof(rf_name),
				"%s.%s", record_field->record->name,
					record_field->name );

				mx_network_get_nf_label( server_record, rf_name,
						nf_label, max_nf_label_length );
			}
		} else {
			mx_network_get_nf_label( server_record,
						remote_record_field_name,
						nf_label, max_nf_label_length );
		}
	}

	return nf_label;
}

/* ====================================================================== */

#define MXU_GET_PUT_ARRAY_ASCII_LOCATION_LENGTH \
	MXU_HOSTNAME_LENGTH + MXU_RECORD_FIELD_NAME_LENGTH + 80

static mx_status_type
mx_get_array_ascii_error_message( uint32_t status_code,
			char *server_name,
			char *record_field_name,
			char *error_message )
{
	char location[ MXU_GET_PUT_ARRAY_ASCII_LOCATION_LENGTH + 1 ];

	snprintf( location, sizeof(location),
		"MX GET from server '%s', record field '%s'",
		server_name, record_field_name );

	return mx_error( (long) status_code, location, "%s", error_message );
}

static mx_status_type
mx_put_array_ascii_error_message( uint32_t status_code,
			char *server_name,
			char *record_field_name,
			char *error_message )
{
	char location[ MXU_GET_PUT_ARRAY_ASCII_LOCATION_LENGTH + 1 ];

	snprintf( location, sizeof(location),
		"MX PUT to server '%s', record field '%s'",
		server_name, record_field_name );

	return mx_error( (long) status_code, location, "%s", error_message );
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_get_field_array( MX_RECORD *server_record,
			char *remote_record_field_name,
			MX_NETWORK_FIELD *nf,
			MX_RECORD_FIELD *local_field,
			void *value_ptr )
{
	static const char fname[] = "mx_get_field_array()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	MX_LIST_HEAD *list_head;
	char nf_label[NF_LABEL_LENGTH];
	MX_RECORD_FIELD_PARSE_STATUS temp_parse_status;
	mx_status_type ( *token_parser )
		(void *, char *, MX_RECORD *, MX_RECORD_FIELD *,
		MX_RECORD_FIELD_PARSE_STATUS *);
	long datatype, num_dimensions, *dimension_array;
	size_t *data_element_size_array;
	mx_bool_type array_is_dynamically_allocated;
	mx_bool_type use_network_handles;

	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *buffer;
	char *message;
	uint32_t header_length, message_length;
	uint32_t send_message_type, receive_message_type;
	uint32_t status_code;
	mx_status_type mx_status;
	char token_buffer[500];

	static char separators[] = MX_RECORD_FIELD_SEPARATORS;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	server = NULL;
	datatype = -1;
	num_dimensions = -1;
	dimension_array = NULL;
	data_element_size_array = NULL;

	use_network_handles = TRUE;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		use_network_handles = FALSE;

		/* We are not using a network handle. */

		if ( (server_record == (MX_RECORD *) NULL)
		  || (remote_record_field_name == (char *) NULL) )
		{
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"If the MX_NETWORK_FIELD argument is NULL, "
			"the server_record and remote_record_field_name "
			"arguments must both not be NULL." );
		}
	} else {
		/* In this case, we ignore the values that were passed
		 * in the server_record and remote_record_field_name
		 * arguments and use the values from the nf structure
		 * instead, since we always prefer to use the values
		 * from MX_NETWORK_FIELD structures.
		 */

		server_record = nf->server_record;
		remote_record_field_name = nf->nfname;
	}

	mx_status = mx_network_field_get_parameters(
			server_record, local_field,
			&datatype, &num_dimensions,
			&dimension_array, &data_element_size_array,
			&server, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( server->server_supports_network_handles == FALSE ) {
		use_network_handles = FALSE;
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** GET ARRAY from '%s'",
			mx_network_get_nf_label(
				server_record,
				remote_record_field_name,
				nf_label, sizeof(nf_label) )
			));
	}

	if ( local_field->flags & MXFF_VARARGS ) {
		array_is_dynamically_allocated = TRUE;
	} else {
		array_is_dynamically_allocated = FALSE;
	}

	mx_status = mx_network_reconnect_if_down( server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*********** Send a 'get array' command. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length = mx_remote_header_length(server);

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	message = buffer + header_length;

	uint32_message = header + (header_length / sizeof(uint32_t));

	if ( use_network_handles == FALSE ) {

		/* Use ASCII record field name */

		send_message_type = MX_NETMSG_GET_ARRAY_BY_NAME;

		strlcpy( message, remote_record_field_name,
				MXU_RECORD_FIELD_NAME_LENGTH );

		message_length = (uint32_t) ( strlen( message ) + 1 );
	} else {

		/* Use binary network handle */

		send_message_type = MX_NETMSG_GET_ARRAY_BY_HANDLE;

		uint32_message[0] = mx_htonl( nf->record_handle );
		uint32_message[1] = mx_htonl( nf->field_handle );

		message_length = 2 * sizeof( uint32_t );
	}

	header[MX_NETWORK_MESSAGE_TYPE] = mx_htonl( send_message_type );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

	server->last_data_type = local_field->datatype;

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] =
				mx_htonl( local_field->datatype );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, aligned_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************* Wait for the response. **************/

	mx_status = mx_network_wait_for_message_id( server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	if ( use_network_handles == FALSE ) {
		MX_HRT_RESULTS( measurement, fname, remote_record_field_name );
	} else {
		MX_HRT_RESULTS( measurement, fname, "(%ld,%ld) %s",
				nf->record_handle, nf->field_handle,
				remote_record_field_name );
	}
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length        = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length       = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	receive_message_type = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code          = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

#if 0
	if ( nf != NULL ) {
		MX_DEBUG(-2,("%s: nf = %p, server = '%s', field = '%s'",
			fname, nf, nf->server_record->name, nf->nfname));
	} else {
		MX_DEBUG(-2,("%s: nf = %p, server = '%s', field = '%s'",
			fname, nf, server_record->name,
			remote_record_field_name));
	}

	MX_DEBUG(-2,("%s: header_length = %lu, message_length = %lu, "
		"message_type = %#lx, status_code = %lu", fname,
		(unsigned long) header_length,
		(unsigned long) message_length,
		(unsigned long) message_type,
		(unsigned long) status_code));
#endif
	/* Check to see if the response type matches the send type. */

	if ( ( server->remote_mx_version < 1005000L )
	  && ( send_message_type == MX_NETMSG_GET_ARRAY_BY_HANDLE )
	  && ( receive_message_type == 
	  		mx_server_response(MX_NETMSG_GET_ARRAY_BY_NAME) ) )
	{
		/* Bug compatibility for old MX servers. */

		/* MX servers prior to MX 1.5 returned the wrong server
		 * response type for 'get array by handle' messages.
		 */
	} else
	if ( receive_message_type != mx_server_response(send_message_type) ) {

		if ( receive_message_type == MX_NETMSG_UNEXPECTED_ERROR ) {
			switch( status_code ) {
			case MXE_SUCCESS:
				break;
			case MXE_BAD_HANDLE:
				MX_DEBUG( 2,
		("%s: MXE_BAD_HANDLE seen for server '%s', nfname = '%s'.",
				fname, nf->server_record->name, nf->nfname));

				nf->record_handle = MX_ILLEGAL_HANDLE;
				nf->field_handle = MX_ILLEGAL_HANDLE;

				/* Fall through to the default case. */
			default:
				return mx_get_array_ascii_error_message(
					status_code,
					server_record->name,
					remote_record_field_name,
					message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Message type for response was not %#lx.  "
		"Instead it was of type = %#lx",
			(unsigned long) mx_server_response( send_message_type ),
			(unsigned long) receive_message_type );
	}

	message = buffer + header_length;

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the array data
	 * we wanted.
	 */

	switch( status_code ) {
	case MXE_SUCCESS:
		break;
	case MXE_BAD_HANDLE:
		MX_DEBUG( 2,
		("%s: MXE_BAD_HANDLE seen for server '%s', nfname = '%s'.",
			fname, nf->server_record->name, nf->nfname));

		nf->record_handle = MX_ILLEGAL_HANDLE;
		nf->field_handle = MX_ILLEGAL_HANDLE;

		/* Fall through to the default case. */
	default:
		return mx_get_array_ascii_error_message(
				status_code,
				server_record->name,
				remote_record_field_name,
				message );
	}

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mx_network_get_remote_field_label( NULL, server_record,
						remote_record_field_name, NULL,
						nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX GET_ARRAY('%s') = ", nf_label );

		mx_network_buffer_show_value( message, server->data_format,
					datatype, receive_message_type,
					message_length,
					server->use_64bit_network_longs );
		fprintf( stderr, "\n" );
	}

	/************ Parse the data that was returned. ***************/

	switch( datatype ) {
	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
	case MXFT_RECORD_FIELD:
		datatype = MXFT_STRING;
	}

	switch( server->data_format ) {
	case MX_NETWORK_DATAFMT_ASCII:

		/* The data was returned using the ASCII MX database format. */

		mx_status = mx_get_token_parser( datatype, &token_parser );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_initialize_parse_status( &temp_parse_status,
						message, separators );

		/* If this is a string field, figure out what the maximum
		 * string token length is.
		 */

		if ( datatype == MXFT_STRING ) {
			temp_parse_status.max_string_token_length =
				mx_get_max_string_token_length( local_field );
		} else {
			temp_parse_status.max_string_token_length = 0L;
		}

		/* Now we are ready to parse the tokens. */

		if ( (num_dimensions == 0) ||
			((datatype == MXFT_STRING) && (num_dimensions == 1)) )
		{
			mx_status = mx_get_next_record_token(
				&temp_parse_status,
				token_buffer, sizeof(token_buffer) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = ( *token_parser ) ( value_ptr,
					token_buffer, NULL, local_field,
					&temp_parse_status );
		} else {
			mx_status = mx_parse_array_description( value_ptr,
				num_dimensions - 1, NULL, local_field,
				&temp_parse_status, token_parser );
		}
		break;

	case MX_NETWORK_DATAFMT_RAW:
		mx_status = mx_copy_network_buffer_to_array(
				message, message_length,
				value_ptr, array_is_dynamically_allocated,
				datatype, num_dimensions,
				dimension_array, data_element_size_array,
				NULL,
				server->use_64bit_network_longs );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
		case MXE_WOULD_EXCEED_LIMIT:
			break;
		default:
			/* Only display an error message here if the returned
			 * error code was not MXE_WOULD_EXCEED_LIMIT, since
			 * MXE_WOULD_EXCEED_LIMIT is used to request that the
			 * network buffer be increased in size.
			 */

			(void) mx_error( mx_status.code, fname,
	"Buffer copy for MX_GET of parameter '%s' in server '%s' failed.",
				remote_record_field_name, server_record->name );
		}
		break;

	case MX_NETWORK_DATAFMT_XDR:
#if HAVE_XDR
		/* For the "special" field types MXFT_RECORD, MXFT_RECORDTYPE,
		 * MXFT_INTERFACE, and MXFT_RECORD_FIELD, the server does not
		 * actually use XDR format.  Instead, it just copies a string
		 * into the message buffer for all data formats, so we must
		 * handle those cases specially.
		 */

		switch( local_field->datatype ) {
		case MXFT_RECORD:
		case MXFT_RECORDTYPE:
		case MXFT_INTERFACE:
		case MXFT_RECORD_FIELD:
			if ( num_dimensions != 1 ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The receiving array for the MXFT_RECORD, "
				"MXFT_RECORDTYPE, MXFT_INTERFACE field, "
				"or MXFT_RECORD_FIELD "
				"'%s' from server '%s' _must_ be a "
				"1-dimensional string.  Instead, it is "
				"a %lu-dimensional array of datatype %lu.",
					remote_record_field_name,
					server_record->name,
					num_dimensions, local_field->datatype);
			}

			/* We just copy the string here. */

			strlcpy( value_ptr, message, dimension_array[0] );

			return MX_SUCCESSFUL_RESULT;
		}

		/* If we get here, then we _should_ have an XDR formatted
		 * buffer to convert.
		 */

		mx_status = mx_xdr_data_transfer( MX_XDR_DECODE,
				value_ptr, array_is_dynamically_allocated,
				datatype, num_dimensions,
				dimension_array, data_element_size_array,
				message, message_length, NULL );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
		case MXE_WOULD_EXCEED_LIMIT:
			break;
		default:
			/* Only display an error message here if the returned
			 * error code was not MXE_WOULD_EXCEED_LIMIT, since
			 * MXE_WOULD_EXCEED_LIMIT is used to request that the
			 * network buffer be increased in size.
			 */

			(void) mx_error( mx_status.code, fname,
	"Buffer copy for MX_GET of parameter '%s' in server '%s' failed.",
				remote_record_field_name, server_record->name );
		}
#else
		return mx_error( MXE_UNSUPPORTED, fname,
			"XDR network data format is not supported "
			"on this system." );
#endif
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized network data format type %lu was requested.",
			server->data_format );
	}

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_put_field_array( MX_RECORD *server_record,
			char *remote_record_field_name,
			MX_NETWORK_FIELD *nf,
			MX_RECORD_FIELD *local_field,
			void *value_ptr )
{
	static const char fname[] = "mx_put_field_array()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_SERVER_FUNCTION_LIST *function_list;
	MX_LIST_HEAD *list_head;
	char nf_label[80];
	mx_status_type ( *token_constructor )
		(void *, char *, size_t, MX_RECORD *, MX_RECORD_FIELD *);
	long datatype, num_dimensions, *dimension_array;
	size_t *data_element_size_array;
	mx_bool_type array_is_dynamically_allocated;
	mx_bool_type use_network_handles;

	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *message, *ptr, *name_ptr;
	unsigned long i, j, max_attempts;

#if defined(_WIN64)
	uint64_t xdr_ptr_address, xdr_remainder_value, xdr_gap_size;
#else
	unsigned long xdr_ptr_address, xdr_remainder_value, xdr_gap_size;
#endif
	uint32_t header_length, field_id_length;
	uint32_t message_length, saved_message_length, max_message_length;
	uint32_t send_message_type, receive_message_type;
	uint32_t status_code;
	size_t buffer_left, num_network_bytes;
	size_t current_length, new_length;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	server = NULL;
	datatype = -1;
	num_dimensions = -1;
	dimension_array = NULL;
	data_element_size_array = NULL;

	use_network_handles = TRUE;

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		use_network_handles = FALSE;

		/* We are not using a network handle. */

		if ( (server_record == (MX_RECORD *) NULL)
		  || (remote_record_field_name == (char *) NULL) )
		{
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"If the MX_NETWORK_FIELD argument is NULL, "
			"the server_record and remote_record_field_name "
			"arguments must both not be NULL." );
		}
	} else {
		/* In this case, we ignore the values that were passed
		 * in the server_record and remote_record_field_name
		 * arguments and use the values from the nf structure
		 * instead, since we always prefer to use the values
		 * from MX_NETWORK_FIELD structures.
		 */

		server_record = nf->server_record;
		remote_record_field_name = nf->nfname;
	}

	mx_status = mx_network_field_get_parameters(
			server_record, local_field,
			&datatype, &num_dimensions,
			&dimension_array, &data_element_size_array,
			&server, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( server->server_supports_network_handles == FALSE ) {
		use_network_handles = FALSE;
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** PUT ARRAY to '%s'",
			mx_network_get_nf_label(
				server_record,
				remote_record_field_name,
				nf_label, sizeof(nf_label) )
			));
	}

	if ( local_field->flags & MXFF_VARARGS ) {
		array_is_dynamically_allocated = TRUE;
	} else {
		array_is_dynamically_allocated = FALSE;
	}

	mx_status = mx_network_reconnect_if_down( server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************ Construct a 'put array' command. *************/

	aligned_buffer = server->message_buffer;

	header  = aligned_buffer->u.uint32_buffer;

	header_length = mx_remote_header_length(server);

	message = aligned_buffer->u.char_buffer + header_length;

	max_message_length = aligned_buffer->buffer_length - header_length;

	uint32_message = header + (header_length / sizeof(uint32_t));

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] =
				mx_htonl( local_field->datatype );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

	if ( use_network_handles == FALSE ) {

		/* Use ASCII record field name */

		send_message_type = MX_NETMSG_PUT_ARRAY_BY_NAME;

		snprintf( message, max_message_length,
				"%*s", -MXU_RECORD_FIELD_NAME_LENGTH,
				remote_record_field_name );

		field_id_length = MXU_RECORD_FIELD_NAME_LENGTH;
	} else {

		/* Use binary network handle */

		send_message_type = MX_NETMSG_PUT_ARRAY_BY_HANDLE;

		uint32_message[0] = mx_htonl( nf->record_handle );
		uint32_message[1] = mx_htonl( nf->field_handle );

		field_id_length = 2 * sizeof( uint32_t );
	}

	header[MX_NETWORK_MESSAGE_TYPE] = mx_htonl( send_message_type );

	ptr = message + field_id_length;

	MX_DEBUG( 2,("%s: message = %p, ptr = %p, field_id_length = %lu",
		fname, message, ptr, (unsigned long) field_id_length));

	buffer_left = aligned_buffer->buffer_length
			- header_length - field_id_length;

	/* Construct the data to send.   We use a retry loop here in case
	 * we need to increase the size of the network buffer to make the
	 * message fit.
	 */

	max_attempts = 10;

	message_length = field_id_length;

	for ( i = 0; i < max_attempts; i++ ) {

	    saved_message_length = message_length;

	    switch( server->data_format ) {
	    case MX_NETWORK_DATAFMT_ASCII:

		/* Send the data using the ASCII MX database format. */

		mx_status = mx_get_token_constructor( datatype,
						&token_constructor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (num_dimensions == 0)
		  || ((datatype == MXFT_STRING) && (num_dimensions == 1)) )
		{
			mx_status = (*token_constructor) ( value_ptr, ptr,
			    aligned_buffer->buffer_length -
			    	( ptr - aligned_buffer->u.char_buffer ),
			    NULL, local_field );
		} else {
			mx_status = mx_create_array_description( value_ptr, 
			    local_field->num_dimensions - 1, ptr,
			    aligned_buffer->buffer_length -
			    	( ptr - aligned_buffer->u.char_buffer ),
			    NULL, local_field, token_constructor );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* The extra 1 is for the '\0' byte at the end of the string. */

		message_length += 1 + strlen( ptr );

		/* ASCII data transfers do not currently support dynamically
		 * resizing network buffers, so we return instead if we get
		 * an MXE_WOULD_EXCEED_LIMIT status code.
		 */

		num_network_bytes = 0;

		if ( mx_status.code == MXE_WOULD_EXCEED_LIMIT )
			return mx_status;
		break;

	    case MX_NETWORK_DATAFMT_RAW:
		mx_status = mx_copy_array_to_network_buffer( value_ptr,
				array_is_dynamically_allocated,
				datatype, num_dimensions,
				dimension_array, data_element_size_array,
				ptr, buffer_left,
				&num_network_bytes,
				server->use_64bit_network_longs );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
		case MXE_WOULD_EXCEED_LIMIT:
			break;
		default:
			/* Only display an error message here if the returned
			 * error code was not MXE_WOULD_EXCEED_LIMIT, since
			 * MXE_WOULD_EXCEED_LIMIT is used to request that the
			 * network buffer be increased in size.
			 */

			(void) mx_error( mx_status.code, fname,
	"Buffer copy for MX_PUT of parameter '%s' in server '%s' failed.",
				remote_record_field_name, server_record->name );
		}

		message_length += num_network_bytes;
		break;

	    case MX_NETWORK_DATAFMT_XDR:
#if HAVE_XDR
		/* The XDR data pointer 'ptr' must be aligned on a 4 byte
		 * address boundary for XDR data conversion to work correctly
		 * on all architectures.  If the pointer does not point to
		 * an address that is a multiple of 4 bytes, we move it to
		 * the next address that _is_ and fill the bytes inbetween
		 * with zeros.
		 */

#if defined(_WIN64)
		xdr_ptr_address = (uint64_t) ptr;
#else
		xdr_ptr_address = (unsigned long) ptr;
#endif

		xdr_remainder_value = xdr_ptr_address % 4;

		if ( xdr_remainder_value != 0 ) {
			xdr_gap_size = 4 - xdr_remainder_value;

			for ( j = 0; j < xdr_gap_size; j++ ) {
				ptr[j] = '\0';
			}

			ptr += xdr_gap_size;

			message_length += xdr_gap_size;
		}

		/* Now we are ready to do the XDR data conversion. */

		mx_status = mx_xdr_data_transfer( MX_XDR_ENCODE,
				value_ptr,
				array_is_dynamically_allocated,
				datatype, num_dimensions,
				dimension_array, data_element_size_array,
				ptr, buffer_left,
				&num_network_bytes );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
		case MXE_WOULD_EXCEED_LIMIT:
			break;
		default:
			/* Only display an error message here if the returned
			 * error code was not MXE_WOULD_EXCEED_LIMIT, since
			 * MXE_WOULD_EXCEED_LIMIT is used to request that the
			 * network buffer be increased in size.
			 */

			(void) mx_error( mx_status.code, fname,
	"Buffer copy for MX_PUT of parameter '%s' in server '%s' failed.",
				remote_record_field_name, server_record->name );
		}

		message_length += num_network_bytes;
#else
		return mx_error( MXE_UNSUPPORTED, fname,
			"XDR network data format is not supported "
			"on this system." );
#endif
		break;

	    default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized network data format type %lu was requested.",
			server->data_format );
	    }

	    /* If we succeeded or if some error other than 
	     * MXE_WOULD_EXCEED_LIMIT occurred, break out
	     * of the for(i) loop.
	     */

	    if ( mx_status.code != MXE_WOULD_EXCEED_LIMIT ) {
		break;
	    }

	    /* The data does not fit into our existing buffer, so we must
	     * try to make the buffer larger.
	     *
	     * NOTE: In this situation, the variable 'num_network_bytes'
	     * _actually_ tells you how many bytes would not fit in the
	     * existing buffer.
	     */

	    current_length = aligned_buffer->buffer_length;

	    new_length = current_length + num_network_bytes;

	    mx_status = mx_reallocate_network_buffer(
			    		aligned_buffer, new_length );

	    if ( mx_status.code != MXE_SUCCESS )
		    return mx_status;

	    /* Update some values that reallocation will have changed. */

	    header  = aligned_buffer->u.uint32_buffer;
	    message = aligned_buffer->u.char_buffer + header_length;

	    /* Revert back to the buffer location before the failed copy. */

	    message_length = saved_message_length;

	    ptr         = message + message_length;
	    buffer_left = aligned_buffer->buffer_length
		    		- header_length - message_length;
	}

	if ( i >= max_attempts ) {
		if ( nf == NULL ) {
			name_ptr = remote_record_field_name;
		} else {
			name_ptr = nf->nfname;
		}
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"%lu attempts to increase the network buffer size for "
		"record field '%s' failed.  "
		"You should never see this error.",
			max_attempts, name_ptr );
	}

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

	MX_DEBUG( 2,("%s: message = '%s'", fname, message));

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		unsigned long handle_length;

		mx_network_get_remote_field_label( NULL, server_record,
						remote_record_field_name, NULL,
						nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX PUT_ARRAY('%s', ", nf_label );

		handle_length = 2 * sizeof(uint32_t);

		mx_network_buffer_show_value( message + handle_length,
					server->data_format,
					datatype, send_message_type,
					message_length - handle_length,
					server->use_64bit_network_longs );
		fprintf( stderr, ")\n" );
	}

	message_length += header_length;

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	/*************** Send the message. **************/

	mx_status = mx_network_send_message( server_record, aligned_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. ************/

	mx_status = mx_network_wait_for_message_id( server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	if ( use_network_handles == FALSE ) {
		MX_HRT_RESULTS( measurement, fname, remote_record_field_name );
	} else {
		MX_HRT_RESULTS( measurement, fname, "(%ld,%ld) %s",
				nf->record_handle, nf->field_handle,
				remote_record_field_name );
	}
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = aligned_buffer->u.uint32_buffer;

        header_length        = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
        message_length       = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
        receive_message_type = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
        status_code          = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

#if 0
	if ( nf != NULL ) {
		MX_DEBUG(-2,("%s: nf = %p, server = '%s', field = '%s'",
			fname, nf, nf->server_record->name, nf->nfname));
	} else {
		MX_DEBUG(-2,("%s: nf = %p, server = '%s', field = '%s'",
			fname, nf, server_record->name,
			remote_record_field_name));
	}

	MX_DEBUG(-2,("%s: header_length = %lu, message_length = %lu, "
		"message_type = %#lx, status_code = %lu", fname,
		(unsigned long) header_length,
		(unsigned long) message_length,
		(unsigned long) message_type,
		(unsigned long) status_code));
#endif

	if ( ( server->remote_mx_version < 1005000L )
	  && ( send_message_type == MX_NETMSG_PUT_ARRAY_BY_HANDLE )
	  && ( receive_message_type == 
	  		mx_server_response(MX_NETMSG_PUT_ARRAY_BY_NAME) ) )
	{
		/* Bug compatibility for old MX servers. */

		/* MX servers prior to MX 1.5 returned the wrong server
		 * response type for 'put array by handle' messages.
		 */
	} else
        if ( receive_message_type != mx_server_response(send_message_type) ) {

		if ( receive_message_type == MX_NETMSG_UNEXPECTED_ERROR ) {
			return mx_put_array_ascii_error_message(
					status_code,
					server_record->name,
					remote_record_field_name,
					message );
		}

                return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Message type for response was not %#lx.  "
		"Instead it was of type = %#lx",
			(unsigned long) mx_server_response(send_message_type),
			(unsigned long) receive_message_type );
        }

        message = aligned_buffer->u.char_buffer + header_length;

        /* If the remote command failed, the message field will include
         * the text of the error message rather than the array data
         * we wanted.
         */

        if ( status_code != MXE_SUCCESS ) {
		return mx_put_array_ascii_error_message(
				status_code,
				server_record->name,
				remote_record_field_name,
				message );
        } else {
		return MX_SUCCESSFUL_RESULT;
	}
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_field_connect( MX_NETWORK_FIELD *nf )
{
	static const char fname[] = "mx_network_field_connect()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	MX_LIST_HEAD *list_head;
	char nf_label[80];
	uint32_t *header;
	char *buffer, *message;
	uint32_t *message_uint32_array;
	uint32_t header_length, message_length;
	uint32_t message_type, status_code;
	uint32_t header_length_in_32bit_words;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s invoked, nf->nfname = '%s'", fname, nf->nfname));

	if ( nf->server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The server_record pointer for network field (%ld,%ld) '%s' is NULL.",
			nf->record_handle, nf->field_handle, nf->nfname );
	}

	server = (MX_NETWORK_SERVER *) nf->server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			nf->server_record->name );
	}

	list_head = mx_get_record_list_head_struct( nf->server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** GET NETWORK HANDLE for '%s'",
			mx_network_get_nf_label(
				nf->server_record, nf->nfname,
				nf_label, sizeof(nf_label) )
			));
	}

	mx_status = mx_network_reconnect_if_down( nf->server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************ Send the 'get network handle' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length = mx_remote_header_length(server);

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_MESSAGE_TYPE]
				= mx_htonl( MX_NETMSG_GET_NETWORK_HANDLE );
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	message = buffer + header_length;

	/* Copy the network field name to the outgoing message buffer. */

	strlcpy( message, nf->nfname, MXU_RECORD_FIELD_NAME_LENGTH );

	message_length = (uint32_t) ( strlen( message ) + 1 );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] = mx_htonl( MXFT_STRING );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( nf->server_record, aligned_buffer);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_wait_for_message_id( nf->server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, nf->nfname );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type
		!= mx_server_response( MX_NETMSG_GET_NETWORK_HANDLE ) )
	{
		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {

			if ( status_code == MXE_NOT_YET_IMPLEMENTED ) {

				/* If we received an MXE_NOT_YET_IMPLEMENTED
				 * status code, this means that the remote
				 * MX server is not a recent enough version
				 * to implement network handles.  We must 
				 * record this fact so that later GET and
				 * PUT operations will not attempt to use
				 * network handle support.
				 */

				MX_DEBUG( 2,
	("%s: network handle support not implemented for MX server '%s'.",
					fname, server->record->name ));

				server->server_supports_network_handles = FALSE;

				return MX_SUCCESSFUL_RESULT;
			} else {
				/* If we got some other status code, just
				 * return the error message.
				 */

				return mx_error( (long)status_code, fname,
					"%s", message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_GET_NETWORK_HANDLE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, "%s", message );
	}

	if ( message_length < (2 * sizeof(uint32_t)) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Incomplete message received.  Message length = %ld",
			(long) message_length );
	}

	/***** Get the network handle out of what we were sent. *****/

	header_length_in_32bit_words = (uint32_t)
				( header_length / sizeof(uint32_t) );

	message_uint32_array = header + header_length_in_32bit_words;

	nf->record_handle = 
		(long) mx_ntohl( (unsigned long) message_uint32_array[0] );

	nf->field_handle  = 
		(long) mx_ntohl( (unsigned long) message_uint32_array[1] );

#if NETWORK_DEBUG_TIMING
	MX_DEBUG(-2,
	("%s: server '%s', field '%s' = (%ld,%ld)",
		fname, nf->server_record->name, nf->nfname,
		nf->record_handle, nf->field_handle ));
#endif

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {

		mx_network_get_nf_label( nf->server_record, nf->nfname,
				nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX GET_NETWORK_HANDLE('%s') = (%lu,%lu)\n",
			nf_label, nf->record_handle, nf->field_handle );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_get_field_type( MX_RECORD *server_record, 
			char *remote_record_field_name,
			long max_dimensions,
			long *datatype,
			long *num_dimensions,
			long *dimension_array )
{
	static const char fname[] = "mx_get_field_type()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	MX_LIST_HEAD *list_head;
	char nf_label[80];
	uint32_t *header;
	char *buffer, *message;
	uint32_t *message_uint32_array;
	uint32_t header_length, message_length;
	uint32_t message_type, status_code;
	uint32_t i, expected_message_length;
	uint32_t header_length_in_32bit_words;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked, remote_record_field_name = '%s'",
			fname, remote_record_field_name));

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** GET FIELD TYPE for '%s'",
			mx_network_get_nf_label(
				server_record,
				remote_record_field_name,
				nf_label, sizeof(nf_label) )
			));
	}

	mx_status = mx_network_reconnect_if_down( server_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************ Send the 'get field type' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length = mx_remote_header_length(server);

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_MESSAGE_TYPE]  = mx_htonl( MX_NETMSG_GET_FIELD_TYPE );
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	message = buffer + header_length;

	strlcpy( message, remote_record_field_name,
					MXU_RECORD_FIELD_NAME_LENGTH );

	message_length = (uint32_t) ( strlen( message ) + 1 );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] = mx_htonl( MXFT_STRING );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, aligned_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_wait_for_message_id( server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, remote_record_field_name );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_GET_FIELD_TYPE ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {
			return mx_error( (long)status_code, fname,
				"%s", message );
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_GET_FIELD_TYPE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, "%s", message );
	}

	if ( message_length < (2 * sizeof(uint32_t)) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Incomplete message received.  Message length = %ld",
			(long) message_length );
	}

	/***** Get the type information out of what we were sent. *****/

	header_length_in_32bit_words = (uint32_t)
				( header_length / sizeof(uint32_t) );

	message_uint32_array = header + header_length_in_32bit_words;

	*datatype = (long) mx_ntohl( (unsigned long) message_uint32_array[0] );
	*num_dimensions
		= (long) mx_ntohl( (unsigned long) message_uint32_array[1] );

	if ( (*num_dimensions) <= 0 ) {
		expected_message_length = (uint32_t) ( 3 * sizeof(uint32_t) );
	} else {
		expected_message_length = (uint32_t)
				( sizeof(uint32_t) * ((*num_dimensions) + 2) );
	}

	if ( message_length < expected_message_length ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Incomplete dimension array received.  %ld dimension values were received, "
"but %ld dimension values were expected.",
			(long) (message_length - 2), *num_dimensions );
	}
	if ( *num_dimensions < max_dimensions ) {
		max_dimensions = *num_dimensions;
	}
	if ( max_dimensions <= 0 ) {
		dimension_array[0] = (long)
			mx_ntohl( (unsigned long) message_uint32_array[2] );
	}
	for ( i = 0; i < max_dimensions; i++ ) {
		dimension_array[i] = (long)
			mx_ntohl( (unsigned long) message_uint32_array[i+2] );
	}

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mx_network_get_nf_label( server_record,
				remote_record_field_name,
				nf_label, sizeof(nf_label) );

		fprintf( stderr,
	"MX GET_FIELD_TYPE('%s') = ( 'datatype' = %ld, 'num_dimensions' = %ld",
			nf_label, *datatype, *num_dimensions );

		if ( *num_dimensions > 0 ) {
			fprintf( stderr, ", 'dimension' = <%ld",
				dimension_array[0] );

			for ( i = 1; i < *num_dimensions; i++ ) {
				fprintf( stderr, ", %ld",
					dimension_array[i] );
			}

			fprintf( stderr, ">" );
		}

		fprintf( stderr, " )\n" );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

/* mx_set_client_info() sends information to the server about the client
 * for debugging and status reporting purposes _only_.  No authentication
 * is done, so this mechanism cannot and _must_ _not_ be used for access
 * control.  Someday we may have SSL/Kerberos/whatever-based authentication,
 * but this function does not do any of that kind of thing just now.
 */

MX_EXPORT mx_status_type
mx_set_client_info( MX_RECORD *server_record,
			const char *username,
			const char *program_name )
{
	static const char fname[] = "mx_set_client_info()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	MX_LIST_HEAD *list_head;
	uint32_t *header;
	char *buffer, *message, *ptr;
	uint32_t header_length, message_length, max_message_length;
	uint32_t message_type, status_code;
	mx_bool_type connection_is_up;
	long i, string_length;
	char *local_username, *local_program_name;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	/*--------------------------------------------------------------*/

	/* Change "illegal" characters in user names and program names
	 * to underscore or null characters.  However, we must not change
	 * these strings in our caller, so we make local copies of them.
	 */

	local_username = strdup( username );

	if ( local_username == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating a copy of the username." );
	}

	string_length = strlen( local_username );

	for ( i = 0; i < string_length; i++ ) {
		if ( local_username[i] == ' ' ) {
			local_username[i] = '_';
		} else
		if ( local_username[i] == '\t' ) {
			local_username[i] = '_';
		} else
		if ( local_username[i] == '\n' ) {
			local_username[i] = '\0';
		}
	}

	local_program_name = strdup( program_name );

	if ( local_program_name == (char *) NULL ) {
		mx_free( local_username );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating a copy of the program name." );
	}

	string_length = strlen( local_program_name );

	for ( i = 0; i < string_length; i++ ) {
		if ( local_program_name[i] == ' ' ) {
			local_program_name[i] = '_';
		} else
		if ( local_program_name[i] == '\t' ) {
			local_program_name[i] = '_';
		} else
		if ( local_program_name[i] == '\n' ) {
			local_program_name[i] = '\0';
		}
	}

	/*--------------------------------------------------------------*/

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,
    ("\n*** SET CLIENT INFO for server '%s', username '%s', program name '%s'.",
			server_record->name,
			local_username,
			local_program_name));
	}

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		char nf_label[ NF_LABEL_LENGTH ];

		mx_network_get_nf_label( server_record, NULL,
					nf_label, sizeof(nf_label) );

		fprintf( stderr,
		"MX SET_CLIENT_INFO('%s', user = '%s', program = '%s')\n",
			nf_label, local_username, local_program_name );
	}

	/* If the network connection is not currently up for some reason,
	 * do not try to send the client information.
	 */

	mx_status = mx_network_connection_is_up( server_record,
						&connection_is_up );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( local_username );
		mx_free( local_program_name );
		return mx_status;
	}

	MX_DEBUG( 2,("%s: server '%s', connection_is_up = %d",
		fname, server_record->name, (int) connection_is_up));

	if ( connection_is_up == FALSE ) {
		mx_free( local_username );
		mx_free( local_program_name );
		return MX_SUCCESSFUL_RESULT;
	}

	/************ Send the 'set client info' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length = mx_remote_header_length(server);

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_MESSAGE_TYPE]  = mx_htonl( MX_NETMSG_SET_CLIENT_INFO);
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	message = buffer + header_length;

	max_message_length = aligned_buffer->buffer_length - header_length;

	strlcpy( message, local_username, max_message_length );

	strlcat( message, " ", max_message_length );

	strlcat( message, local_program_name, max_message_length );

	strlcat( message, " ", max_message_length );

	message_length = (uint32_t) strlen( message );

	ptr = message + message_length;

	snprintf( ptr, max_message_length - message_length,
			"%lu", mx_process_id() );

	MX_DEBUG( 2,("%s: message = '%s'", fname, message));

	message_length = (uint32_t) ( strlen( message ) + 1 );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] = mx_htonl( MXFT_STRING );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

	mx_free( local_username );
	mx_free( local_program_name );

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, aligned_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_wait_for_message_id( server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_SET_CLIENT_INFO ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {
			return mx_error( (long)status_code, fname,
				"%s", message );
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_SET_CLIENT_INFO.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, "%s", message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_get_option( MX_RECORD *server_record,
			unsigned long option_number,
			unsigned long *option_value )
{
	static const char fname[] = "mx_network_get_option()";

	MX_NETWORK_SERVER *server;
	MX_LIST_HEAD *list_head;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *buffer, *message;
	uint32_t header_length, message_length, message_type;
	long status_code;
	uint32_t header_length_in_32bit_words;
	mx_bool_type quiet;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked, option_number = '%#lx'",
			fname, option_number));

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}
	if ( option_value == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"option_value pointer passed was NULL." );
	}

	if ( option_number & MXE_QUIET ) {
		quiet = TRUE;

		option_number &= (~MXE_QUIET);
	} else {
		quiet = FALSE;
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** GET OPTION %lu for server '%s'.",
			option_number, server_record->name ));
	}

	/************ Send the 'get option' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length = mx_remote_header_length(server);

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_MESSAGE_TYPE]  = mx_htonl( MX_NETMSG_GET_OPTION );
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	message = buffer + header_length;
	uint32_message = header + (header_length / sizeof(uint32_t));

	uint32_message[0] = mx_htonl( option_number );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( sizeof(uint32_t) );

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] = mx_htonl( MXFT_ULONG );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, aligned_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_wait_for_message_id( server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = (long) mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_GET_OPTION ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {

			if ( status_code == MXE_NOT_YET_IMPLEMENTED ) {

				/* The remote server is an old server that
				 * does not implement the 'get option'
				 * message.
				 */

				return mx_error(
				( status_code | MXE_QUIET ), fname,
					"The MX 'get option' message type is "
					"not implemented by server '%s'.",
						server_record->name );
			} else {
				/* Some other error occurred. */

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Unexpected server error for MX server '%s'.  "
	"Server error was '%s -> %s'", server_record->name,
				mx_status_code_string((long) status_code ),
					message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_GET_OPTION.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		if ( quiet ) {
			status_code |= MXE_QUIET;
		}

		return mx_error( (long)status_code, fname, "%s", message );
	}

	if ( message_length < sizeof(uint32_t) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Incomplete message received.  Message length = %ld",
			(long) message_length );
	}

	header_length_in_32bit_words = (uint32_t)
				( header_length / sizeof(uint32_t) );

	uint32_message = header + header_length_in_32bit_words;

	*option_value = mx_ntohl( uint32_message[0] );

	MX_DEBUG( 2,("%s invoked, *option_value = '%#lx'",
			fname, *option_value));

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		char nf_label[ NF_LABEL_LENGTH ];

		mx_network_get_nf_label( server_record, NULL,
					nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX GET_OPTION('%s', %lu ) = %lu\n",
			nf_label, option_number, *option_value );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_set_option( MX_RECORD *server_record,
			unsigned long option_number,
			unsigned long option_value )
{
	static const char fname[] = "mx_network_set_option()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	MX_LIST_HEAD *list_head;
	uint32_t *header, *uint32_message;
	char *buffer, *message;
	uint32_t header_length, message_length, message_type;
	long status_code;
	mx_bool_type quiet;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,
		("%s invoked, option_number = '%#lx', option_value = '%#lx'",
			fname, option_number, option_value));

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	if ( option_number & MXE_QUIET ) {
		quiet = TRUE;

		option_number &= (~MXE_QUIET);
	} else {
		quiet = FALSE;
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** SET OPTION %lu for server '%s'.",
			option_number, server_record->name ));
	}

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		char nf_label[ NF_LABEL_LENGTH ];

		mx_network_get_nf_label( server_record, NULL,
					nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX SET_OPTION('%s') Set option %lu to %lu\n",
			nf_label, option_number, option_value );
	}

	/************ Send the 'set option' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length = mx_remote_header_length(server);

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_MESSAGE_TYPE]  = mx_htonl( MX_NETMSG_SET_OPTION );
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	message = buffer + header_length;
	uint32_message = header + (header_length / sizeof(uint32_t));

	uint32_message[0] = mx_htonl( option_number );
	uint32_message[1] = mx_htonl( option_value );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( 2 * sizeof(uint32_t) );

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] = mx_htonl( MXFT_ULONG );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( server_record, aligned_buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_wait_for_message_id( server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = (long) mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	MXW_UNUSED( message_length );

	if ( message_type != mx_server_response( MX_NETMSG_SET_OPTION ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {

			if ( status_code == MXE_NOT_YET_IMPLEMENTED ) {

				/* The remote server is an old server that
				 * does not implement the 'set option'
				 * message.
				 */

				return mx_error(
				( status_code | MXE_QUIET ), fname,
					"The MX 'set option' message type is "
					"not implemented by server '%s'.",
						server_record->name );
			} else {
				/* Some other error occurred. */

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Unexpected server error for MX server '%s'.  "
	"Server error was '%s -> %s'", server_record->name,
				mx_status_code_string((long) status_code ),
					message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_SET_OPTION.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {

		if ( quiet ) {
			status_code |= MXE_QUIET;
		}

		return mx_error( (long)status_code, fname, "%s", message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_field_get_attribute( MX_NETWORK_FIELD *nf,
				unsigned long attribute_number,
				double *attribute_value )
{
	static const char fname[] = "mx_network_field_get_attribute()";

	MX_NETWORK_SERVER *server;
	MX_LIST_HEAD *list_head;
	char nf_label[80];
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *buffer, *message;

	union {
		double double_value;
		uint32_t uint32_value[2];
	} u;

	uint32_t header_length, message_length, message_type;
	long status_code;
	XDR xdrs;
	int xdr_status;
	mx_bool_type connected;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked, attribute_number = '%#lx'",
			fname, attribute_number));

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}
	if ( attribute_value == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"attribute_value pointer passed was NULL." );
	}

	mx_status = mx_network_field_is_connected( nf, &connected );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( connected == FALSE ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	server = (MX_NETWORK_SERVER *) nf->server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			nf->server_record->name );
	}

	list_head = mx_get_record_list_head_struct( nf->server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** GET ATTRIBUTE from '%s'",
			mx_network_get_nf_label(
				nf->server_record, nf->nfname,
				nf_label, sizeof(nf_label) )
			));
	}

	/************ Send the 'get attribute' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length = mx_remote_header_length(server);

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_MESSAGE_TYPE]  = mx_htonl( MX_NETMSG_GET_ATTRIBUTE );
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	message = buffer + header_length;
	uint32_message = header + (header_length / sizeof(uint32_t));

	uint32_message[0] = mx_htonl( nf->record_handle );
	uint32_message[1] = mx_htonl( nf->field_handle );
	uint32_message[2] = mx_htonl( attribute_number );

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( 3 * sizeof(uint32_t) );

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] = mx_htonl( MXFT_DOUBLE );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message(nf->server_record, aligned_buffer);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_wait_for_message_id( nf->server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = (long) mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_GET_ATTRIBUTE ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {

			if ( status_code == MXE_NOT_YET_IMPLEMENTED ) {

				/* The remote server is an old server that
				 * does not implement the 'get attribute'
				 * message.
				 */

				return mx_error(
				( status_code | MXE_QUIET ), fname,
				"The MX 'get attribute' message type is "
				"not implemented by server '%s'.",
						nf->server_record->name );
			} else {
				/* Some other error occurred. */

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Unexpected server error for MX server '%s'.  "
	"Server error was '%s -> %s'", nf->server_record->name,
				mx_status_code_string((long) status_code ),
					message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_GET_ATTRIBUTE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, "%s", message );
	}

	if ( message_length < sizeof(double) ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Incomplete message received.  Message length = %ld",
			(long) message_length );
	}

	message = buffer + header_length;

	uint32_message = header + (header_length / sizeof(uint32_t));

	switch( server->data_format ) {
	case MX_NETWORK_DATAFMT_ASCII:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Message type MX_NETMSG_GET_ATTRIBUTE is not supported "
			"for ASCII format network connections." );
		break;

	case MX_NETWORK_DATAFMT_RAW:
		/* Do _not_ use mx_ntohl() for the attribute value, since
		 * the raw value is actually an IEEE 754 double.
		 */

		u.uint32_value[0] = uint32_message[0];
		u.uint32_value[1] = uint32_message[1];

		*attribute_value = u.double_value;
		break;

#if HAVE_XDR
	case MX_NETWORK_DATAFMT_XDR:
		xdrmem_create( &xdrs, message, message_length, XDR_DECODE);

		xdr_status = xdr_double( &xdrs, attribute_value );

		MXW_UNUSED( xdr_status );

		xdr_destroy( &xdrs );
		break;
#endif /* HAVE_XDR */

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported data format %ld requested for MX server '%s'.",
			server->data_format, nf->server_record->name );
		break;
	}

	MX_DEBUG( 2,("%s invoked, *attribute_value = '%g'",
			fname, *attribute_value));

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mx_network_get_nf_label( nf->server_record, nf->nfname,
					nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX GET_ATTRIBUTE('%s', %lu ) = %g\n",
			nf_label, attribute_number, *attribute_value );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_field_set_attribute( MX_NETWORK_FIELD *nf,
				unsigned long attribute_number,
				double attribute_value )
{
	static const char fname[] = "mx_network_field_set_attribute()";

	MX_NETWORK_SERVER *server;
	MX_LIST_HEAD *list_head;
	char nf_label[80];
	MX_NETWORK_MESSAGE_BUFFER *aligned_buffer;
	uint32_t *header, *uint32_message;
	char *buffer, *message;

	union {
		double double_value;
		uint32_t uint32_value[2];
	} u;

	uint32_t header_length, message_length, message_type;
	long status_code;
	XDR xdrs;
	int xdr_status;
	uint32_t *uint32_value_ptr;
	mx_bool_type connected;
	mx_status_type mx_status;

#if NETWORK_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,
	("%s invoked, attribute_number = '%#lx', attribute_value = '%g'",
			fname, attribute_number, attribute_value));

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	mx_status = mx_network_field_is_connected( nf, &connected );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( connected == FALSE ) {
		mx_status = mx_network_field_connect( nf );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	server = (MX_NETWORK_SERVER *) nf->server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			nf->server_record->name );
	}

	list_head = mx_get_record_list_head_struct( nf->server_record );

	if ( list_head->network_debug_flags & MXF_NETDBG_VERBOSE ) {
		MX_DEBUG(-2,("\n*** SET ATTRIBUTE for '%s'",
			mx_network_get_nf_label(
				nf->server_record, nf->nfname,
				nf_label, sizeof(nf_label) )
			));
	}

	if ( list_head->network_debug_flags & MXF_NETDBG_SUMMARY ) {
		mx_network_get_nf_label( nf->server_record, nf->nfname,
					nf_label, sizeof(nf_label) );

		fprintf( stderr, "MX SET_ATTRIBUTE('%s', %lu, %g )\n",
			nf_label, attribute_number, attribute_value );
	}

	/************ Send the 'set attribute' message. *************/

	aligned_buffer = server->message_buffer;

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length = mx_remote_header_length(server);

	header[MX_NETWORK_MAGIC]         = mx_htonl( MX_NETWORK_MAGIC_VALUE );
	header[MX_NETWORK_HEADER_LENGTH] = mx_htonl( header_length );
	header[MX_NETWORK_MESSAGE_TYPE]  = mx_htonl( MX_NETMSG_SET_ATTRIBUTE );
	header[MX_NETWORK_STATUS_CODE]   = mx_htonl( MXE_SUCCESS );

	message = buffer + header_length;
	uint32_message = header + (header_length / sizeof(uint32_t));

	uint32_message[0] = mx_htonl( nf->record_handle );
	uint32_message[1] = mx_htonl( nf->field_handle );
	uint32_message[2] = mx_htonl( attribute_number );

	uint32_value_ptr = &(uint32_message[3]);

	switch( server->data_format ) {
	case MX_NETWORK_DATAFMT_ASCII:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Message type MX_NETMSG_SET_ATTRIBUTE is not supported "
			"for ASCII format network connections." );
		break;

	case MX_NETWORK_DATAFMT_RAW:
		/* Do _not_ use mx_htonl() for the attribute value, since
		 * the raw value is actually an IEEE 754 double.
		 */

		u.double_value = attribute_value;

		uint32_value_ptr[0] = u.uint32_value[0];
		uint32_value_ptr[1] = u.uint32_value[1];

		message_length = 3 * sizeof(uint32_t) + sizeof(double);
		break;

#if HAVE_XDR
	case MX_NETWORK_DATAFMT_XDR:
		xdrmem_create( &xdrs, (void *)uint32_value_ptr, 8, XDR_ENCODE );

		xdr_status = xdr_double( &xdrs, &attribute_value );

		MXW_UNUSED( xdr_status );

		xdr_destroy( &xdrs );

		message_length = 3 * sizeof(uint32_t) + 8;
		break;
#endif /* HAVE_XDR */

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported data format %ld requested for MX server '%s'.",
			server->data_format, nf->server_record->name );
		break;
	}

	header[MX_NETWORK_MESSAGE_LENGTH] = mx_htonl( message_length );

	if ( mx_server_supports_message_ids(server) ) {

		header[MX_NETWORK_DATA_TYPE] = mx_htonl( MXFT_DOUBLE );

		mx_network_update_message_id( &(server->last_rpc_message_id) );

		header[MX_NETWORK_MESSAGE_ID] =
				mx_htonl( server->last_rpc_message_id );
	}

#if NETWORK_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mx_network_send_message( nf->server_record, aligned_buffer);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/************** Wait for the response. **************/

	mx_status = mx_network_wait_for_message_id( nf->server_record,
						aligned_buffer,
						server->last_rpc_message_id,
						server->timeout );
#if NETWORK_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, " " );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update these pointers in case they were
	 * changed by mx_reallocate_network_buffer()
	 * in mx_network_wait_for_message_id().
	 */

	header = &(aligned_buffer->u.uint32_buffer[0]);
	buffer = &(aligned_buffer->u.char_buffer[0]);

	header_length  = mx_ntohl( header[ MX_NETWORK_HEADER_LENGTH ] );
	message_length = mx_ntohl( header[ MX_NETWORK_MESSAGE_LENGTH ] );
	message_type   = mx_ntohl( header[ MX_NETWORK_MESSAGE_TYPE ] );
	status_code = (long) mx_ntohl( header[ MX_NETWORK_STATUS_CODE ] );

	if ( message_type != mx_server_response( MX_NETMSG_SET_ATTRIBUTE ) ) {

		if ( message_type == MX_NETMSG_UNEXPECTED_ERROR ) {

			if ( status_code == MXE_NOT_YET_IMPLEMENTED ) {

				/* The remote server is an old server that
				 * does not implement the 'set attribute'
				 * message.
				 */

				return mx_error(
				( status_code | MXE_QUIET ), fname,
				"The MX 'set attribute' message type is "
					"not implemented by server '%s'.",
						nf->server_record->name );
			} else {
				/* Some other error occurred. */

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Unexpected server error for MX server '%s'.  "
	"Server error was '%s -> %s'", nf->server_record->name,
				mx_status_code_string((long) status_code ),
					message );
			}
		}

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"Message type for response was not MX_NETMSG_SET_ATTRIBUTE.  "
"Instead it was of type = %#lx", (unsigned long) message_type );
	}

	/* If the remote command failed, the message field will include
	 * the text of the error message rather than the field type
	 * data we requested.
	 */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( (long)status_code, fname, "%s", message );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

static struct {
	unsigned long attribute_number;
	char attribute_name[MXU_FIELD_NAME_LENGTH+1];
} mxp_network_attribute_list[] = 
{
	{MXNA_VALUE_CHANGE_THRESHOLD,	"value_change_threshold"},
	{MXNA_POLL, 			"poll"},
	{MXNA_READ_ONLY,		"read_only"},
};

static unsigned long mxp_num_network_attributes =
			sizeof( mxp_network_attribute_list )
			/ sizeof( mxp_network_attribute_list[0] );

MX_EXPORT mx_status_type
mx_network_field_get_attribute_by_name( MX_NETWORK_FIELD *nf,
					char *attribute_name,
					double *attribute_value )
{
	static const char fname[] = "mx_network_field_get_attribute_by_name()";

	unsigned long i, attribute_number;
	char *list_attribute_name;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked, attribute_name = '%s'",
			fname, attribute_name));

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}
	if ( attribute_value == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"attribute_value pointer passed was NULL." );
	}

	for ( i = 0; i < mxp_num_network_attributes; i++ ) {
		list_attribute_name =
			mxp_network_attribute_list[i].attribute_name;

		if ( strcmp( attribute_name, list_attribute_name ) == 0 ) {

			attribute_number =
				mxp_network_attribute_list[i].attribute_number;

			mx_status = mx_network_field_get_attribute( nf,
							attribute_number,
							attribute_value );

			return mx_status;
		}
	}

	return mx_error( MXE_NOT_FOUND, fname,
	"Network field attribute '%s' does not exist.", attribute_name );
}

MX_EXPORT mx_status_type
mx_network_field_set_attribute_by_name( MX_NETWORK_FIELD *nf,
					char *attribute_name,
					double attribute_value )
{
	static const char fname[] = "mx_network_field_set_attribute_by_name()";

	unsigned long i, attribute_number;
	char *list_attribute_name;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked, attribute_name = '%s'",
			fname, attribute_name));

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}

	for ( i = 0; i < mxp_num_network_attributes; i++ ) {
		list_attribute_name =
			mxp_network_attribute_list[i].attribute_name;

		if ( strcmp( attribute_name, list_attribute_name ) == 0 ) {

			attribute_number =
				mxp_network_attribute_list[i].attribute_number;

			mx_status = mx_network_field_set_attribute( nf,
							attribute_number,
							attribute_value );

			return mx_status;
		}
	}

	return mx_error( MXE_NOT_FOUND, fname,
	"Network field attribute '%s' does not exist.", attribute_name );
}

MX_EXPORT mx_status_type
mx_network_field_get_attribute_number( MX_NETWORK_FIELD *nf,
					char *attribute_name,
					unsigned long *attribute_number )
{
	static const char fname[] = "mx_network_field_get_attribute_number()";

	unsigned long i;
	char *list_attribute_name;

	MX_DEBUG( 2,("%s invoked, attribute_name = '%s'",
			fname, attribute_name));

	if ( nf == (MX_NETWORK_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_FIELD pointer passed was NULL." );
	}
	if ( attribute_number == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"attribute_number pointer passed was NULL." );
	}

	for ( i = 0; i < mxp_num_network_attributes; i++ ) {
		list_attribute_name =
			mxp_network_attribute_list[i].attribute_name;

		if ( strcmp( attribute_name, list_attribute_name ) == 0 ) {

			*attribute_number =
				mxp_network_attribute_list[i].attribute_number;

			return MX_SUCCESSFUL_RESULT;
		}
	}

	return mx_error( MXE_NOT_FOUND, fname,
	"Network field attribute '%s' does not exist.", attribute_name );
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_request_data_format( MX_RECORD *server_record,
				unsigned long requested_format )
{
	static const char fname[] = "mx_network_request_data_format()";

	MX_NETWORK_SERVER *server;
	MX_NETWORK_MESSAGE_BUFFER *message_buffer;
	unsigned long local_native_data_format, remote_native_data_format;
	unsigned long remote_wordsize;
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	message_buffer = server->message_buffer;

	if ( message_buffer == (MX_NETWORK_MESSAGE_BUFFER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_NETWORK_MESSAGE_BUFFER pointer for server '%s' is NULL.",
	    		server_record->name );
	}

	/* If the caller has not requested automatic data format negotiation,
	 * just attempt to set the requested format and fail if it is not
	 * available.
	 */

	if ( requested_format != MX_NETWORK_DATAFMT_NEGOTIATE ) {

		mx_status = mx_network_set_option( server_record,
			MX_NETWORK_OPTION_DATAFMT, requested_format );

		if ( mx_status.code == MXE_SUCCESS ) {
			server->data_format = requested_format;

			message_buffer->data_format = requested_format;

			return MX_SUCCESSFUL_RESULT;
		} else
		if ( ( mx_status.code == MXE_NOT_YET_IMPLEMENTED )
		  && ( requested_format != MX_NETWORK_DATAFMT_ASCII ) )
		{
			/* The server does not support anything other
			 * than ASCII format.
			 */

			server->data_format = MX_NETWORK_DATAFMT_ASCII;

			message_buffer->data_format = MX_NETWORK_DATAFMT_ASCII;

			return mx_error( mx_status.code, fname,
			"MX server '%s' only supports ASCII data format.",
				server_record->name );
		} else {
			return mx_status;
		}
	}

	/* We have been requested to perform automatic data format negotiation.
	 * To do this, we must first get the local native data format.
	 */

	local_native_data_format = mx_native_data_format();

	/* Find out the native data format of the remote server. */

	mx_status = mx_network_get_option( server_record,
			MX_NETWORK_OPTION_NATIVE_DATAFMT,
			&remote_native_data_format );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_YET_IMPLEMENTED:

		/* The server is not new enough to have 'get option' and
		 * 'set option' implemented in it, so we have to settle
		 * for ASCII data format.
		 */

		MX_DEBUG( 2,
			("%s: Server '%s' only supports ASCII network format.",
	 		fname, server_record->name ));

		server->data_format = MX_NETWORK_DATAFMT_ASCII;

		message_buffer->data_format = MX_NETWORK_DATAFMT_ASCII;
		
		/* Cannot do any further negotiation, so we just return now. */

		return MX_SUCCESSFUL_RESULT;
	default:
		return mx_status;
	}

	MX_DEBUG( 2,
    ("%s: local_native_data_format = %#lx, remote_native_data_format = %#lx",
     		fname, local_native_data_format, remote_native_data_format));

	/* Find out the native wordsize of the remote server. */

	mx_status = mx_network_get_option( server_record,
			MX_NETWORK_OPTION_WORDSIZE | MXE_QUIET,
			&remote_wordsize );

	MX_DEBUG( 2,("%s: mx_status.code = %lu, remote_wordsize = %lu",
		fname, mx_status.code, remote_wordsize));

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_ILLEGAL_ARGUMENT:
		/* MX 1.5.0 and before do not support the option
		 * MX_NETWORK_OPTION_WORDSIZE, so we make the
		 * _assumption_ that the remote wordsize is 32,
		 * since almost all old MX installations are 
		 * running on 32-bit machines.
		 */

		remote_wordsize = 32;
		break;
	default:
		return mx_status;
		break;
	}

	/* Try to select a binary data format. */

	if ( ( local_native_data_format == remote_native_data_format )
	  && ( MX_WORDSIZE == remote_wordsize ) )
	{
		/* The local and remote data formats are the same and the
		 * wordsizes are the same, so we should try raw format.
		 */

		requested_format = MX_NETWORK_DATAFMT_RAW;
	} else {
		/* Otherwise, we should use XDR format. */

		requested_format = MX_NETWORK_DATAFMT_XDR;
	}

	MX_DEBUG( 2,("%s: requested_format = %lu", fname, requested_format));

	mx_status = mx_network_set_option( server_record,
			MX_NETWORK_OPTION_DATAFMT, requested_format );

	switch ( mx_status.code ) {
	case MXE_SUCCESS:
		/* If the 'set option' command succeeded, we are done. */

		MX_DEBUG( 2,("%s: binary data format successfully selected.",
			fname));

		server->data_format = requested_format;

		message_buffer->data_format = requested_format;

		return MX_SUCCESSFUL_RESULT;
	case MXE_NOT_YET_IMPLEMENTED:
		/* The server does not implement 'set option'. */

		MX_DEBUG( 2,("%s: Server '%s' does not support 'set option'.",
			fname, server_record->name));

		server->data_format = MX_NETWORK_DATAFMT_ASCII;

		message_buffer->data_format = MX_NETWORK_DATAFMT_ASCII;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Selecting a binary data format failed, so try ASCII format. */

	mx_status = mx_network_set_option( server_record,
			MX_NETWORK_OPTION_DATAFMT, MX_NETWORK_DATAFMT_ASCII );

	/* Even if selecting ASCII format failed, we have no alternative
	 * to assuming ASCII format is in use.  So we make this assumption
	 * and hope for the best.
	 */

	server->data_format = MX_NETWORK_DATAFMT_ASCII;

	message_buffer->data_format = MX_NETWORK_DATAFMT_ASCII;

	if ( mx_status.code == MXE_SUCCESS ) {
		MX_DEBUG( 2,("%s: ASCII data format successfully selected.",
				fname));
	} else {
		MX_DEBUG( 2,("%s: Assuming ASCII data format is in use.",
				fname));
	}

	return MX_SUCCESSFUL_RESULT;
}


/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_request_64bit_longs( MX_RECORD *server_record,
				mx_bool_type use_64bit_network_longs )
{
	static const char fname[] = "mx_network_request_64bit_longs()";

	MX_NETWORK_SERVER *server;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	server = (MX_NETWORK_SERVER *) server_record->record_class_struct;

	if ( server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			server_record->name );
	}

	MX_DEBUG( 2,("%s: request 64-bit network longs = %d for server '%s'",
		fname, (int) use_64bit_network_longs, server_record->name ));

	server->use_64bit_network_longs = FALSE;

#if ( MX_WORDSIZE < 64 )
	/* On 32-bit computers, it is OK to turn off 64-bit network longs,
	 * but it is not OK to try to turn them on.
	 */

	if ( use_64bit_network_longs ) {
		return mx_error( MXE_UNSUPPORTED, fname,
			"The client computer is not a 64-bit computer, "
			"so it cannot request 64-bit longs for network "
			"communication." );
	}

#else   /* MX_WORDSIZE == 64 */

	switch( server->data_format ) {
	case MX_NETWORK_DATAFMT_RAW:
		/* This is the only supported case. */

		break;
	case MX_NETWORK_DATAFMT_XDR:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Network communication with server '%s' is using XDR data "
		"format which does not support 64-bit network longs.",
			server_record->name );

	case MX_NETWORK_DATAFMT_ASCII:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Network communication with server '%s' is using ASCII data "
		"format which does not support 64-bit network longs.",
			server_record->name );

	default:
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The network communication data structures for server '%s' "
		"claim that they are using unrecognized data format %lu.  "
		"This should _never_ happen and should be reported to "
		"the software developers for MX.",
			server_record->name, server->data_format );
	}

#endif  /* MX_WORDSIZE == 64 */

	if ( server->remote_mx_version < 1002000 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The remote server cannot support 64-bit network longs since "
		"it is running MX version %ld.%ld.%ld.  The oldest version "
		"of MX that supports 64-bit network longs is MX 1.2.0.",
			server->remote_mx_version / 1000000,
			( server->remote_mx_version % 1000000 ) / 1000,
			server->remote_mx_version % 1000 );
	}

	mx_status = mx_network_set_option( server_record,
					MX_NETWORK_OPTION_64BIT_LONG,
					use_64bit_network_longs );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		server->use_64bit_network_longs = use_64bit_network_longs;
		break;
	default:
		server->use_64bit_network_longs = FALSE;

		return mx_status;
	}

	MX_DEBUG( 2,("%s: server->use_64bit_network_longs = %d",
		fname, (int) server->use_64bit_network_longs));

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_send_client_version( MX_RECORD *server_record )
{
	static const char fname[] = "mx_network_send_client_version()";

	MX_LIST_HEAD *list_head;
	unsigned long client_mx_version;
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer found for record '%s' is NULL.",
			server_record->name );
	}

	client_mx_version = list_head->mx_version;

	mx_status = mx_network_set_option( server_record,
				MX_NETWORK_OPTION_CLIENT_VERSION | MXE_QUIET,
				client_mx_version );

	/* Do _NOT_ return an error if the MX server sent us
	 * an MXE_ILLEGAL_ARGUMENT error code.  This just
	 * means that this MX server does not know about the
	 * MX_NETWORK_OPTION_CLIENT_VERSION option code.
	 */

	if ( mx_status.code == MXE_ILLEGAL_ARGUMENT ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_status;
	}
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_send_client_version_time( MX_RECORD *server_record )
{
	static const char fname[] = "mx_network_send_client_version_time()";

	MX_LIST_HEAD *list_head;
	uint64_t client_mx_version_time;
	mx_status_type mx_status;

	if ( server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"server_record argument passed was NULL." );
	}

	list_head = mx_get_record_list_head_struct( server_record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer found for record '%s' is NULL.",
			server_record->name );
	}

	client_mx_version_time = list_head->mx_version_time;

	/* FIXME: The client MX version time should be sent as a 64-bit
	 * integer.  Rather than create 64-bit options, it would be better
	 * to add a way of reading and writing socket attributes in the
	 * server's MX_SOCKET_HANDLER structure that can use _any_ MX
	 * data type.  This would probably be some sort of get(), put()
	 * interface similar to the network field and network attribute
	 * interfaces.
	 */

	mx_status = mx_network_set_option( server_record,
			MX_NETWORK_OPTION_CLIENT_VERSION_TIME | MXE_QUIET,
			(unsigned long) client_mx_version_time );

	/* Do _NOT_ return an error if the MX server sent us
	 * an MXE_ILLEGAL_ARGUMENT error code.  This just
	 * means that this MX server does not know about the
	 * MX_NETWORK_OPTION_CLIENT_VERSION_TIME option code.
	 */

	if ( mx_status.code == MXE_ILLEGAL_ARGUMENT ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_status;
	}
}

/* ====================================================================== */

/* The most general form of an MX network field address is as follows:
 *
 *	server_name@server_arguments:record_name.field_name
 *
 * where server_name and server_arguments are optional.
 *
 * Examples:
 *
 *      localhost@9727:mx_database.mx_version
 *      myserver:theta.position
 *      192.168.1.1:omega.positive_limit_hit
 *      edge_energy.value
 */

MX_EXPORT mx_status_type
mx_parse_network_field_id( char *network_field_id,
		char *server_name, size_t max_server_name_length,
		char *server_arguments, size_t max_server_arguments_length,
		char *record_name, size_t max_record_name_length,
		char *field_name, size_t max_field_name_length )
{
	static const char fname[] = "mx_parse_network_field_id()";

	char *server_name_ptr, *server_arguments_ptr;
	char *record_name_ptr, *field_name_ptr;
	char *at_sign_ptr, *colon_ptr, *period_ptr;
	size_t server_name_length, server_arguments_length;
	size_t record_name_length, field_name_length;

	if ( (network_field_id == NULL) || (server_name == NULL)
	  || (server_arguments == NULL) || (record_name == NULL)
	  || (field_name == NULL) )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One of the arguments passed to this function was NULL." );
	}

	server_name_length = 0;
	server_arguments_length = 0;
	record_name_length = 0;
	field_name_length = 0;

	/* Look for the first '@' character and the first ':' character
	 * that follows the '@' character.
	 */

	at_sign_ptr = strchr( network_field_id, '@' );

	if ( at_sign_ptr == NULL ) {
		colon_ptr = strchr( network_field_id, ':' );
	} else {
		colon_ptr = strchr( at_sign_ptr, ':' );
	}

	/* In addition, we look for the _last_ '.' character in
	 * the network_field_id.
	 */

	period_ptr = strrchr( network_field_id, '.' );

	if ( period_ptr == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The period '.' character that is supposed to separate "
		"the field name from the record name was not found in the "
		"line containing '%s'.", network_field_id );
	}

	field_name_ptr = period_ptr + 1;

	field_name_length = strlen( field_name_ptr ) + 1;

	/* We do several different things depending on whether the
	 * at_sign_ptr and/or the colon_ptr are NULL.
	 */

	server_name_ptr = NULL;		server_name_length = 0;
	server_arguments_ptr = NULL;	server_arguments_length = 0;
	record_name_ptr = NULL;		record_name_length = 0;

	if ( at_sign_ptr == NULL ) {
		if ( colon_ptr == NULL ) {
			/* Everything before the last period is the
			 * record name.
			 */

			MX_DEBUG( 2,("%s: CASE #11", fname));

			record_name_ptr = network_field_id;
			record_name_length = period_ptr - network_field_id + 1;
		} else {
			/* Everything before the first colon is the
			 * server name.
			 */

			MX_DEBUG( 2,("%s: CASE #12", fname));

			server_name_ptr = network_field_id;
			server_name_length = colon_ptr - network_field_id + 1;

			record_name_ptr = colon_ptr + 1;
			record_name_length = period_ptr - record_name_ptr + 1;
		}
	} else {
		/* at_sign_ptr is _not_ NULL */

		if ( colon_ptr == NULL ) {
			/* A name with an '@' character in it, is required
			 * to have a ':' character somewhere after it.  
			 * Therefore, this particular combination is
			 * illegal.
			 */

			MX_DEBUG( 2,("%s: CASE #21", fname));

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The '@' character was used in network field id '%s' "
			"without a ':' character somewhere after it.",
				network_field_id );
		} else {
			/* at_sign_ptr and colon_ptr are both not NULL. */

			MX_DEBUG( 2,("%s: CASE #22", fname));

			server_name_ptr = network_field_id;
			server_name_length = at_sign_ptr - network_field_id + 1;

			server_arguments_ptr = at_sign_ptr + 1;
			server_arguments_length =
					colon_ptr - server_arguments_ptr + 1;

			record_name_ptr = colon_ptr + 1;
			record_name_length = period_ptr - record_name_ptr + 1;
		}
	}

	MX_DEBUG( 2,("%s: #1 server_name_ptr = '%s', server_name_length = %ld",
		fname, server_name_ptr, (long) server_name_length));

	MX_DEBUG( 2,
	("%s: #1 server_arguments_ptr = '%s', server_arguments_length = %ld",
		fname, server_arguments_ptr, (long) server_arguments_length));

	MX_DEBUG( 2,("%s: #1 record_name_ptr = '%s', record_name_length = %ld",
		fname, record_name_ptr, (long) record_name_length));

	MX_DEBUG( 2,("%s: #1 field_name_ptr = '%s', field_name_length = %ld",
		fname, field_name_ptr, (long) field_name_length));

	/* Make sure that the name lengths do not exceed the maximums
	 * specified by the calling routine.
	 */

	if ( server_name_length > max_server_name_length )
		server_name_length = max_server_name_length;

	if ( server_arguments_length > max_server_arguments_length )
		server_arguments_length = max_server_arguments_length;

	if ( record_name_length > max_record_name_length )
		record_name_length = max_record_name_length;

	if ( field_name_length > max_field_name_length )
		field_name_length = max_field_name_length;

	/* Finally, copy the strings to the caller's buffers. */

	if ( server_name_ptr != NULL ) {
		strlcpy( server_name, server_name_ptr, server_name_length );
	} else {
		strlcpy( server_name, "localhost", max_server_name_length );
	}

	if ( server_arguments_ptr != NULL ) {
		strlcpy( server_arguments, server_arguments_ptr,
						server_arguments_length );
	} else {
		strlcpy( server_arguments, "9727",
						max_server_arguments_length );
	}

	if ( record_name_ptr != NULL ) {
		strlcpy( record_name, record_name_ptr, record_name_length );
	} else {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Somehow when we got to here, record_name_ptr was NULL.  "
		"This should not be able to happen." );
	}

	if ( field_name_ptr != NULL ) {
		strlcpy( field_name, field_name_ptr, field_name_length );
	} else {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Somehow when we got to here, field_name_ptr was NULL.  "
		"This should not be able to happen." );
	}

	MX_DEBUG( 2,("%s: #2 server_name = '%s', server_name_length = %ld",
		fname, server_name, (long) server_name_length));

	MX_DEBUG( 2,
	("%s: #2 server_arguments = '%s', server_arguments_length = %ld",
		fname, server_arguments, (long) server_arguments_length));

	MX_DEBUG( 2,("%s: #2 record_name = '%s', record_name_length = %ld",
		fname, record_name, (long) record_name_length));

	MX_DEBUG( 2,("%s: #2 field_name = '%s', field_name_length = %ld",
		fname, field_name, (long) field_name_length));

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_get_mx_server_record( MX_RECORD *record_list,
			char *server_name,
			char *server_arguments,
			MX_RECORD **server_record,
			double server_connection_timeout )
{
	static const char fname[] = "mx_get_mx_server_record()";

	static const char number_digits[] = "0123456789";

	MX_RECORD *current_record, *new_record;
	MX_TCPIP_SERVER *tcpip_server;
	MX_UNIX_SERVER *unix_server;
	int port_number;
	long server_type;
	size_t arguments_length, strspn_length;
	char description[200];
	char format[100];
	MX_CLOCK_TICK timeout_ticks, current_tick, finish_tick;
	unsigned long sleep_ms;
	int comparison;
	mx_bool_type wait_forever = FALSE;
	mx_bool_type exit_while_loop = FALSE;
	mx_status_type mx_status;

	if ( record_list == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The record_list pointer passed was NULL." );
	}
	if ( server_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The server_name pointer passed was NULL." );
	}
	if ( server_arguments == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The server_arguments pointer passed was NULL." );
	}
	if ( server_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The server_record pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s: server_name = '%s', server_arguments = '%s'",
			fname, server_name, server_arguments));

	/* Figure out what kind of server we are looking for.
	 *
	 * We assume that if 'server_arguments' contains only the
	 * number digits 0-9, then it is referring to a TCP port 
	 * number.  Otherwise, we assume that it is a Unix domain
	 * socket.
	 */

	port_number = atoi( server_arguments );

	MX_DEBUG( 2,("%s: port_number = %d, server_arguments = '%s'",
			 	fname, port_number, server_arguments));


	arguments_length = strlen( server_arguments );

	strspn_length = strspn( server_arguments, number_digits );

	if ( strspn_length == arguments_length ) {
		server_type = MXN_NET_TCPIP;

		if ( strlen( server_name ) == 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The server name passed for the server "
			"is of zero length." );
		}
	} else {
		server_type = MXN_NET_UNIX;
	}

	MX_DEBUG( 2,("%s: server_type = %ld", fname, server_type));

	/* Be sure to start at the beginning of the record list. */

	record_list = record_list->list_head;

	current_record = record_list->next_record;

	/* Loop through the record list looking for a matching record. */

	while ( current_record != record_list ) {

	    if ( server_type == current_record->mx_type ) {
		/* We found a driver type match, so examine this
		 * record further.
		 */

		MX_DEBUG( 2,("%s: driver match for record '%s'",
			fname, current_record->name));

		switch( server_type ) {
		case MXN_NET_TCPIP:
		    tcpip_server = (MX_TCPIP_SERVER *)
					current_record->record_type_struct;

		    if ( (strcmp(server_name, tcpip_server->hostname) == 0)
		      && (port_number == tcpip_server->port) )
		    {
			/* We have found a matching record, so return it. */

			*server_record = current_record;
			return MX_SUCCESSFUL_RESULT;
		    }
		    break;

		case MXN_NET_UNIX:
		    unix_server = (MX_UNIX_SERVER *)
					current_record->record_type_struct;

		    if (strcmp(server_arguments, unix_server->pathname) == 0 )
		    {
			/* We have found a matching record, so return it. */

			*server_record = current_record;
			return MX_SUCCESSFUL_RESULT;
		    }
		    break;

		default:
		    /* Continue on to the next record. */
		    break;
		}
	    }

	    current_record = current_record->next_record;
	}

	/* If we get this far, there is no matching record in the database,
	 * so we create one now on the spot.
	 */

	switch( server_type ) {
	case MXN_NET_TCPIP:
		snprintf( format, sizeof(format),
	"s%%-.%ds server network tcp_server \"\" \"\" 0x0 %%s %%d",
			MXU_RECORD_NAME_LENGTH-2 );

		MX_DEBUG( 2,("%s: format = '%s'", fname, format));
		MX_DEBUG( 2,("%s: server_name = '%s', port_number = %d",
			fname, server_name, port_number));

		snprintf( description, sizeof(description), format,
				server_name, server_name, port_number );
		break;

	case MXN_NET_UNIX:
		snprintf( format, sizeof(format),
	"s%%-.%ds server network unix_server \"\" \"\" 0x0 %%s",
			MXU_RECORD_NAME_LENGTH-2 );

		MX_DEBUG( 2,("%s: format = '%s'", fname, format));
		MX_DEBUG( 2,("%s: server_name = '%s', server_arguments = '%s'",
			fname, server_name, server_arguments));

		snprintf( description, sizeof(description),
				format, server_name, server_arguments );
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The attempt to create a server record of MX type %ld "
		"was unsuccessful since the requested driver type "
		"is not MXN_NET_TCPIP or MXN_NET_UNIX.  "
		"This should never happen.", server_type );
	}

	MX_DEBUG( 2,("%s: description = '%s'",
			fname, description));

	mx_status = mx_create_record_from_description( record_list,
					description, &new_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_finish_record_initialization( new_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The timeout and retry loop are here mainly for the use of the
	 * 'mxautosave' program.  'mxautosave' starts at approximately
	 * the same time as the 'mxserver' program, so we need to be
	 * able to wait for 'mxserver'.
	 *
	 * Note: A timeout that is < 0 is considered a request to 
	 * wait forever until the connection succeeds.
	 */

	if ( server_connection_timeout < 0.0 ) {
		wait_forever = TRUE;
	} else {
		wait_forever = FALSE;

		timeout_ticks = mx_convert_seconds_to_clock_ticks(
						server_connection_timeout );

		current_tick = mx_current_clock_tick();

		finish_tick = mx_add_clock_ticks( current_tick, timeout_ticks );
	}

	sleep_ms = 1000;

	exit_while_loop = FALSE;

	while (1) {
		mx_status = mx_open_hardware( new_record );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			exit_while_loop = TRUE;
			break;

		case MXE_NETWORK_CONNECTION_REFUSED:
		case MXE_NETWORK_IO_ERROR:
			/* We need to go back and try again. */
			break;

		default:
			return mx_status;
		}

		if ( exit_while_loop ) {
			break;
		}

		if ( wait_forever ) {
			mx_msleep(sleep_ms);
			continue;
		}

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks(current_tick, finish_tick);

		if ( comparison > 0 ) {
			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %f seconds to connect "
			"to the MX server at '%s' on port %d.",
				server_connection_timeout,
				server_name, port_number );
		}

		mx_msleep(sleep_ms);
	}

	*server_record = new_record;

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_network_copy_message_to_field( MX_RECORD *source_server_record,
				MX_RECORD_FIELD *destination_field )
{
	static const char fname[] = "mx_network_copy_message_to_field()";

	MX_NETWORK_SERVER *source_server;
	MX_NETWORK_MESSAGE_BUFFER *source_message_buffer;
	unsigned long header_length, message_length;
	long status_code;
	uint32_t *source_header;
	char *char_header, *char_message;
	void *value_ptr;
	mx_bool_type dynamically_allocated;
	mx_status_type mx_status;

	if ( source_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( destination_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	/* Find everything we need to know about the server's message buffer. */

	source_server =
		(MX_NETWORK_SERVER *) source_server_record->record_class_struct;

	if ( source_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for server record '%s' is NULL.",
			source_server_record->name );
	}

	source_message_buffer = source_server->message_buffer;

	if ( source_message_buffer == ( MX_NETWORK_MESSAGE_BUFFER * ) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The message_buffer pointer for server '%s' is NULL.",
			source_server_record->name );
	}

	/* Figure out the address of the message buffer body for the server. */

	source_header = source_message_buffer->u.uint32_buffer;

	if ( source_header == (uint32_t *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The uint32_buffer pointer for server '%s' is NULL.",
			source_server_record->name );
	}

	/* 'char_message' is the address of the data that we need to copy
	 * to the MX record field.  'message_length' is the length in bytes
	 * of the data to copy to the MX record field.
	 */

	header_length = mx_ntohl( source_header[MX_NETWORK_HEADER_LENGTH] );

	message_length = mx_ntohl( source_header[MX_NETWORK_MESSAGE_LENGTH] );

	status_code = (long) mx_ntohl( source_header[MX_NETWORK_STATUS_CODE] );

	char_header = source_message_buffer->u.char_buffer;

	char_message = char_header + header_length;

	/* If the server response is an error message, then return. */

	if ( status_code != MXE_SUCCESS ) {
		return mx_error( status_code, fname, "%s", char_message );
	}

	/* 'value_ptr' is the address at which the record field value
	 * is stored at.
	 */

	value_ptr = mx_get_field_value_pointer( destination_field );

	if ( destination_field->flags & MXFF_VARARGS ) {
		dynamically_allocated = TRUE;
	} else {
		dynamically_allocated = FALSE;
	}

	/* Now we are ready to copy the data. */

	switch( source_message_buffer->data_format ) {
	case MX_NETWORK_DATAFMT_ASCII:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Support for the ASCII MX data format "
			"has not been implemented.");
		break;

	case MX_NETWORK_DATAFMT_RAW:
		mx_status = mx_copy_network_buffer_to_array(
				char_message,
				message_length,
				value_ptr,
				dynamically_allocated,
				destination_field->datatype,
				destination_field->num_dimensions,
				destination_field->dimension,
				destination_field->data_element_size,
				NULL,
				source_server->use_64bit_network_longs );
		break;

	case MX_NETWORK_DATAFMT_XDR:
		mx_status = mx_xdr_data_transfer( MX_XDR_DECODE,
				value_ptr,
				dynamically_allocated,
				destination_field->datatype,
				destination_field->num_dimensions,
				destination_field->dimension,
				destination_field->data_element_size,
				char_message,
				message_length,
				NULL );
		break;

	default:
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"MX server '%s' is configured for unrecognized "
			"network data format %lu",
				source_server_record->name,
				source_message_buffer->data_format );
		break;
	}

	return mx_status;
}

