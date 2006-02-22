/*
 * Name:    i_pfcu.c
 *
 * Purpose: MX interface driver for the X-ray Instrumentation Associates
 *          PFCU RS-232 interface to the PF4 and PF2S2 filter and shutter
 *          controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PFCU_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "i_pfcu.h"

MX_RECORD_FUNCTION_LIST mxi_pfcu_record_function_list = {
	NULL,
	mxi_pfcu_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_pfcu_open,
	NULL,
	NULL,
	mxi_pfcu_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxi_pfcu_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PFCU_STANDARD_FIELDS
};

long mxi_pfcu_num_record_fields
		= sizeof( mxi_pfcu_record_field_defaults )
			/ sizeof( mxi_pfcu_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pfcu_rfield_def_ptr
			= &mxi_pfcu_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_pfcu_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_pfcu_create_record_structures()";

	MX_PFCU *pfcu;

	/* Allocate memory for the necessary structures. */

	pfcu = (MX_PFCU *) malloc( sizeof(MX_PFCU) );

	if ( pfcu == (MX_PFCU *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PFCU structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = pfcu;

	record->record_function_list = &mxi_pfcu_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	pfcu->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pfcu_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_pfcu_open()";

	MX_PFCU *pfcu;
	MX_RS232 *rs232;
	unsigned long i, max_attempts;
	int timed_out;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	pfcu = (MX_PFCU *) record->record_type_struct;

	if ( pfcu == (MX_PFCU *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PFCU pointer for record '%s' is NULL.", record->name);
	}

	if ( pfcu->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The 'rs232_record' pointer for PFCU controller '%s' is NULL.",
			record->name );
	}

	mx_status = mx_rs232_get_pointers( pfcu->rs232_record,
						&rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pfcu->exposure_in_progress = FALSE;

	/* Check for a valid RS-232 configuration. */

	mx_status = mx_rs232_verify_configuration( pfcu->rs232_record,
			9600, 8, 'N', 1, 'N', 0x0d0a, 0x0d0a );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Attempt to start communication with the daisy-chained PFCUs.
	 *
	 * Sometimes they fail to respond to the first command sent to
	 * them, so we must include a retry loop with a timeout here.
	 * We suppress the timeout messages so that users will not be
	 * alarmed by them.
	 */

	rs232->rs232_flags |= MXF_232_SUPPRESS_TIMEOUT_ERROR_MESSAGES;

	/* If the RS-232 port does not have a timeout specified, set
	 * the timeout to 1 second.
	 */

	if ( rs232->timeout < 0.0 ) {
		rs232->timeout = 1.0;

		MX_DEBUG( 2,("%s: forcing the timeout to 1 second.", fname));
	}

	/* Start the retry loop. */

	max_attempts = 5;

	for ( i = 0; i < max_attempts; i++ ) {

		/* Lock or unlock the front panel depending on the value
		 * of 'pfcu_flags'.
		 */

		if ( pfcu->pfcu_flags & MXF_PFCU_LOCK_FRONT_PANEL ) {
			mx_status = mxi_pfcu_command( pfcu, MX_PFCU_PFCUALL,
						"L", NULL, 0, MXI_PFCU_DEBUG );
		} else {
			mx_status = mxi_pfcu_command( pfcu, MX_PFCU_PFCUALL,
						"U", NULL, 0, MXI_PFCU_DEBUG );
		}

		if ( mx_status.code == MXE_TIMED_OUT ) {
			timed_out = TRUE;
		} else {
			timed_out = FALSE;
		}

		if (( mx_status.code != MXE_SUCCESS )
		  && ( mx_status.code != MXE_TIMED_OUT ))
			return mx_status;

		if ( timed_out == FALSE )
			break;			/* Exit the for loop(). */

		/* More than one PFCU may reply.  If so, this will generate
		 * more than one line of response.
		 */

		mx_msleep( 100 );

		mx_status = mxi_pfcu_resynchronize( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
	"%lu attempts to communicate with filter daisy chain '%s' timed out.  "
	"Are you sure that the PFCUs are connected and powered on?",
			max_attempts, pfcu->record->name );
	}

	/* Enable or disable shutter commands depending on the value
	 * of 'pfcu_flags'.
	 */

	if ( pfcu->pfcu_flags & MXF_PFCU_ENABLE_SHUTTER ) {
		mx_status = mxi_pfcu_command( pfcu, MX_PFCU_PFCUALL,
						"2", NULL, 0, MXI_PFCU_DEBUG );
	} else {
		mx_status = mxi_pfcu_command( pfcu, MX_PFCU_PFCUALL,
						"4", NULL, 0, MXI_PFCU_DEBUG );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* More than one PFCU may reply.  If so, this will generate more than
	 * one line of response.
	 */

	mx_msleep( 100 );

	mx_status = mxi_pfcu_resynchronize( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_pfcu_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_pfcu_resynchronize()";

	MX_PFCU *pfcu;
	MX_RECORD *rs232_record;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	pfcu = (MX_PFCU *) record->record_type_struct;

	if ( pfcu == (MX_PFCU *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PFCU pointer for record '%s' is NULL.", record->name);
	}

	rs232_record = pfcu->rs232_record;

	mx_status = mx_rs232_discard_unread_input( rs232_record,
							MXI_PFCU_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unwritten_output( rs232_record,
							MXI_PFCU_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_resynchronize_record( rs232_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_pfcu_command( MX_PFCU *pfcu,
			unsigned long module_number,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_pfcu_command()";

	char command_buffer[100];
	char response_buffer[100];
	char *status_ptr, *status_end_ptr;
	char *error_message_ptr, *response_ptr;
	size_t response_length, status_length;
	mx_status_type mx_status;

	if ( pfcu == (MX_PFCU *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PFCU pointer passed was NULL." );
	}

	if ( pfcu->exposure_in_progress ) {
		return mx_error( MXE_NOT_READY, fname,
"Cannot send a command to PFCU '%s' since a shutter exposure is in progress.",
			pfcu->record->name );
	}

	if ( module_number == MX_PFCU_PFCUALL ) {
		sprintf( command_buffer, "!PFCUALL %s", command );
	} else {
		sprintf( command_buffer, "!PFCU%02lu %s",
					module_number, command );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command_buffer, pfcu->record->name ));
	}

	/* Send the command. */

	mx_status = mx_rs232_putline( pfcu->rs232_record,
					command_buffer, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response from the PFCU. */

	mx_status = mx_rs232_getline( pfcu->rs232_record,
					response_buffer,
					sizeof( response_buffer ),
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response_buffer, pfcu->record->name ));
	}

	/* Remove the trailing semicolon.  mx_rs232_getline() will already
	 * have removed the trailing carriage return character.
	 */

	response_length = strlen( response_buffer );

	if ( response_length == 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"No characters were seen in the response to command '%s' by PFCU '%s'.",
			command_buffer, pfcu->record->name );
	}

#if 0
	{
		int i;

		for ( i = 0; i < response_length; i++ ) {
			MX_DEBUG(-2,("%s: response_buffer[%d] = '%c' (%#x)",
			    fname, i, response_buffer[i], response_buffer[i]));
		}
	}
#endif

	if ( response_buffer[response_length-1] == ';' ) {
		response_buffer[response_length-1] = '\0';
	}

	/* Find the start of the command status code. */

	status_ptr = strchr( response_buffer, ' ' );

	if ( status_ptr == NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Cannot find the space character separating the module id from the "
	"command status in the response '%s' to command '%s' for PFCU '%s'.",
			response_buffer, command_buffer, pfcu->record->name );
	}

	status_ptr++;

	/* Find the length of the command status code. */

	status_end_ptr = strchr( status_ptr, ' ' );

	if ( status_end_ptr == NULL ) {
		status_length = strlen( status_ptr );
	} else {
		status_length = status_end_ptr - status_ptr;
	}

	/* Figure out which status code was returned by the PFCU. */

	if ( strncmp( status_ptr, "ERROR:", status_length ) == 0 ) {
		if ( status_end_ptr == NULL ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"PFCU '%s' returned an ERROR status for command '%s' "
		"without any further information.",
				pfcu->record->name, command_buffer );
		} else {
			error_message_ptr = status_end_ptr + 1;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"PFCU '%s' returned the error '%s' for command '%s'.",
				pfcu->record->name,
				error_message_ptr,
				command_buffer );
		}
	}

	if ( strncmp( status_ptr, "OK", status_length ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"PFCU '%s' returned an unrecognized status code in "
		"the response '%s' to command '%s'.",
			pfcu->record->name, response_buffer, command_buffer );
	}

	/* If the calling routine expected a response, copy the response to it.
	 */

	if ( response != NULL ) {
		if ( status_end_ptr == NULL ) {
			/* There was no response text to copy. */

			strcpy( response, "" );
		} else {
			response_ptr = status_end_ptr + 1;

			strlcpy( response, response_ptr,
					response_buffer_length );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

