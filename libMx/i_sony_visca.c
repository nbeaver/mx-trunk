/*
 * Name:    i_sony_visca.c
 *
 * Purpose: MX driver for the Sony VISCA protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SONY_VISCA_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_sony_visca.h"

MX_RECORD_FUNCTION_LIST mxi_sony_visca_record_function_list = {
	NULL,
	mxi_sony_visca_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_sony_visca_open
};

MX_RECORD_FIELD_DEFAULTS mxi_sony_visca_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_SONY_VISCA_STANDARD_FIELDS
};

mx_length_type mxi_sony_visca_num_record_fields
				= sizeof( mxi_sony_visca_rf_defaults )
				/ sizeof( mxi_sony_visca_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_sony_visca_rfield_def_ptr
			= &mxi_sony_visca_rf_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxi_sony_visca_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxi_sony_visca_create_record_structures()";

        MX_SONY_VISCA *sony_visca;

        /* Allocate memory for the necessary structures. */

        sony_visca = (MX_SONY_VISCA *) malloc( sizeof(MX_SONY_VISCA) );

        if ( sony_visca == (MX_SONY_VISCA *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_SONY_VISCA structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = NULL;
        record->record_type_struct = sony_visca;
        record->class_specific_function_list = NULL;

	sony_visca->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sony_visca_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_sony_visca_open()";

	MX_SONY_VISCA *sony_visca;
	unsigned char response[20];
	size_t num_response_bytes;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	sony_visca = (MX_SONY_VISCA *) record->record_type_struct;

	if ( sony_visca == (MX_SONY_VISCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SONY_VISCA pointer passed was NULL." );
	}

#if MXI_SONY_VISCA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	/* Send the initialization commands. */

	/* VISCA wants the DTR output of the computer to be high. */

	mx_status = mx_rs232_set_signal_bit( sony_visca->rs232_record,
					MXF_232_DATA_TERMINAL_READY, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input(sony_visca->rs232_record, 0);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the address set command. */

	mx_status = mxi_sony_visca_cmd_broadcast( sony_visca,
					(unsigned char *) "\x30\x01\xff",
					response, sizeof(response),
					&num_response_bytes );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_response_bytes != 4 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not receive the expected 4 byte response for the "
		"Sony VISCA 'address set' command sent to Sony VISCA "
		"interface '%s'.  Instead saw %lu bytes.",
			sony_visca->record->name,
			(unsigned long) num_response_bytes );
	}

	if ( ( response[0] != 0x88 ) || ( response[1] != 0x30 ) ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not receive the bytes 0x88 and 0x30 at the "
			"start of the response to the Sony VISCA 'address set' "
			"command sent to Sony VISCA interface '%s'.  "
			"Instead saw %#x %#x.", sony_visca->record->name,
				response[0], response[1] );
	}

	sony_visca->num_cameras = response[2] - 1;

	if ( sony_visca->num_cameras > 7 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Sony VISCA interface '%s' reported that it has "
			"%d cameras which is more than the maximum of 7.",
				sony_visca->record->name,
				sony_visca->num_cameras );
	}

#if MXI_SONY_VISCA_DEBUG
	MX_DEBUG(-2,("%s: Sony VISCA interface '%s', %d cameras detected.",
		fname, sony_visca->record->name, sony_visca->num_cameras ));
#endif

	/* Send the interface clear command. */

	mx_status = mxi_sony_visca_cmd_broadcast( sony_visca,
					(unsigned char *) "\x01\x00\x01\xff",
					NULL, 0, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the input buffer in case we get more than one response. */

	mx_msleep(100);

	mx_status = mx_rs232_discard_unread_input( sony_visca->rs232_record,
							MXI_SONY_VISCA_DEBUG );

	return mx_status;
}

/* ----- */

MX_EXPORT mx_status_type
mxi_sony_visca_copy( unsigned char *destination,
			unsigned char *source,
			size_t max_length )
{
	static const char fname[] = "mxi_sony_visca_copy()";

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

		if ( ( destination[i]) == 0xff ) {
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

MX_EXPORT mx_status_type
mxi_sony_visca_value_string( unsigned char *destination,
			size_t max_destination_length,
			size_t prefix_length,
			unsigned char *prefix,
			unsigned long value )
{
	static const char fname[] = "mxi_sony_visca_value_string()";

	unsigned char *ptr;
	size_t i;

	if ( destination == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The destination pointer passed was NULL." );
	}
	if ( prefix == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The prefix pointer passed was NULL." );
	}

	if ( prefix_length + 6 > max_destination_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested command would be %lu bytes long, but the "
		"supplied destination buffer is only %lu bytes long.",
			(unsigned long) (prefix_length + 6),
			(unsigned long) max_destination_length );
	}

	for ( i = 0; i < prefix_length; i++ ) {
		destination[i] = prefix[i];
	}

	ptr = destination + prefix_length;

	ptr[0] = (unsigned char) (( value >> 12 ) & 0xf);
	ptr[1] = (unsigned char) (( value >> 8 ) & 0xf);
	ptr[2] = (unsigned char) (( value >> 4 ) & 0xf);
	ptr[3] = (unsigned char) (value & 0xf);
	ptr[4] = 0xff;
	ptr[5] = 0x0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sony_visca_handle_error( MX_SONY_VISCA *sony_visca,
				int error_type,
				char *sent_visca_ascii )
{
	static const char fname[] = "mxi_sony_visca_handle_error()";

	unsigned char c;
	mx_status_type mx_status;

	/* If we received an error code, there should be a 0xff byte
	 * message terminator after it.  Read that terminator and
	 * throw it away.
	 */

	mx_status = mx_rs232_getchar( sony_visca->rs232_record,
					(char *) &c, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != 0xff ) {
		mx_warning( "The error message received from "
		"Sony VISCA interface '%s' was not terminated "
		"with a 0xff byte.  Instead, it was "
		"terminated with a %#x byte.",
			sony_visca->record->name, c );
	}

	switch( error_type ) {
	case 0x01:
		return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
		"The message '%s' sent to Sony VISCA interface '%s' was longer "
		"than the maximum length of 14 bytes.",
			sent_visca_ascii,
			sony_visca->record->name );
	case 0x02:
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"The message '%s' sent to Sony VISCA interface '%s' had "
		"a syntax error in it.",
			sent_visca_ascii,
			sony_visca->record->name );
	case 0x03:
		return mx_error( MXE_TRY_AGAIN, fname,
		"The command buffer for Sony VISCA interface '%s' is full.",
			sony_visca->record->name );
	case 0x04:
		return mx_error( MXE_INTERRUPTED, fname,
		"The command '%s' for Sony VISCA interface '%s' was cancelled.",
			sent_visca_ascii,
			sony_visca->record->name );
	case 0x05:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"No socket for command '%s' (to be canceled) for "
		"Sony VISCA interface '%s'.",
			sent_visca_ascii,
			sony_visca->record->name );
	case 0x41:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Command '%s' for Sony VISCA interface '%s' is not valid.",
			sent_visca_ascii,
			sony_visca->record->name );
	default:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"An unrecognized error code %#x was received "
		"for command '%s' sent to Sony VISCA interface '%s'.",
			error_type,
			sent_visca_ascii,
			sony_visca->record->name );
	}

	/* Borland C insists that there be a return statement here, while
	 * other compilers insist that there not be one.
	 */

#if defined(__BORLANDC__)
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxi_sony_visca_cmd( MX_SONY_VISCA *sony_visca,
			int camera_number,
			unsigned char *command,
			unsigned char *response,
			size_t max_response_length,
			size_t *actual_response_length )
{
	static const char fname[] = "mxi_sony_visca_cmd()";

	char local_receive_buffer[80], hex_buffer[10];
	char sent_visca_ascii[ 5 * MXU_SONY_VISCA_MAX_CMD_LENGTH + 1];
	char received_visca_ascii[ 5 * MXU_SONY_VISCA_MAX_CMD_LENGTH + 1];
	unsigned char c, response_type, first_byte, second_byte, third_byte;
	char *command_ptr;
	size_t num_response_bytes, bytes_read;
	int num_command_body_bytes, more_message_to_read, exit_loop;
	int encoded_camera_number, received_camera_number;
	unsigned long i, wait_ms, max_attempts;
	uint32_t num_bytes_available;
	mx_status_type mx_status;

	/* How many bytes are in the command body? */

	num_command_body_bytes = 0;

	command_ptr = (char *) command;

	for (;;) {
		num_command_body_bytes++;

		if ( *command_ptr == '\xff' ) {
			break;			/* Exit the for(;;) loop. */
		}

		command_ptr++;
	}

	/* Compute the encoded camera number. */

	encoded_camera_number = 0x80 + camera_number;

	/* If requested, display the command to the user. */

	snprintf( sent_visca_ascii, sizeof(sent_visca_ascii),
			"%#x ", encoded_camera_number );

	for ( i = 0; i < num_command_body_bytes; i++ ) {
		snprintf( hex_buffer, sizeof(hex_buffer),
			"%#x ", command[i] );

		strlcat(sent_visca_ascii, hex_buffer, sizeof(sent_visca_ascii));
	}

#if MXI_SONY_VISCA_DEBUG
	MX_DEBUG(-2,("%s: sending %s to Sony PTZ controller '%s'.",
		fname, sent_visca_ascii, sony_visca->record->name ));
#endif

	/* Send the encoded camera number. */

	mx_status = mx_rs232_putchar( sony_visca->rs232_record,
					(char) encoded_camera_number, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command body. */

	mx_status = mx_rs232_write( sony_visca->rs232_record,
				(char *) command, num_command_body_bytes,
				NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the first three characters of the response and
	 * decide what to do with the message.
	 */

	for (;;) {

		/* Wait for the first character in the response. */

		wait_ms = 100;
		max_attempts = 50;

		for ( i = 0; i < max_attempts; i++ ) {
			mx_status = mx_rs232_num_input_bytes_available(
					sony_visca->rs232_record,
					&num_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_bytes_available != 0 ) {
				break;		/* Exit the for(i) loop. */
			}

			mx_msleep( wait_ms );
		}

		if ( i >= max_attempts ) {
			return mx_error( MXE_TIMED_OUT, fname,
		    "Timed out after waiting %g seconds for a response from "
		    "Sony Pan/Tilt/Zoom controller '%s' to the command %s.",
				1000.0 * (double)( wait_ms * max_attempts ),
				sony_visca->record->name,
				sent_visca_ascii );
		}

		/* Read in the first three characters. */

		mx_status = mx_rs232_read( sony_visca->rs232_record,
						local_receive_buffer,
						3, &bytes_read, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( bytes_read != 3 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Read only %lu bytes from Sony VISCA interface '%s' "
			"rather than the 3 bytes we expected.",
				(unsigned long) bytes_read,
				sony_visca->record->name );
		}

		first_byte  = local_receive_buffer[0];
		second_byte = local_receive_buffer[1];
		third_byte  = local_receive_buffer[2];

		if ( first_byte != 0x88 ) {
			/* Does the returned camera number match the number
			 * that we sent?
			 */

		received_camera_number = ( first_byte & 0x70 ) >> 4;

			if (received_camera_number != camera_number) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"The camera number %d in the message received "
				"from RS-232 port '%s' does not match the "
				"expected camera number %d.",
					received_camera_number,
					sony_visca->record->name,
					camera_number );
			}
		}

		/* If this is not the response to a broadcast command,
		 * then figure out what kind of response this is.
		 */

		if ( first_byte == 0x88 ) {
			more_message_to_read = TRUE;
			exit_loop = TRUE;
		} else {
			response_type = ( second_byte & 0xf0 ) >> 4;

			more_message_to_read = TRUE;
			exit_loop = FALSE;

			switch ( response_type ) {
			case 4:
				/* This should be an ACK message.  If so,
				 * it should be followed by 0xff.
				 */

				 if ( third_byte != 0xff ) {
				 	return mx_error(
						MXE_INTERFACE_IO_ERROR, fname,
					"The ACK byte %#x was was followed by "
					"the byte %#x which should have been "
					"0xff instead.",
						local_receive_buffer[1],
						third_byte );
				}

				more_message_to_read = FALSE;
				exit_loop = TRUE;
				break;
			case 5:
				/* If the third byte is a 0xff, then this is a 
				 * command completion message which we will
				 * ignore.  Otherwise, we assume it is an
				 * inquiry response that we will need to return
				 * to the caller.
				 */

				if ( third_byte == 0xff ) {
				 	/* This is a command completion message
					 * which we will skip over.  This means
					 * that we need to go back to the top
					 * of the for(;;) loop to look for the
					 * message we really want.
					 */

					MX_DEBUG(-2,
					("%s: command completion message seen.",
						fname ));

					more_message_to_read = FALSE;
					exit_loop = FALSE;
				} else {
					/* This is an inquiry message that we
					 * may need to return to the caller.
					 */

					more_message_to_read = TRUE;
					exit_loop = TRUE;
				}
				break;
			case 6:
				/* This is an error message. */

				mx_status = mxi_sony_visca_handle_error(
						sony_visca, third_byte,
						sent_visca_ascii );

				return mx_status;
			default:
				(void) mx_rs232_discard_unread_input(
						sony_visca->rs232_record,
						MXI_SONY_VISCA_DEBUG );

				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Unknown response type %d seen in response "
				"%#x %#x %#x from Sony VISCA "
				"interface '%s'.",
					response_type, first_byte,
					second_byte, third_byte,
					sony_visca->record->name );
			}
		}

		if ( exit_loop ) {
			 snprintf( received_visca_ascii,
			 	sizeof(received_visca_ascii),
				"%#x %#x %#x ", first_byte,
				second_byte, third_byte );

			 break;		/* Exit the for(;;) loop. */
		}
	}

	/* Read in the rest of response. */

	if ( more_message_to_read == FALSE ) {
		num_response_bytes = 3;
	} else {
		for ( i = 3; i < sizeof(local_receive_buffer); i++ ) {
			mx_status = mx_rs232_getchar(
						sony_visca->rs232_record,
						(char *) &c, MXF_232_WAIT );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			local_receive_buffer[i] = c;

			snprintf( hex_buffer, sizeof(hex_buffer),
					"%#x ", (unsigned char) c );

			strlcat( received_visca_ascii, hex_buffer,
				sizeof( received_visca_ascii ) );

			if ( ((unsigned char) c ) == 0xff ) {
				break;		/* Exit the while() loop. */
			}
		}

		if ( i >= sizeof(local_receive_buffer) ) {
			(void) mx_rs232_discard_unread_input(
				sony_visca->rs232_record, 0 );

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The response from Sony VISCA interface '%s' was "
			"too long to fit into the local receive buffer.  "
			"Response = '%s'.",
				sony_visca->record->name,
				received_visca_ascii );
		}
		num_response_bytes = i+1;
	}

#if MXI_SONY_VISCA_DEBUG
	MX_DEBUG(-2,("%s: received %s from Sony PTZ controller '%s'.",
		fname, received_visca_ascii, sony_visca->record->name ));
#endif

	if ( response != NULL ) {
		if ( num_response_bytes > max_response_length ) {
			mx_warning( "The number of bytes %lu received from "
			"Sony VISCA interface '%s' was greater than "
			"the size %lu of the buffer supplied by the caller.  "
			"The message returned will be truncated.",
				(unsigned long) num_response_bytes,
				sony_visca->record->name,
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

