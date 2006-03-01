/*
 * Name:    i_panasonic_kx_dp702.c
 *
 * Purpose: MX driver for the Panasonic VISCA protocol.
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

#define MXI_PANASONIC_KX_DP702_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_panasonic_kx_dp702.h"

MX_RECORD_FUNCTION_LIST mxi_panasonic_kx_dp702_record_function_list = {
	NULL,
	mxi_panasonic_kx_dp702_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_panasonic_kx_dp702_open
};

MX_RECORD_FIELD_DEFAULTS mxi_panasonic_kx_dp702_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PANASONIC_KX_DP702_STANDARD_FIELDS
};

long mxi_panasonic_kx_dp702_num_record_fields =
		sizeof( mxi_panasonic_kx_dp702_rf_defaults )
		/ sizeof( mxi_panasonic_kx_dp702_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_panasonic_kx_dp702_rfield_def_ptr
			= &mxi_panasonic_kx_dp702_rf_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxi_panasonic_kx_dp702_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxi_panasonic_kx_dp702_create_record_structures()";

        MX_PANASONIC_KX_DP702 *panasonic_kx_dp702;

        /* Allocate memory for the necessary structures. */

        panasonic_kx_dp702 = (MX_PANASONIC_KX_DP702 *)
					malloc( sizeof(MX_PANASONIC_KX_DP702) );

        if ( panasonic_kx_dp702 == (MX_PANASONIC_KX_DP702 *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Unable to allocate memory for an MX_PANASONIC_KX_DP702 structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = NULL;
        record->record_type_struct = panasonic_kx_dp702;
        record->class_specific_function_list = NULL;

	panasonic_kx_dp702->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_panasonic_kx_dp702_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_panasonic_kx_dp702_open()";

	MX_PANASONIC_KX_DP702 *kx_dp702;
	unsigned char response[20];
	size_t num_response_bytes;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	kx_dp702 = (MX_PANASONIC_KX_DP702 *) record->record_type_struct;

	if ( kx_dp702 == (MX_PANASONIC_KX_DP702 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PANASONIC_KX_DP702 pointer passed was NULL." );
	}

#if MXI_PANASONIC_KX_DP702_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	/* Empty out the receive buffer. */

	mx_status = mx_rs232_discard_unread_input( kx_dp702->rs232_record,
						MXI_PANASONIC_KX_DP702_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the daisy chain command.
	 *
	 * Setting the last camera number to 0 and then sending a command
	 * to camera 0 will inhibit the mxi_panasonic_kx_dp702_raw_cmd()
	 * function from sending out a camera selection command for this
	 * command.
	 */

	kx_dp702->last_camera_number = 0;

	mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702, 0,
					(unsigned char *) "\xee\x00", 2,
					response, 2,
					&num_response_bytes );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_response_bytes != 2 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not receive the expected 2 byte response for the "
		"daisy chain command sent to Panasonic PTZ "
		"interface '%s'.  Instead saw %lu bytes.",
			kx_dp702->record->name,
			(unsigned long) num_response_bytes );
	}

	if ( response[0] != 0xee ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not receive the byte 0xee at the start of the "
			"response to the daisy chain command sent to "
			"Panasonic PTZ interface '%s'.  "
			"Instead saw %#x.", kx_dp702->record->name,
				response[0] );
	}

	kx_dp702->num_cameras = response[1];

	if ( kx_dp702->num_cameras > 255 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Panasonic PTZ interface '%s' reported that it has "
			"%d cameras which is more than the maximum of 255.",
				kx_dp702->record->name, kx_dp702->num_cameras );
	}

#if MXI_PANASONIC_KX_DP702_DEBUG
	MX_DEBUG(-2,("%s: Panasonic PTZ interface '%s', %d cameras detected.",
		fname, kx_dp702->record->name, kx_dp702->num_cameras ));
#endif

	return mx_status;
}

/* ----- */

MX_EXPORT mx_status_type
mxi_panasonic_kx_dp702_cmd( MX_PANASONIC_KX_DP702 *kx_dp702,
			long camera_number,
			unsigned char *command,
			size_t command_length )
{
	static const char fname[] = "mxi_panasonic_kx_dp702_cmd()";

	unsigned char response[1];
	size_t actual_response_length;
	mx_status_type mx_status;

	mx_status = mxi_panasonic_kx_dp702_raw_cmd( kx_dp702,
			camera_number, command, command_length,
			response, sizeof(response),
			&actual_response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( response[0] ) {
	case 0xb1:		/* ACK response. */

		return MX_SUCCESSFUL_RESULT;
	case 0xb4:		/* NAK response. */

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Error in command sent to Panasonic PTZ '%s'.",
			kx_dp702->record->name );
	default:
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unrecognized first character %#x in response "
			"to command sent to Panasonic PTZ '%s'.",
			response[0], kx_dp702->record->name );
	}

	/* Borland C insists that there be a return statement here, while
	 * other compilers insist that there not be one.
	 */

#if defined(__BORLANDC__)
	return MX_SUCCESSFUL_RESULT;
#endif
}


MX_EXPORT mx_status_type
mxi_panasonic_kx_dp702_raw_cmd( MX_PANASONIC_KX_DP702 *kx_dp702,
			long camera_number,
			unsigned char *command,
			size_t command_length,
			unsigned char *response,
			size_t response_length,
			size_t *actual_response_length )
{
	static const char fname[] = "mxi_panasonic_kx_dp702_raw_cmd()";

	char command_ascii[80], response_ascii[80], hex_buffer[10];
	unsigned long i, num_bytes_available, wait_ms, max_attempts;
	unsigned char local_command[5], local_response[5];
	char *response_ptr;
	size_t bytes_read;
	int exit_loop;
	mx_status_type mx_status;

	command_ascii[0] = '\0';

	for ( i = 0; i < command_length; i++ ) {
		snprintf( hex_buffer, sizeof(hex_buffer), "%#x ", command[i] );

		strlcat( command_ascii, hex_buffer, sizeof(command_ascii) );
	}

	/* If the camera number is different than the last camera number,
	 * switch cameras.
	 */

	if ( kx_dp702->last_camera_number != camera_number ) {
		local_command[0] = 0xef;
		local_command[1] = (unsigned char) camera_number;

#if MXI_PANASONIC_KX_DP702_DEBUG
		MX_DEBUG(-2,("%s: Switching to camera %d",
				fname, camera_number));
#endif

		/* Send the switch camera command. */

		mx_status = mx_rs232_write( kx_dp702->rs232_record,
					(char *) local_command, 2, NULL,
					MXI_PANASONIC_KX_DP702_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Read back the echoed command. */

		mx_status = mx_rs232_read( kx_dp702->rs232_record,
					(char *) local_response, 2, NULL,
					MXI_PANASONIC_KX_DP702_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		kx_dp702->last_camera_number = camera_number;
	}

#if MXI_PANASONIC_KX_DP702_DEBUG
	MX_DEBUG(-2,
	("%s: Sending %sto Panasonic PTZ interface '%s', camera %d.",
		fname, command_ascii, kx_dp702->record->name, camera_number ));
#endif

	/* Send the command. */

	mx_status = mx_rs232_write( kx_dp702->rs232_record,
					(char *) command, command_length, NULL,
					MXI_PANASONIC_KX_DP702_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the first byte in the response. */

	wait_ms = 100;
	max_attempts = 50;

	for (;;) {
		for ( i = 0; i < max_attempts; i++ ) {
			mx_status = mx_rs232_num_input_bytes_available(
					kx_dp702->rs232_record,
					&num_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_bytes_available != 0 ) {
				break;		/* Exit the for(i,) loop. */
			}

			mx_msleep( wait_ms );
		}

		if ( i >= max_attempts ) {
			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds for a response "
			"from Panasonic Pan/Tilt/Zoom controller '%s' to "
			"the command %s.",
				1000.0 * (double)( wait_ms * max_attempts ),
				kx_dp702->record->name, command_ascii );
		}

		/* Read in the first character to see if it is just a
		 * camera completion code.
		 */

		mx_status = mx_rs232_read( kx_dp702->rs232_record,
				(char *) response, 1, &bytes_read,
				MXI_PANASONIC_KX_DP702_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exit_loop = FALSE;

		switch( response[0] ) {
		case 0xbb:
		case 0xbc:
		case 0xbd:
		case 0xbe:
		case 0xbf:
			break;
		default:
			/* We have received a byte that is not a camera
			 * completion code.  This means that we can exit
			 * this loop and read the rest of the response
			 * we were looking for.
			 */

			exit_loop = TRUE;
			break;
		}

		if ( exit_loop ) {
			break;		/* Exit the for(;;) loop. */
		}
	}

	/* Read in the rest of the characters. */

	if ( response_length > 1 ) {
		response_ptr = (char *) &(response[1]);

		mx_status = mx_rs232_read( kx_dp702->rs232_record,
				response_ptr, response_length-1, &bytes_read,
				MXI_PANASONIC_KX_DP702_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_read += 1;
	}

	response_ascii[0] = '\0';

	for ( i = 0; i < bytes_read; i++ ) {
		snprintf( hex_buffer, sizeof(hex_buffer), "%#x ", response[i] );

		strlcat( response_ascii, hex_buffer, sizeof(response_ascii) );
	}

	if ( bytes_read != response_length ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Read only %lu bytes from Panasonic PTZ interface '%s' "
		"in the response '%s' to the command '%s' rather than "
		"the %lu bytes we expected.",
			(unsigned long) bytes_read,
			kx_dp702->record->name,
			response_ascii, command_ascii,
			(unsigned long) response_length );
	}

#if MXI_PANASONIC_KX_DP702_DEBUG
	MX_DEBUG(-2,("%s: Received %sfrom Panasonic PTZ '%s'.",
		fname, response_ascii, kx_dp702->record->name ));
#endif

	if ( actual_response_length != NULL ) {
		*actual_response_length = bytes_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

