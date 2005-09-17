/*
 * Name:    d_sony_visca_ptz.c
 *
 * Purpose: MX driver for Sony Pan/Tilt/Zoom camera supports that use the
 *          VISCA protocol for communication.
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

#define MXD_SONY_VISCA_PTZ_DEBUG	TRUE

#include <stdio.h>

#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_ptz.h"
#include "d_sony_visca_ptz.h"

MX_RECORD_FUNCTION_LIST mxd_sony_visca_ptz_record_function_list = {
	NULL,
	mxd_sony_visca_ptz_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_sony_visca_ptz_open
};

MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_sony_visca_ptz_ptz_function_list = {
	mxd_sony_visca_ptz_command,
	mxd_sony_visca_ptz_status
};

MX_RECORD_FIELD_DEFAULTS mxd_sony_visca_ptz_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PAN_TILT_ZOOM_STANDARD_FIELDS,
	MXD_SONY_VISCA_PTZ_STANDARD_FIELDS
};

long mxd_sony_visca_ptz_num_record_fields
	= sizeof( mxd_sony_visca_ptz_rf_defaults )
		/ sizeof( mxd_sony_visca_ptz_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sony_visca_ptz_rfield_def_ptr
			= &mxd_sony_visca_ptz_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_sony_visca_ptz_get_pointers( MX_PAN_TILT_ZOOM *ptz,
			MX_SONY_VISCA_PTZ **sony_visca_ptz,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sony_visca_ptz_get_pointers()";

	if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PAN_TILT_ZOOM pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sony_visca_ptz == (MX_SONY_VISCA_PTZ **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SONY_VISCA_PTZ pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*sony_visca_ptz = (MX_SONY_VISCA_PTZ *) ptz->record->record_type_struct;

	if ( *sony_visca_ptz == (MX_SONY_VISCA_PTZ *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SONY_VISCA_PTZ pointer for record '%s' is NULL.",
			ptz->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_sony_visca_ptz_create_record_structures()";

        MX_PAN_TILT_ZOOM *ptz;
        MX_SONY_VISCA_PTZ *sony_visca_ptz;

        /* Allocate memory for the necessary structures. */

        ptz = (MX_PAN_TILT_ZOOM *) malloc(sizeof(MX_PAN_TILT_ZOOM));

        if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Unable to allocate memory for an MX_PAN_TILT_ZOOM structure." );
        }

        sony_visca_ptz = (MX_SONY_VISCA_PTZ *)
				malloc( sizeof(MX_SONY_VISCA_PTZ) );

        if ( sony_visca_ptz == (MX_SONY_VISCA_PTZ *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_SONY_VISCA_PTZ structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ptz;
        record->record_type_struct = sony_visca_ptz;
        record->class_specific_function_list =
				&mxd_sony_visca_ptz_ptz_function_list;

        ptz->record = record;
	sony_visca_ptz->record = record;

        return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sony_visca_copy( char *destination, char *source, size_t max_length )
{
	static const char fname[] = "mxd_sony_visca_copy()";

	size_t i;

	if ( destination == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The destination pointer passed was NULL." );
	}
	if ( source == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The source pointer passed was NULL." );
	}

	for ( i = 0; i < max_length; i++ ) {
		destination[i] = source[i];

		/* Have we found the message terminator 0xff yet? */

		if ( ((unsigned char) destination[i]) == 0xff ) {
			break;			/* Exit the for() loop. */
		}
	}

	if ( i >= max_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The source buffer was too large to fit into the "
		"destination buffer of length %lu.",
			(unsigned long) max_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sony_visca_handle_error( MX_SONY_VISCA_PTZ *sony_visca_ptz,
				int error_type,
				char *sent_visca_ascii )
{
	static const char fname[] = "mxd_sony_visca_handle_error()";

	unsigned char c;
	mx_status_type mx_status;

	/* If we received an error code, there should be a 0xff byte
	 * message terminator after it.  Read that terminator and
	 * throw it away.
	 */

	mx_status = mx_rs232_getchar( sony_visca_ptz->rs232_record,
					&c, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != 0xff ) {
		mx_warning( "The error message received from Sony PTZ '%s' "
		"was not terminated with a 0xff byte.  Instead, it was "
		"terminated with a %#x byte.",
			sony_visca_ptz->record->name, c );
	}

	switch( error_type ) {
	case 0x01:
		return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
		"The message '%s' sent to Sony PTZ '%s' was longer "
		"than the maximum length of 14 bytes.",
			sent_visca_ascii,
			sony_visca_ptz->record->name );
		break;
	case 0x02:
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The message '%s' sent to Sony PTZ '%s' had "
		"a syntax error in it.",
			sent_visca_ascii,
			sony_visca_ptz->record->name );
		break;
	case 0x03:
		return mx_error( MXE_TRY_AGAIN, fname,
		"The command buffer for Sony PTZ '%s' is full.",
			sony_visca_ptz->record->name );
		break;
	case 0x04:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"The command '%s' for Sony PTZ '%s' was cancelled.",
			sent_visca_ascii,
			sony_visca_ptz->record->name );
		break;
	case 0x05:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"No socket for command '%s' (to be canceled) for "
		"Sony PTZ '%s'.",
			sent_visca_ascii,
			sony_visca_ptz->record->name );
		break;
	case 0x41:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"Command '%s' for Sony PTZ '%s' is not executable.",
			sent_visca_ascii,
			sony_visca_ptz->record->name );
		break;
	default:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"An unrecognized error code %#x was received "
		"for command '%s' sent to Sony PTZ '%s'.",
			error_type,
			sent_visca_ascii,
			sony_visca_ptz->record->name );
		break;
	}

	/* We should never get here. */
}

#define mxd_sony_visca_cmd( s, c, r, m, a ) \
	  mxd_sony_visca_cmd_raw( (s), (s)->camera_number, (c), (r), (m), (a) )

#define mxd_sony_visca_cmd_broadcast( s, c, r, m, a ) \
	  mxd_sony_visca_cmd_raw( (s), 8, (c), (r), (m), (a) )

static mx_status_type
mxd_sony_visca_cmd_raw( MX_SONY_VISCA_PTZ *sony_visca_ptz,
			int camera_number,
			char *command,
			char *response,
			size_t max_response_length,
			size_t *actual_response_length )
{
	static const char fname[] = "mxd_sony_visca_cmd()";

	char local_receive_buffer[80], hex_buffer[10];
	char sent_visca_ascii[ 5 * MXU_SONY_VISCA_PTZ_MAX_CMD_LENGTH + 1];
	char received_visca_ascii[ 5 * MXU_SONY_VISCA_PTZ_MAX_CMD_LENGTH + 1];
	unsigned char c, response_type, third_byte;
	size_t num_response_bytes, bytes_read;
	int num_command_body_bytes, more_message_to_read, exit_loop;
	int encoded_camera_number, received_camera_number;
	unsigned long i, num_bytes_available, wait_ms, max_attempts;
	mx_status_type mx_status;

	/* How many bytes are in the command body? */

	num_command_body_bytes = 0;

	while (1) {
		num_command_body_bytes++;

		if ( *command == '\xff' ) {
			break;			/* Exit the while() loop. */
		}
	}

	/* Compute the encoded camera number. */

	encoded_camera_number = 0x80 + camera_number;

	/* If requested, display the command to the user. */

	snprintf( sent_visca_ascii, sizeof(sent_visca_ascii),
			"%#x ", encoded_camera_number );

	for ( i = 0; i < num_command_body_bytes; i++ ) {
		snprintf( hex_buffer, sizeof(hex_buffer), "%#x ", command[i] );

		strlcat(sent_visca_ascii, hex_buffer, sizeof(sent_visca_ascii));
	}

#if MXD_SONY_VISCA_PTZ_DEBUG
	MX_DEBUG(-2,("%s: sending %s to Sony Pan/Tilt/Zoom controller '%s'.",
		fname, sent_visca_ascii, sony_visca_ptz->record->name ));
#endif

	/* Send the encoded camera number. */

	mx_status = mx_rs232_putchar( sony_visca_ptz->rs232_record,
					encoded_camera_number, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command body. */

	mx_status = mx_rs232_write( sony_visca_ptz->rs232_record,
					command, num_command_body_bytes,
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the first three characters of the response and
	 * decide what to do with the message.
	 */

	while (1) {

		/* Wait for the first character in the response. */

		wait_ms = 100;
		max_attempts = 50;

		for ( i = 0; i < max_attempts; i++ ) {
			mx_status = mx_rs232_num_input_bytes_available(
					sony_visca_ptz->rs232_record,
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
		    "Timed out after waiting %g seconds for a response from "
		    "Sony Pan/Tilt/Zoom controller '%s' to the command %s.",
				1000.0 * (double)( wait_ms * max_attempts ),
				sony_visca_ptz->record->name,
				sent_visca_ascii );
		}

		/* Read in the first three characters. */

		mx_status = mx_rs232_read( sony_visca_ptz->rs232_record,
						local_receive_buffer,
						3, &bytes_read, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( bytes_read != 3 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Read only %ld bytes from Sony PTZ '%s' rather "
			"than the 3 bytes we expected.",
				(unsigned long) bytes_read,
				sony_visca_ptz->record->name );
		}

		/* Does the returned camera number match the number
		 * that we sent?
		 */

		received_camera_number = 
			( local_receive_buffer[0] & 0xf0 ) >> 8;

		if ( (camera_number != 8) 
		  && (received_camera_number != camera_number) )
		{
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"The camera number %d in the message received from "
			"RS-232 port '%s' does not match the expected "
			"camera number %d.",
				received_camera_number,
				sony_visca_ptz->record->name,
				sony_visca_ptz->camera_number );
		}

		/* Figure out what kind of response this is. */

		response_type = ( local_receive_buffer[1] & 0xf0 ) >> 8;

		third_byte = local_receive_buffer[2];

		more_message_to_read = TRUE;
		exit_loop = FALSE;

		switch ( response_type ) {
		case 4:
			/* This should be an ACK message.  If so,
			 * it should be followed by 0xff.
			 */

			 if ( third_byte != 0xff ) {
			 	return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"The ACK byte %#x was was followed by "
				"the byte %#x which should have been "
				"0xff instead.", local_receive_buffer[1],
					third_byte );
			}

			more_message_to_read = FALSE;
			exit_loop = TRUE;
			break;
		case 5:
			/* If the third byte is a 0xff, then this is a 
			 * command completion message which we will ignore.
			 * Otherwise, we assume it is an inquiry response
			 * that we will need to return to the caller.
			 */

			if ( third_byte == 0xff ) {
			 	/* This is a command completion message
				 * which we will skip over.  This means
				 * that we need to go back to the top
				 * of the while() loop to look for the
				 * message we really want.
				 */

				MX_DEBUG(-2,
				("%s: command completion message seen.",
					fname ));

				more_message_to_read = FALSE;
				exit_loop = FALSE;
			} else {
				/* This is an inquiry message that we may
				 * need to return to the caller.
				 */

				more_message_to_read = TRUE;
				exit_loop = TRUE;
			}
			break;
		case 6:
			/* This is an error message. */

			mx_status = mxd_sony_visca_handle_error( sony_visca_ptz,
						third_byte, sent_visca_ascii );

			return mx_status;
			break;
		default:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unknown response type %d seen from Sony PTZ '%s'.",
				response_type, sony_visca_ptz->record->name );
			break;
		}

		if ( exit_loop ) {
			/* Delete the camera number by moving each byte 
			 * one array element to the left.
			 */

			 local_receive_buffer[0] = local_receive_buffer[1];
			 local_receive_buffer[1] = local_receive_buffer[2];
			 local_receive_buffer[2] = 0;

			 snprintf( received_visca_ascii,
			 	sizeof(received_visca_ascii),
				"%#x %#x ", local_receive_buffer[0],
					local_receive_buffer[1] );

			 break;		/* Exit the while loop. */
		}
	}

	/* Read in the rest of response. */

	if ( more_message_to_read == FALSE ) {
		num_response_bytes = 3;
	} else {
		*received_visca_ascii = '\0';

		for ( i = 2; i < sizeof(local_receive_buffer); i++ ) {
			mx_status = mx_rs232_getchar(
						sony_visca_ptz->rs232_record,
						&c, MXF_232_WAIT );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			local_receive_buffer[i] = c;

			snprintf( hex_buffer, sizeof(hex_buffer), "%#x ", c );

			strlcat( received_visca_ascii, hex_buffer,
				sizeof( received_visca_ascii ) );

			if ( ((unsigned char) c ) == 0xff ) {
				break;		/* Exit the while() loop. */
			}
		}

		if ( i >= sizeof(local_receive_buffer) ) {
			(void) mx_rs232_discard_unread_input(
				sony_visca_ptz->rs232_record, 0 );

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The response from Sony PTZ '%s' was too long to fit "
			"into the local receive buffer.  Response = '%s'.",
				sony_visca_ptz->record->name,
				received_visca_ascii );
		}
		num_response_bytes = i+1;
	}

#if MXD_SONY_VISCA_PTZ_DEBUG
	MX_DEBUG(-2,("%s: received %s from Sony Pan/Tilt/Zoom controller '%s'.",
		fname, received_visca_ascii, sony_visca_ptz->record->name ));
#endif

	if ( response != NULL ) {
		if ( num_response_bytes > max_response_length ) {
			mx_warning( "The number of bytes %lu received from "
			"Sony PTZ '%s' was greater than the size %lu of the "
			"buffer supplied by the caller.  The message returned "
			"will be truncated.",
				(unsigned long) num_response_bytes,
				sony_visca_ptz->record->name,
				(unsigned long) max_response_length );

			num_response_bytes = max_response_length;
		}

		/* Copy the message to the response buffer. */

		memcpy( response, local_receive_buffer, num_response_bytes );
	}

	if ( actual_response_length != NULL ) {
		*actual_response_length = num_response_bytes;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sony_visca_ptz_open()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_SONY_VISCA_PTZ *sony_visca_ptz;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	ptz = (MX_PAN_TILT_ZOOM *) record->record_class_struct;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
						&sony_visca_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SONY_VISCA_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	/* Only send the initialization commands if this is the first
	 * camera on the daisy chain.
	 */

	if ( sony_visca_ptz->camera_number == 1 ) {

		/* VISCA wants the DTR output of the computer to be high. */

		mx_status = mx_rs232_set_signal_bit(
					sony_visca_ptz->rs232_record,
					MXF_232_DATA_TERMINAL_READY, 1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_discard_unread_input(
					sony_visca_ptz->rs232_record, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Send the address set command. */

		mx_status = mxd_sony_visca_cmd_broadcast( sony_visca_ptz,
						"\x30\x01\xff",
						NULL, 0, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Send the interface clear command. */

		mx_status = mxd_sony_visca_cmd_broadcast( sony_visca_ptz,
						"\x01\x00\x01\xff",
						NULL, 0, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Clear the input buffer in case we get more than
		 * one response.
		 */

		mx_status = mx_rs232_discard_unread_input(
					sony_visca_ptz->rs232_record, 0 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_command( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_sony_visca_ptz_command()";

	MX_SONY_VISCA_PTZ *sony_visca_ptz;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
						&sony_visca_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ptz->command ) {
	case MXF_PTZ_DRIVE_UP:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x03\x01\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_DOWN:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x03\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_LEFT:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x01\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_RIGHT:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x02\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_UPPER_LEFT:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x01\x01\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_UPPER_RIGHT:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x02\x01\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_LOWER_LEFT:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x01\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_LOWER_RIGHT:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x02\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_STOP:
		mxd_sony_visca_copy( command,
				"\x01\x06\x01\x03\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_DRIVE_HOME:
		mxd_sony_visca_copy( command,
				"\x01\x06\x04\xff", sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_OUT:
		mxd_sony_visca_copy( command,
				"\x01\x04\x07\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_IN:
		mxd_sony_visca_copy( command,
				"\x01\x04\x07\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_ZOOM_STOP:
		mxd_sony_visca_copy( command,
				"\x01\x04\x07\x00\xff", sizeof( command ) );
		break;
	case MXF_PTZ_FOCUS_MANUAL:
		mxd_sony_visca_copy( command,
				"\x01\x04\x38\x03\xff", sizeof( command ) );
		break;
	case MXF_PTZ_FOCUS_AUTO:
		mxd_sony_visca_copy( command,
				"\x01\x04\x38\x02\xff", sizeof( command ) );
		break;
	case MXF_PTZ_FOCUS_STOP:
		mxd_sony_visca_copy( command,
				"\x01\x04\x08\x00\xff", sizeof( command ) );
		break;
	
		/* For the next two cases, we have to switch to manual mode
		 * before sending the command.
		 */

	case MXF_PTZ_FOCUS_FAR:
		ptz->command = MXF_PTZ_FOCUS_MANUAL;

		mx_status = mxd_sony_visca_ptz_command( ptz );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now send the focus far command. */

		mxd_sony_visca_copy( command,
				"\x01\x04\x08\x02\xff", sizeof( command ));
		break;

	case MXF_PTZ_FOCUS_NEAR:
		ptz->command = MXF_PTZ_FOCUS_MANUAL;

		mx_status = mxd_sony_visca_ptz_command( ptz );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now send the focus far command. */

		mxd_sony_visca_copy( command,
				"\x01\x04\x08\x03\xff", sizeof( command ));
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The command %lu received for Sony PTZ '%s' "
			"is not a known command type.",
				ptz->command, sony_visca_ptz->record->name );
		break;
	}

	/* Send the command. */

	mx_status = mxd_sony_visca_cmd( sony_visca_ptz, command,
						NULL, 0, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sony_visca_ptz_status( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_sony_visca_ptz_status()";

	MX_SONY_VISCA_PTZ *sony_visca_ptz;
	mx_status_type mx_status;

	mx_status = mxd_sony_visca_ptz_get_pointers( ptz,
						&sony_visca_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptz->status = 0;

	return mx_status;
}

