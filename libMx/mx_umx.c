/*
 * Name: mx_umx.c
 *
 * Purpose: Client side of UMX protocol.  UMX is an ASCII-based protocol
 *          used by microcontrollers to control hardware.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2019-2020, 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_umx.h"
#include "n_umx.h"

/*------------------------------------------------------------------------*/

static mx_status_type
mxp_umx_error_message( MX_RECORD *mx_record,
			const char *device_name,
			const char *fname,
			const char *command,
			const char *response )
{
	unsigned long umx_error_code;
	int num_items;

	num_items = sscanf( response, "!E%lu", &umx_error_code );

	if ( num_items != 1 ) {
		umx_error_code = MXE_UNKNOWN_ERROR;
	}

	return mx_error( umx_error_code, fname,
		"UMX command '%s' returned error message '%s'",
			command, response );
}

/*------------------------------------------------------------------------*/

static mx_status_type
mxp_umx_handle_monitor_callback( MX_RECORD *rs232_record, char *buffer_ptr )
{
	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_umx_command( MX_RECORD *umx_record,
		const char *device_name,
		const char *fname,
		char *command,
		char *response,
		size_t max_response_length,
		mx_bool_type debug_flag
)
{
	MX_UMX_SERVER *umx_server = NULL;
	MX_RECORD *rs232_record = NULL;
	char local_response_buffer[200];
	char *buffer_ptr = NULL;
	size_t buffer_length;
	mx_status_type mx_status;

	if ( umx_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The umx_record pointer passed was NULL." );
	}

	umx_server = (MX_UMX_SERVER *) umx_record->record_type_struct;

	if ( umx_server == (MX_UMX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_UMX_SERVER pointer for UMX server '%s' is NULL.",
			umx_record->name );
	}

	if ( umx_server->umx_flags & MXF_UMX_SERVER_DEBUG ) {
		debug_flag = TRUE;
	}

	rs232_record = umx_server->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for UMX server '%s' is NULL.",
			umx_record->name );
	}

	if ( command != (char *) NULL ) {
		/* If requested, send a command. */

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
				fname, command, umx_record->name ));
		}

		mx_status = mx_rs232_putline( rs232_record, command,
					NULL, (unsigned long) 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( response != (char *) NULL ) {
		/* If requested, receive the response. */

		buffer_ptr = response;
		buffer_length = max_response_length;
	} else {
		/* If no response is expected, we still need to
		 * find out if the last command succeeded.
		 */

		buffer_ptr = local_response_buffer;
		buffer_length = sizeof(local_response_buffer);
	}

	while (TRUE) {
		/* We loop here just in case we need to retry the read.
		 * That can happen if we received a monitor callback '<'
		 * or a comment '#' line.
		 */

		mx_status = mx_rs232_getline( rs232_record,
					buffer_ptr, buffer_length,
					NULL, (unsigned long) 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, buffer_ptr, umx_record->name ));
		}

		switch( buffer_ptr[0] ) {
		case '$':	/* Success */

			/* If the caller expected a response, then the
			 * response is already in their buffer.  If it
			 * did not, then the response we got into the
			 * local buffer will be thrown away once we
			 * return.  In either case, we can just
			 * return now.
			 */

			return MX_SUCCESSFUL_RESULT;
			break;
		case '!':	/* Error */

			mx_status = mxp_umx_error_message( umx_record,
				device_name, fname, command, response );
			break;
		case '<':	/* A monitor callback. */

			mx_status = mxp_umx_handle_monitor_callback(
						rs232_record, buffer_ptr );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* If we get here, then we will retry the attempt
			 * to receive the response we were looking for. 
			 */

			continue; /* Go back to the top of the while () loop. */
			break;
		case '#':	/* We were sent a 'comment'. */

			/* Throw the rest of the line away and go back to the
			 * top of the while() loop to try again.
			 */

			continue; /* Go back to the top of the while () loop. */
			break;
		default:
			break;
		}

		/* If we get here, then we are done with processing
		 * responses for now and can return.
		 */

		return mx_status;
	}
}

