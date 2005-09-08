/*
 * Name:    mx_bluice.c
 *
 * Purpose: Client side part of Blu-Ice network protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_mutex.h"
#include "mx_bluice.h"

#define BLUICE_DEBUG		TRUE

#define BLUICE_DEBUG_MESSAGE	TRUE

#define BLUICE_DEBUG_TIMING	TRUE

#if BLUICE_DEBUG_TIMING
#include "mx_hrt_debug.h"
#endif

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
			long binary_data_length,
			long required_data_length,
			int send_header )
{
	static const char fname[] = "mx_bluice_send_message()";

	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *bluice_server_socket;
	char message_header[MX_BLUICE_MSGHDR_LENGTH];
	char null_pad[500];
	long text_data_length, bytes_sent, null_bytes_to_send;
	mx_status_type mx_status;

	mx_status = mx_bluice_get_pointers( bluice_server_record,
				&bluice_server, &bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( required_data_length > ((long) sizeof(null_pad)) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The required data length %lu is longer than the "
		"maximum allowed length of %lu.",
			(unsigned long) required_data_length,
			(unsigned long) sizeof(null_pad) );
	}

#if BLUICE_DEBUG_MESSAGE
	MX_DEBUG(-2,("%s: sending '%s' to server '%s'.",
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

	text_data_length = strlen( text_data ) + 1;

	if ( send_header ) {
		sprintf( message_header, "%*lu%*lu",
			MX_BLUICE_MSGHDR_TEXT_LENGTH,
			(unsigned long) text_data_length,
			MX_BLUICE_MSGHDR_BINARY_LENGTH,
			(unsigned long) binary_data_length );

		/* Send the message header. */

		mx_status = mx_socket_send( bluice_server_socket,
					&message_header,
					MX_BLUICE_MSGHDR_LENGTH );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_sent += MX_BLUICE_MSGHDR_LENGTH;
	}

	/* Send the text data. */

	mx_status = mx_socket_send( bluice_server_socket,
					text_data,
					text_data_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bytes_sent += text_data_length;

	/* Send the binary data (if any). */

	if ( ( binary_data_length > 0 ) && ( binary_data != NULL ) ) {
		mx_status = mx_socket_send( bluice_server_socket,
						binary_data,
						binary_data_length );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_sent += binary_data_length;
	}

	if ( required_data_length >= 0 ) {
		if ( bytes_sent < required_data_length ) {
			null_bytes_to_send = required_data_length - bytes_sent;

			memset( null_pad, 0, null_bytes_to_send );

			mx_status = mx_socket_send( bluice_server_socket,
						null_pad,
						null_bytes_to_send );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_receive_message( MX_RECORD *bluice_server_record,
				char *data_buffer,
				long data_buffer_length,
				long *actual_data_length )
{
	static const char fname[] = "mx_bluice_receive_message()";

	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *bluice_server_socket;
	char message_header[MX_BLUICE_MSGHDR_LENGTH];
	long text_data_length, binary_data_length, total_data_length;
	long maximum_length;
	size_t actual_bytes_received;
	char *data_pointer;
	mx_status_type mx_status;

	mx_status = mx_bluice_get_pointers( bluice_server_record,
				&bluice_server, &bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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
	MX_DEBUG(-2,("%s: received '%s' from server '%s'.",
		fname, data_pointer, bluice_server_record->name));
#endif

	*actual_data_length = actual_bytes_received;

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_num_unread_bytes_available( MX_RECORD *bluice_server_record,
					int *num_bytes_available )
{
	static const char fname[] = "mx_bluice_num_unread_bytes_available()";

	MX_BLUICE_SERVER *bluice_server;
	MX_SOCKET *bluice_server_socket;
	mx_status_type mx_status;

	mx_status = mx_bluice_get_pointers( bluice_server_record,
				&bluice_server, &bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available argument passed was NULL." );
	}

	/* Check the server socket to see if any bytes are available. */

	if ( mx_socket_is_open( bluice_server_socket ) == FALSE ) {
		return mx_error( MXE_NETWORK_CONNECTION_LOST, fname,
			"Blu-Ice server'%s' is not connected.",
	    		bluice_server_record->name );
	}

	mx_status = mx_socket_num_input_bytes_available( bluice_server->socket,
							num_bytes_available );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_bluice_discard_unread_bytes( MX_RECORD *bluice_server_record )
{
	static const char fname[] = "mx_bluice_discard_unread_bytes()";

	MX_SOCKET *bluice_server_socket;
	mx_status_type mx_status;

	mx_status = mx_bluice_get_pointers( bluice_server_record, NULL,
					&bluice_server_socket, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_discard_unread_input( bluice_server_socket );

	return mx_status;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_get_message_type( char *message_string, int *message_type )
{
	static const char fname[] = "mx_bluice_get_message_type()";

	if ( message_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'message_string' pointer passed was NULL." );
	}
	if ( message_type == (int *) NULL ) {
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

/* We do not have polymorphism in C, so the following is a kludge to let us
 * sort of simulate polymorphism.  That way we do not have to have a separate
 * routine for finding each kind of Blu-Ice foreign device.
 */

MX_EXPORT mx_status_type
mx_bluice_get_device_pointer( char *name,
				void **foreign_device_array,
				int *num_foreign_devices,
				size_t foreign_pointer_size,
				size_t foreign_device_size,
				void **foreign_device )
{
	static const char fname[] = "mx_bluice_get_device_pointer()";

	size_t i;
	int num_elements;
	MX_BLUICE_FOREIGN_DEVICE *foreign_device_ptr;

	if ( name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'name' pointer passed was NULL." );
	}
	if ( foreign_device_array == (void **) NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'foreign_device_array' pointer passed was NULL." );
	}
	if ( num_foreign_devices == (int *) NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'num_foreign_devices' pointer passed was NULL." );
	}
	if ( foreign_device == (void **) NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'foreign_device' pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s: name = '%s'", fname, name));

	MX_DEBUG( 2,("%s: *foreign_device_array = %p",
		fname, *foreign_device_array));

	MX_DEBUG( 2,("%s: foreign_device = %p", fname, foreign_device));

	foreign_device_ptr = NULL;

	for ( i = 0; i < *num_foreign_devices; i++ ) {
		foreign_device_ptr = foreign_device_array[i];

		MX_DEBUG( 2,("%s: foreign_device_array[%d] = %p",
			fname, i, foreign_device_ptr));

		if ( foreign_device_ptr == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
			continue;	/* Skip to the next entry. */
		} else {
			MX_DEBUG( 2,
			  ("%s: name = '%s', foreign_device_ptr->name = '%s'",
				fname, name, foreign_device_ptr->name));

			if ( strcmp( foreign_device_ptr->name, name ) == 0 ) {
				MX_DEBUG( 2,("%s: exiting loop at i = %d",
					fname, i));

				break;	/* Exit the for() loop. */
			}
			MX_DEBUG( 2,("%s: name did not match.", fname));
		}
	}

	MX_DEBUG( 2,("%s: i = %d", fname, i));

	/* If a preexisting device has been found, return now. */

	if ( i < *num_foreign_devices ) {
		*foreign_device = foreign_device_array[i];

		MX_DEBUG( 2,("%s: #1 *foreign_device = %p",
				fname, *foreign_device));

		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, make sure the array is big enough for the new pointer. */

	if ( i >= *num_foreign_devices ) {
		num_elements = 0;

		if ( *num_foreign_devices == 0 ) {
			MX_DEBUG( 2,("%s: Allocating new array.", fname));

			num_elements = MX_BLUICE_ARRAY_BLOCK_SIZE;

			*foreign_device_array = (MX_BLUICE_FOREIGN_DEVICE *)
				malloc( num_elements * foreign_pointer_size );
		} else
		if (((*num_foreign_devices) % MX_BLUICE_ARRAY_BLOCK_SIZE) == 0)
		{
			MX_DEBUG( 2,("%s: Expanding old array.", fname));

			num_elements =
			  (*num_foreign_devices) + MX_BLUICE_ARRAY_BLOCK_SIZE;

			*foreign_device_array = (MX_BLUICE_FOREIGN_DEVICE *)
				realloc( (*foreign_device_array),
		    			num_elements * foreign_pointer_size );

		}

		if ( (num_elements > 0) && 
		 ((*foreign_device_array) == (MX_BLUICE_FOREIGN_DEVICE *) NULL))
		{
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory allocating an %d element array "
			"of Blu-Ice foreign devices.", num_elements );
		}
	}

	/* Add the new entry to the array. */

	*foreign_device =
		(MX_BLUICE_FOREIGN_DEVICE *) malloc( foreign_device_size );

	if ( (*foreign_device) == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate a Blu-Ice foreign device of %lu bytes.",
			(unsigned long) foreign_device_size );
	}

	foreign_device_ptr = *foreign_device;

	mx_strncpy( foreign_device_ptr->name, name, MXU_BLUICE_NAME_LENGTH );

	foreign_device_array[*num_foreign_devices] = *foreign_device;

	(*num_foreign_devices)++;

	MX_DEBUG( 2,("%s: #2 *foreign_device = %p", fname, *foreign_device));

	return MX_SUCCESSFUL_RESULT;
}

/* ====================================================================== */

MX_EXPORT mx_status_type
mx_bluice_configure_motor( MX_BLUICE_SERVER *bluice_server,
				char *config_string )
{
	static const char fname[] = "mx_bluice_configure_motor()";

	void *foreign_motor_ptr;
	MX_BLUICE_FOREIGN_MOTOR *foreign_motor;
	char format_string[40];
	char name[MXU_BLUICE_NAME_LENGTH+1];
	int message_type, num_items;
	mx_status_type mx_status;

	if ( bluice_server == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_SERVER pointer passed was NULL." );
	}
	if ( config_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'config_string' pointer passed was NULL." );
	}

	mx_status = mx_bluice_get_message_type( config_string, &message_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( format_string, "%%*s %%%ds", MXU_BLUICE_NAME_LENGTH );

	num_items = sscanf( config_string, format_string, name );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Device name not found in message '%s' from "
		"Blu-Ice server '%s'.",
			config_string, bluice_server->record->name );
	}

	/* Get a pointer to the Blu-Ice foreign motor structure. */

	mx_status = mx_bluice_get_device_pointer( name,
					(void **) &(bluice_server->motor_array),
					&(bluice_server->num_motors),
					sizeof(MX_BLUICE_FOREIGN_MOTOR *),
					sizeof(MX_BLUICE_FOREIGN_MOTOR),
					&foreign_motor_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	foreign_motor = foreign_motor_ptr;

	MX_DEBUG(-2,("%s: foreign_motor = '%s'", fname, foreign_motor->name));

	/* Now parse the rest of the configuration string. */

	switch( message_type ) {
	case MXT_BLUICE_STOG:
		if ( strncmp( config_string,
			"stog_configure_real_motor", 25 ) == 0 )
		{
			foreign_motor->is_pseudo = FALSE;

			sprintf( format_string,
"%%*s %%*s %%%ds %%%ds %%lg %%lg %%lg %%lg %%lg %%lg %%lg %%d %%d %%d %%d %%d",
				MXU_BLUICE_NAME_LENGTH,
				MXU_BLUICE_NAME_LENGTH );

			num_items = sscanf( config_string, format_string,
				foreign_motor->dhs_server_name,
				foreign_motor->dhs_name,
				&(foreign_motor->position),
				&(foreign_motor->upper_limit),
				&(foreign_motor->lower_limit),
				&(foreign_motor->scale_factor),
				&(foreign_motor->speed),
				&(foreign_motor->acceleration_time),
				&(foreign_motor->backlash),
				&(foreign_motor->upper_limit_on),
				&(foreign_motor->lower_limit_on),
				&(foreign_motor->motor_lock_on),
				&(foreign_motor->backlash_on),
				&(foreign_motor->reverse_on) );

			if ( num_items != 14 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Message '%s' from Blu-Ice server '%s' did not have the expected format.",
				config_string, bluice_server->record->name );
			}
		} else
		if ( strncmp( config_string,
			"stog_configure_pseudo_motor", 27 ) == 0 )
		{
			foreign_motor->is_pseudo = TRUE;

			foreign_motor->scale_factor = 0.0;
			foreign_motor->speed = 0.0;
			foreign_motor->acceleration_time = 0.0;
			foreign_motor->backlash = 0.0;
			foreign_motor->backlash_on = FALSE;
			foreign_motor->reverse_on = FALSE;

			sprintf( format_string,
			"%%*s %%*s %%%ds %%%ds %%lg %%lg %%lg %%d %%d %%d",
				MXU_BLUICE_NAME_LENGTH,
				MXU_BLUICE_NAME_LENGTH );

			num_items = sscanf( config_string, format_string,
				foreign_motor->dhs_server_name,
				foreign_motor->dhs_name,
				&(foreign_motor->position),
				&(foreign_motor->upper_limit),
				&(foreign_motor->lower_limit),
				&(foreign_motor->upper_limit_on),
				&(foreign_motor->lower_limit_on),
				&(foreign_motor->motor_lock_on) );

			if ( num_items != 8 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
"Message '%s' from Blu-Ice server '%s' did not have the expected format.",
				config_string, bluice_server->record->name );
			}
		} else {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Unexpected 'stog' message type in Blu-Ice "
			"message '%s' from DCSS '%s'.", config_string,
				bluice_server->record->name );
		}
		break;
	default:
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Unexpected message type in Blu-Ice "
		"message '%s' from DCSS '%s'.", config_string,
			bluice_server->record->name );
		break;
	}

	MX_DEBUG(-2,("%s: -------------------------------------------", fname));
	MX_DEBUG(-2,("%s: Foreign motor '%s':", fname, foreign_motor->name));
	MX_DEBUG(-2,("%s: is pseudo = %d", fname, foreign_motor->is_pseudo));
	MX_DEBUG(-2,("%s: DHS server = '%s'",
				fname, foreign_motor->dhs_server_name));
	MX_DEBUG(-2,("%s: DHS name = '%s'",
				fname, foreign_motor->dhs_name));
	MX_DEBUG(-2,("%s: position = %g", fname, foreign_motor->position));
	MX_DEBUG(-2,("%s: upper limit = %g",
				fname, foreign_motor->upper_limit));
	MX_DEBUG(-2,("%s: lower limit = %g",
				fname, foreign_motor->lower_limit));
	MX_DEBUG(-2,("%s: scale factor = %g",
				fname, foreign_motor->scale_factor));
	MX_DEBUG(-2,("%s: speed = %g", fname, foreign_motor->speed));
	MX_DEBUG(-2,("%s: acceleration time = %g",
				fname, foreign_motor->acceleration_time));
	MX_DEBUG(-2,("%s: backlash = %g", fname, foreign_motor->backlash));
	MX_DEBUG(-2,("%s: upper limit on = %d",
				fname, foreign_motor->upper_limit_on));
	MX_DEBUG(-2,("%s: lower limit on = %d",
				fname, foreign_motor->lower_limit_on));
	MX_DEBUG(-2,("%s: motor lock on = %d",
				fname, foreign_motor->motor_lock_on));
	MX_DEBUG(-2,("%s: backlash on = %d",
				fname, foreign_motor->backlash_on));
	MX_DEBUG(-2,("%s: reverse on = %d",
				fname, foreign_motor->reverse_on));
	MX_DEBUG(-2,("%s: -------------------------------------------", fname));

	return mx_status;
}

