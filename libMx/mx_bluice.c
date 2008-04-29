/*
 * Name:    mx_bluice.c
 *
 * Purpose: Client side part of Blu-Ice network protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_bluice.h"
#include "n_bluice_dcss.h"

#define BLUICE_DEBUG_SETUP	FALSE

#define BLUICE_DEBUG_MESSAGE	TRUE

#define BLUICE_DEBUG_CONFIG	TRUE

#define BLUICE_DEBUG_MOTION	TRUE

static mx_status_type
mx_bluice_get_pointers( MX_RECORD *bluice_server_record,
			MX_BLUICE_SERVER **bluice_server,
			MX_SOCKET **bluice_server_socket,
			const char *calling_fname )
{
	static const char fname[] = "mx_bluice_get_pointers()";

	MX_BLUICE_SERVER *bluice_server_ptr;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_server_ptr = (MX_BLUICE_SERVER *)
				bluice_server_record->record_class_struct;

	if ( bluice_server_ptr == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for record '%s' is NULL.",
			bluice_server_record->name );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = bluice_server_ptr;
	}

	if ( bluice_server_socket != (MX_SOCKET **) NULL ) {
		*bluice_server_socket = bluice_server_ptr->socket;

		if ( *bluice_server_socket == NULL ) {
			return mx_error(
				MXE_NETWORK_CONNECTION_LOST, calling_fname,
				"Blu-Ice server '%s' is not connected.",
					bluice_server_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_send_message( MX_RECORD *bluice_server_record,
			char *text_data,
			char *binary_data,
			long binary_data_length )
{
	static const char fname[] = "mx_bluice_send_message()";

	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *bluice_server_socket;
	char message_header[MX_BLUICE_MSGHDR_LENGTH+1];
	char null_pad[500];
	long text_data_length, bytes_sent, null_bytes_to_send;
	mx_status_type mx_status;
	long mx_status_code;

	bluice_server = NULL;
	bluice_server_socket = NULL;

	mx_status = mx_bluice_get_pointers( bluice_server_record,
				&bluice_server, &bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_DEBUG_MESSAGE
	MX_DEBUG(-3,("%s: sending '%s' to server '%s'.",
			fname, text_data, bluice_server_record->name ));

	if ( ( binary_data_length > 0 ) && ( binary_data != NULL ) ) {
		long i;

		fprintf(stderr, "%s: Binary data: ", fname);

		for ( i = 0; i < binary_data_length; i++ ) {
			fprintf(stderr, "%#x ", binary_data[i]);
		}
		fprintf(stderr, "\n");
	}
#endif

	bytes_sent = 0;

	text_data_length = (long) (strlen( text_data ) + 1);

	mx_status_code = mx_mutex_lock( bluice_server->socket_send_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the socket send mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	if ( bluice_server->protocol_version > MX_BLUICE_PROTOCOL_1 ) {

		snprintf( message_header, MX_BLUICE_MSGHDR_LENGTH,
			"%*lu%*lu",
			MX_BLUICE_MSGHDR_TEXT_LENGTH,
			(unsigned long) text_data_length,
			MX_BLUICE_MSGHDR_BINARY_LENGTH,
			(unsigned long) binary_data_length );

		/* Send the message header. */

		mx_status = mx_socket_send( bluice_server_socket,
					&message_header,
					MX_BLUICE_MSGHDR_LENGTH );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_mutex_unlock( bluice_server->socket_send_mutex );
			return mx_status;
		}

		bytes_sent += MX_BLUICE_MSGHDR_LENGTH;
	}

	/* Send the text data. */

	mx_status = mx_socket_send( bluice_server_socket,
					text_data,
					text_data_length );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_mutex_unlock( bluice_server->socket_send_mutex );
		return mx_status;
	}

	bytes_sent += text_data_length;

	/* Send the binary data (if any). */

	if ( ( binary_data_length > 0 ) && ( binary_data != NULL ) ) {
		mx_status = mx_socket_send( bluice_server_socket,
						binary_data,
						binary_data_length );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_mutex_unlock( bluice_server->socket_send_mutex );
			return mx_status;
		}

		bytes_sent += binary_data_length;
	}

	if ( bluice_server->protocol_version == MX_BLUICE_PROTOCOL_1 ) {

		if ( bytes_sent < MX_BLUICE_OLD_MESSAGE_LENGTH ) {

			null_bytes_to_send =
				MX_BLUICE_OLD_MESSAGE_LENGTH - bytes_sent;

			memset( null_pad, 0, null_bytes_to_send );

			mx_status = mx_socket_send( bluice_server_socket,
						null_pad,
						null_bytes_to_send );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_mutex_unlock(
					bluice_server->socket_send_mutex );

				return mx_status;
			}
		}
	}

	mx_mutex_unlock( bluice_server->socket_send_mutex );

	return mx_status;
}

/* ====================================================================== */

/* WARNING: After the Blu-Ice server record's open() routine has run, only
 * the server receive thread may invoke mx_bluice_receive_message().
 */

MX_EXPORT mx_status_type
mx_bluice_receive_message( MX_RECORD *bluice_server_record,
				char *data_buffer,
				long data_buffer_length,
				long *actual_data_length,
				double timeout_in_seconds )
{
	static const char fname[] = "mx_bluice_receive_message()";

	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *bluice_server_socket;
	char message_header[MX_BLUICE_MSGHDR_LENGTH+1];
	long text_data_length, binary_data_length, total_data_length;
	long maximum_length;
	size_t actual_bytes_received;
	long num_bytes_available;
	unsigned long i, wait_ms, max_attempts;
	char *data_pointer;
	mx_status_type mx_status;

	bluice_server = NULL;
	bluice_server_socket = NULL;

	mx_status = mx_bluice_get_pointers( bluice_server_record,
				&bluice_server, &bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for a new message to come in.
	 *
	 * A negative timeout means wait forever, so we only need to loop
	 * if the timeout is >= 0.
	 */

	if ( timeout_in_seconds >= 0.0 ) {
		wait_ms = 10;
		max_attempts =
		    mx_round((1000.0 * timeout_in_seconds) / (double) wait_ms);

		for ( i = 0; i < max_attempts; i++ ) {
			mx_status = mx_socket_num_input_bytes_available(
					bluice_server->socket,
					&num_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_bytes_available != 0 ) {
				break;		/* Exit the for() loop. */
			}

			mx_msleep( wait_ms );
		}

		if ( i >= max_attempts ) {
			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after %g seconds of waiting for "
			"Blu-Ice server '%s' to send a message.",
				timeout_in_seconds,
				bluice_server_record->name );
		}
	}

	/* Read in the header from the Blu-Ice server. */

	mx_status = mx_socket_receive( bluice_server_socket,
					message_header,
					MX_BLUICE_MSGHDR_LENGTH,
					NULL, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the message header is null terminated. */

	message_header[MX_BLUICE_MSGHDR_LENGTH-1] = '\0';

	/* Get the length of the binary data. */

	binary_data_length = atol( &message_header[MX_BLUICE_MSGHDR_BINARY] );

	if ( binary_data_length != 0 ) {
		mx_warning( "The binary data length for the message being "
		"received from Blu-Ice server '%s' is not 0.  "
		"Instead, it is %lu.",
			bluice_server_record->name,
			(unsigned long) binary_data_length );
	}

	/* Get the length of the text data. */

	message_header[MX_BLUICE_MSGHDR_BINARY] = '\0'; /* Null terminate it. */

	text_data_length = atol( message_header );

	total_data_length = text_data_length + binary_data_length;

#if 0
	MX_DEBUG(-2,("%s: text = %lu, binary = %lu, total = %lu",
		fname, (unsigned long) text_data_length,
		(unsigned long) binary_data_length,
		(unsigned long) total_data_length));
#endif

	if ( data_buffer == NULL ) {
		data_pointer = bluice_server->receive_buffer;
		maximum_length = bluice_server->receive_buffer_length;
	} else {
		data_pointer = data_buffer;
		maximum_length = data_buffer_length;
	}

	if ( total_data_length > maximum_length ) {
		mx_warning( "Truncating a %lu byte message body from "
		"Blu-Ice server '%s' to %lu bytes.",
			(unsigned long) total_data_length,
			bluice_server_record->name,
			(unsigned long) maximum_length );

		total_data_length = maximum_length;
	}

	/* Now receive the data for the message. */

	mx_status = mx_socket_receive( bluice_server_socket,
					data_pointer,
					total_data_length,
					&actual_bytes_received,
					NULL, 0 );

#if BLUICE_DEBUG_MESSAGE
	MX_DEBUG(-3,("%s: received '%s' from server '%s'.",
		fname, data_pointer, bluice_server_record->name));
#endif

	*actual_data_length = (long) actual_bytes_received;

	if ( actual_bytes_received < maximum_length ) {
		data_pointer[actual_bytes_received] = '\0';
	}

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_get_message_type( char *message_string, long *message_type )
{
	static const char fname[] = "mx_bluice_get_message_type()";

	if ( message_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'message_string' pointer passed was NULL." );
	}
	if ( message_type == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'message_type' pointer passed was NULL." );
	}

	if ( strncmp( message_string, "gtos", 4 ) == 0 ) {
		*message_type = MXT_BLUICE_GTOS;
	} else
	if ( strncmp( message_string, "stog", 4 ) == 0 ) {
		*message_type = MXT_BLUICE_STOG;
	} else
	if ( strncmp( message_string, "htos", 4 ) == 0 ) {
		*message_type = MXT_BLUICE_HTOS;
	} else
	if ( strncmp( message_string, "stoh", 4 ) == 0 ) {
		*message_type = MXT_BLUICE_STOH;
	} else
	if ( strncmp( message_string, "stoc", 4 ) == 0 ) {
		*message_type = MXT_BLUICE_STOC;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Blu-Ice message '%s' starts with an illegal message type.",
			message_string);
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------*/

static mx_status_type
mx_bluice_device_pointer_fn( MX_BLUICE_SERVER *bluice_server,
			char *name,
			MX_BLUICE_FOREIGN_DEVICE ***foreign_device_array_ptr,
			long *num_foreign_devices_ptr,
			mx_bool_type make_new_pointer_if_necessary,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr )
{
	static const char fname[] = "mx_bluice_device_pointer_fn()";

	size_t i;
	long num_elements, old_num_elements;
	char *ptr;
	MX_BLUICE_FOREIGN_DEVICE *foreign_device;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	size_t foreign_pointer_size = sizeof(MX_BLUICE_FOREIGN_DEVICE *);
	size_t foreign_device_size = sizeof(MX_BLUICE_FOREIGN_DEVICE);
	long mx_status_code;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed was NULL." );
	}
	if ( name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'name' pointer passed was NULL." );
	}
	if ( foreign_device_array_ptr == (MX_BLUICE_FOREIGN_DEVICE ***) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'foreign_device_array_ptr' pointer passed was NULL." );
	}
	if ( num_foreign_devices_ptr == (long *) NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'num_foreign_devices_ptr' pointer passed was NULL." );
	}
	if ( foreign_device_ptr == (MX_BLUICE_FOREIGN_DEVICE **) NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'foreign_device_ptr' pointer passed was NULL." );
	}

#if BLUICE_DEBUG_SETUP
	MX_DEBUG(-2,("%s: name = '%s'", fname, name));
	MX_DEBUG(-2,("%s: *foreign_device_array_ptr = %p",
		fname, *foreign_device_array_ptr));
	MX_DEBUG(-2,("%s: foreign_device_ptr = %p", fname, foreign_device_ptr));
#endif
	if ( bluice_server->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_BLUICE_SERVER "
		"pointer %p is NULL.", bluice_server );
	}

	if ( (*num_foreign_devices_ptr) > 0 ) {
		if ( (*foreign_device_array_ptr) == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"'*foreign_device_array_ptr' is NULL even though "
			"'*num_foreign_devices_ptr' is %ld.",
				*num_foreign_devices_ptr );
		}
	}

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	if ( bluice_server->record->mx_type == MXN_BLUICE_DCSS_SERVER ) {
		bluice_dcss_server = bluice_server->record->record_type_struct;

		if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		  "The MX_BLUICE_DCSS_SERVER pointer for record '%s' is NULL.",
		  		bluice_server->record->name );
		}

		if ( bluice_dcss_server->is_authenticated == FALSE ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"Cannot access Blu-Ice device '%s' since "
			"we have not successfully authenticated with the "
			"Blu-Ice server at '%s', port %ld.", name,
				bluice_dcss_server->hostname,
				bluice_dcss_server->port_number );
		}
	}

	*foreign_device_ptr = NULL;

	for ( i = 0; i < *num_foreign_devices_ptr; i++ ) {
		foreign_device = (*foreign_device_array_ptr)[i];

#if BLUICE_DEBUG_SETUP
		MX_DEBUG(-2,("%s: (*foreign_device_array_ptr)[%d] = %p",
			fname, i, foreign_device));
#endif

		if ( foreign_device == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
			continue;	/* Skip to the next entry. */
		} else {
#if BLUICE_DEBUG_SETUP
			MX_DEBUG(-2,
			  ("%s: name = '%s', foreign_device->name = '%s'",
				fname, name, foreign_device->name));
#endif

			if ( strcmp( foreign_device->name, name ) == 0 ) {
#if BLUICE_DEBUG_SETUP
				MX_DEBUG(-2,("%s: exiting loop at i = %d",
					fname, i));
#endif

				break;	/* Exit the for() loop. */
			}
#if BLUICE_DEBUG_SETUP
			MX_DEBUG(-2,("%s: name did not match.", fname));
#endif
		}
	}

#if BLUICE_DEBUG_SETUP
	MX_DEBUG(-2,("%s: i = %d", fname, i));
#endif

	/* If a preexisting device has been found, return now. */

	if ( i < *num_foreign_devices_ptr ) {
		*foreign_device_ptr = (*foreign_device_array_ptr)[i];

#if BLUICE_DEBUG_SETUP
		MX_DEBUG(-2,("%s: #1 *foreign_device_ptr = %p",
				fname, *foreign_device_ptr));
#endif

		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return MX_SUCCESSFUL_RESULT;
	}

	/* NOTE: If either foreign_pointer_size or foreign_device_size 
	 *       are set to 0, this means that we are only searching for
	 *       existing device entries.  If we have not found one by
	 *       the time we get to this point in the function, we return
	 *       an MXE_NOT_FOUND error.
	 */

	if ( make_new_pointer_if_necessary == FALSE ) {

		*foreign_device_ptr = NULL;

		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
			"Blu-Ice device '%s' was not found.", name );
	}

#if BLUICE_DEBUG_SETUP
	MX_DEBUG(-2,("%s: MARKER 1 -> *foreign_device_array_ptr = %p",
		fname, *foreign_device_array_ptr));
#endif

	/* Otherwise, make sure the array is big enough for the new pointer. */

	if ( i >= *num_foreign_devices_ptr ) {
		old_num_elements = 0;
		num_elements = 0;

		if ( *num_foreign_devices_ptr == 0 ) {
#if BLUICE_DEBUG_SETUP
			MX_DEBUG(-2,("%s: Allocating new array.", fname));
#endif

			old_num_elements = 0;

			num_elements = MX_BLUICE_ARRAY_BLOCK_SIZE;

			*foreign_device_array_ptr
			    = (MX_BLUICE_FOREIGN_DEVICE **)
				calloc( num_elements, foreign_pointer_size );
		} else
		if (((*num_foreign_devices_ptr)
				% MX_BLUICE_ARRAY_BLOCK_SIZE) == 0)
		{
#if BLUICE_DEBUG_SETUP
			MX_DEBUG(-2,("%s: Expanding old array.", fname));
#endif

			old_num_elements = *num_foreign_devices_ptr;

			num_elements =
				old_num_elements + MX_BLUICE_ARRAY_BLOCK_SIZE;

			*foreign_device_array_ptr
			    = (MX_BLUICE_FOREIGN_DEVICE **)
				    realloc( (*foreign_device_array_ptr),
		    			num_elements * foreign_pointer_size );
		}

#if BLUICE_DEBUG_SETUP
		{
			int j;

			MX_DEBUG(-2,("%s: num_elements = %ld",
				fname, num_elements));

			MX_DEBUG(-2,("%s: foreign_device_array_ptr = %p",
				fname, foreign_device_array_ptr));

			MX_DEBUG(-2,("%s: *foreign_device_array_ptr = %p",
				fname, *foreign_device_array_ptr));

			for ( j = 0; j < 4; j++ ) {
			    MX_DEBUG(-2,
			    	("%s: (*foreign_device_array_ptr)[%d] = %p",
			    	fname, j, (*foreign_device_array_ptr)[j]));
			}
		}
#endif

		if ( old_num_elements == 0 ) {
			memset( *foreign_device_array_ptr, 0,
				foreign_pointer_size * num_elements );
		} else
		if ( num_elements != old_num_elements ) {
			ptr = (char *) (*foreign_device_array_ptr) +
				    foreign_pointer_size * old_num_elements;

			memset( ptr, 0,
		    foreign_pointer_size * (num_elements - old_num_elements) );
		}

		if ( (num_elements > 0) && 
	    ((*foreign_device_array_ptr) == (MX_BLUICE_FOREIGN_DEVICE **) NULL))
		{
			mx_mutex_unlock( bluice_server->foreign_data_mutex );

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory allocating an %ld element array "
			"of Blu-Ice foreign devices.", num_elements );
		}
	}

#if BLUICE_DEBUG_SETUP
	MX_DEBUG(-2,("%s: MARKER 2 -> *foreign_device_array_ptr = %p",
		fname, *foreign_device_array_ptr));
#endif

	/* Add the new entry to the array. */

	*foreign_device_ptr =
		(MX_BLUICE_FOREIGN_DEVICE *) malloc( foreign_device_size );

	if ( (*foreign_device_ptr) == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate a Blu-Ice foreign device of %lu bytes.",
			(unsigned long) foreign_device_size );
	}

#if BLUICE_DEBUG_SETUP
	MX_DEBUG(-2,("%s: new *foreign_device_ptr = %p",
		fname, *foreign_device_ptr));
#endif

	memset( *foreign_device_ptr, 0, foreign_device_size );

	foreign_device = *foreign_device_ptr;

	strlcpy( foreign_device->name, name, MXU_BLUICE_NAME_LENGTH );

	(*foreign_device_array_ptr)[*num_foreign_devices_ptr]
					= *foreign_device_ptr;

	(*num_foreign_devices_ptr)++;

#if BLUICE_DEBUG_SETUP
	MX_DEBUG(-2,("%s: #2 *foreign_device_ptr = %p",
				fname, *foreign_device_ptr));
#endif

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_bluice_setup_device_pointer( MX_BLUICE_SERVER *bluice_server,
			char *name,
			MX_BLUICE_FOREIGN_DEVICE ***foreign_device_array_ptr,
			long *num_foreign_devices_ptr,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_device_pointer_fn( bluice_server,
			name, foreign_device_array_ptr, num_foreign_devices_ptr,
			TRUE, foreign_device_ptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_bluice_get_device_pointer( MX_BLUICE_SERVER *bluice_server,
			char *name,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_array,
			long num_foreign_devices,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr )
{
	mx_status_type mx_status;

	mx_status = mx_bluice_device_pointer_fn( bluice_server,
			name, &foreign_device_array, &num_foreign_devices,
			FALSE, foreign_device_ptr );

	return mx_status;
}

/*------------*/

MX_EXPORT mx_status_type
mx_bluice_wait_for_device_pointer_initialization(
			MX_BLUICE_SERVER *bluice_server,
			char *name,
			long bluice_foreign_type,
			MX_BLUICE_FOREIGN_DEVICE ***foreign_device_array_ptr,
			long *num_foreign_devices_ptr,
			MX_BLUICE_FOREIGN_DEVICE **foreign_device_ptr,
			double timeout_in_seconds )
{
	static const char fname[] =
			"mx_bluice_wait_for_device_pointer_initialization()";

	unsigned long i, wait_ms, max_attempts;
	mx_status_type mx_status;

	/* Wait for the Blu-Ice server thread to assign a value to the pointer.
	 */
	
	wait_ms = 100;

	max_attempts = 
		mx_round( (1000.0 * timeout_in_seconds) / (double) wait_ms );

	for ( i = 0; i < max_attempts; i++ ) {

		mx_status = mx_bluice_device_pointer_fn( bluice_server,
						name,
						foreign_device_array_ptr,
						num_foreign_devices_ptr,
						FALSE,
						foreign_device_ptr );

		if ( mx_status.code == MXE_NOT_FOUND ) {
			mx_msleep( wait_ms );
		} else {
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( (*foreign_device_ptr) == NULL ) {
				mx_msleep( wait_ms );
			} else {
				break;		/* Exit the for() loop. */
			}
		}
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for Blu-Ice server '%s' "
		"to initialize Blu-Ice device '%s'.", timeout_in_seconds,
					bluice_server->record->name, name );
	}

	if ( bluice_foreign_type != (*foreign_device_ptr)->foreign_type ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The type (%ld) of Blu-Ice server device '%s' does not "
		"match the expected type of %ld.  Perhaps you have specified "
		"an incorrect device name?",
			(*foreign_device_ptr)->foreign_type,
			name, bluice_foreign_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_is_master( MX_BLUICE_SERVER *bluice_server,
				mx_bool_type *master_flag )
{
	static const char fname[] = "mx_bluice_is_master()";

	MX_RECORD *bluice_server_record;
	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	long mx_status_code;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed is NULL." );
	}
	if ( master_flag == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'master_flag' argument passed is NULL." );
	}

	bluice_server_record = bluice_server->record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_BLUICE_SERVER pointer %p "
		"that was passed is NULL.", bluice_server );
	}
	
	/* DHS server connections effectively are always master. */

	if ( bluice_server_record->mx_type == MXN_BLUICE_DHS_SERVER ) {
		*master_flag = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* We have to check for DCSS servers. */

	bluice_dcss_server = (MX_BLUICE_DCSS_SERVER *)
				bluice_server_record->record_type_struct;

	if ( bluice_dcss_server == (MX_BLUICE_DCSS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DCSS_SERVER pointer for Blu-Ice server '%s' "
		"is NULL.", bluice_server_record->name );
	}

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	if ( bluice_dcss_server->is_master ) {
		*master_flag = TRUE;
	} else {
		*master_flag = FALSE;
	}

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

MX_API mx_status_type
mx_bluice_take_master( MX_BLUICE_SERVER *bluice_server,
			mx_bool_type take_master )
{
	static const char fname[] = "mx_bluice_take_master()";

	MX_RECORD *bluice_server_record;
	mx_status_type mx_status;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed is NULL." );
	}

	bluice_server_record = bluice_server->record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_BLUICE_SERVER pointer %p "
		"that was passed is NULL.", bluice_server );
	}

	if ( bluice_server_record->mx_type == MXN_BLUICE_DHS_SERVER ) {

		/* DHS server connections are effectively always masters
		 * so we need not do anything here.
		 */

		 return MX_SUCCESSFUL_RESULT;
	}
	
	if ( take_master ) {
		mx_status = mx_bluice_send_message( bluice_server->record,
					"gtos_become_master force", NULL, 0 );
	} else {
		mx_status = mx_bluice_send_message( bluice_server->record,
					"gtos_become_slave", NULL, 0 );
	}

	return mx_status;
}

MX_API mx_status_type
mx_bluice_check_for_master( MX_BLUICE_SERVER *bluice_server )
{
	static const char fname[] = "mx_bluice_check_for_master()";

	MX_BLUICE_DCSS_SERVER *bluice_dcss_server;
	mx_bool_type master_flag;
	unsigned long i, wait_ms, max_attempts, auto_take_master;
	mx_status_type mx_status;

	mx_status = mx_bluice_is_master( bluice_server, &master_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( master_flag ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		/* We can only get here for DCSS servers. */

		bluice_dcss_server = (MX_BLUICE_DCSS_SERVER *)
				bluice_server->record->record_type_struct;

		auto_take_master = bluice_dcss_server->bluice_dcss_flags
					& MXF_BLUICE_DCSS_AUTO_TAKE_MASTER;

		if ( auto_take_master ) {
			mx_status = mx_bluice_take_master(bluice_server, TRUE);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Wait to become master. */

			wait_ms = 100;
			max_attempts = 50;

			for ( i = 0; i < max_attempts; i++ ) {
				mx_status = mx_bluice_is_master( bluice_server,
								&master_flag );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				if ( master_flag ) {
					return MX_SUCCESSFUL_RESULT;
				}

				mx_msleep( wait_ms );
			}

			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds to become master "
			"for Blu-Ice server '%s'.",
				0.001 * (double)( wait_ms * max_attempts ),
				bluice_server->record->name );
		} else {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"Your client is not currently master for "
			"Blu-Ice server '%s'.",
				bluice_server->record->name );
		}
	}
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_update_motion_status( MX_BLUICE_SERVER *bluice_server,
				char *status_message,
				mx_bool_type move_in_progress )
{
	static const char fname[] = "mx_bluice_update_motion_status()";

	MX_BLUICE_FOREIGN_DEVICE *foreign_motor;
	char *ptr, *token_ptr, *motor_name;
	double motor_position;
	mx_bool_type start_of_move;
	mx_status_type mx_status;
	long mx_status_code;

#if BLUICE_DEBUG_MOTION
	MX_DEBUG(-2,("%s invoked for message '%s' from server '%s'",
		fname, bluice_server->receive_buffer,
		bluice_server->record->name ));
#endif

	/* Is this the start of a move? */

	ptr = bluice_server->receive_buffer;

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The message '%s' received from Blu-Ice server '%s' "
		"contained only space characters.", 
	    		bluice_server->receive_buffer,
			bluice_server->record->name );
	}

	if ( ( strcmp(token_ptr, "stog_motor_move_started") == 0 )
	  || ( strcmp(token_ptr, "htos_motor_move_started") == 0 ) )
	{
		start_of_move = TRUE;
	} else {
		start_of_move = FALSE;
	}

	/* Get the motor name. */

	motor_name = mx_string_token( &ptr, " " );

	if ( motor_name == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
	"Motor name not found in message received from Blu-Ice server '%s'.",
	    		bluice_server->record->name );
	}

	/* Get the motor position. */

	token_ptr = mx_string_token( &ptr, " " );

	if ( token_ptr == NULL ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not find the motor position in a message received "
		"from Blu-Ice server '%s'.", bluice_server->record->name );
	}

	motor_position = atof( token_ptr );

#if BLUICE_DEBUG_MOTION
	{
		char *status_ptr;

		/* Was there any trailing status information? */

		status_ptr = mx_string_token( &ptr, "{}" );

		if ( status_ptr == NULL ) {
			MX_DEBUG(-2,
		("%s: motor '%s', move_in_progress = %d, position = %g",
			fname, motor_name, move_in_progress, motor_position));
		} else {
			MX_DEBUG(-2,
	 ("%s: motor '%s', move_in_progress = %d, position = %g, status = '%s'",
	      fname, motor_name, move_in_progress, motor_position, status_ptr));
		}
	}
#endif

	/* Update the values in the motor structures. */

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	mx_status = mx_bluice_get_device_pointer( bluice_server,
						motor_name,
						bluice_server->motor_array,
						bluice_server->num_motors,
						&foreign_motor );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		if ( mx_status.code == MXE_NOT_FOUND ) {

			/* mx_bluice_get_device_pointer() suppresses the
			 * display of MXE_NOT_FOUND errors, but we want
			 * the user to see this particular error.
			 */

			return mx_error( mx_status.code,
				mx_status.location, mx_status.message );
		} else {
			return mx_status;
		}
	}

	if ( foreign_motor == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		mx_mutex_unlock( bluice_server->foreign_data_mutex );

		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MX_BLUICE_FOREIGN_DEVICE pointer for DCSS motor '%s' "
		"has not been initialized.", motor_name );
	}

	/* Update the motion status. */

	if ( start_of_move == FALSE ) {
		foreign_motor->u.motor.position = motor_position;
	}

	foreign_motor->u.motor.move_in_progress = move_in_progress;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return MX_SUCCESSFUL_RESULT;
}

