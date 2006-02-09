/*
 * Name:    i_tpg262.c
 *
 * Purpose: MX interface driver for Pfeiffer TPG 261 and TPG 262 vacuum
 *          gauge controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_TPG262_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_tpg262.h"

MX_RECORD_FUNCTION_LIST mxi_tpg262_record_function_list = {
	NULL,
	mxi_tpg262_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_tpg262_open,
	NULL,
	NULL,
	mxi_tpg262_resynchronize
};

MX_RECORD_FIELD_DEFAULTS mxi_tpg262_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_TPG262_STANDARD_FIELDS
};

mx_length_type mxi_tpg262_num_record_fields
		= sizeof( mxi_tpg262_record_field_defaults )
			/ sizeof( mxi_tpg262_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_tpg262_rfield_def_ptr
			= &mxi_tpg262_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_tpg262_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_tpg262_create_record_structures()";

	MX_TPG262 *tpg262;

	/* Allocate memory for the necessary structures. */

	tpg262 = (MX_TPG262 *) malloc( sizeof(MX_TPG262) );

	if ( tpg262 == (MX_TPG262 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TPG262 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = tpg262;

	record->record_function_list = &mxi_tpg262_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	tpg262->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tpg262_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_tpg262_open()";

	MX_TPG262 *tpg262;
	MX_RS232 *rs232;
	char response[40];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	tpg262 = (MX_TPG262 *) record->record_type_struct;

	if ( tpg262 == (MX_TPG262 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TPG262 pointer for record '%s' is NULL.", record->name);
	}

	if ( tpg262->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The 'rs232_record' pointer for TPG262 controller '%s' is NULL.",
			record->name );
	}

	mx_status = mx_rs232_get_pointers( tpg262->rs232_record,
						&rs232, NULL, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that RS-232 line terminators are set to 0x0d0a. */

	if ( ( rs232->read_terminators != 0x0d0a )
	  || ( rs232->write_terminators != 0x0d0a ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The read and write terminators for RS-232 port '%s' used "
		"by TPG 262 controller '%s' must both be set to 0x0d0a.",
			tpg262->rs232_record->name, record->name );
	}

	/* Discard any pending I/O for the controller, since the
	 * controller may have been in continuous output mode.
	 */

	mx_status = mx_rs232_discard_unwritten_output( tpg262->rs232_record,
							MXI_TPG262_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( tpg262->rs232_record,
							MXI_TPG262_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop continuous output from the TPG 262 by requesting
	 * its firmware version.
	 */

	mx_status = mx_rs232_putline( tpg262->rs232_record, "PNR",
					NULL, MXI_TPG262_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any output generated, which may still include a line
	 * or two of continuous output.
	 */

	mx_status = mx_rs232_discard_unread_input( tpg262->rs232_record,
							MXI_TPG262_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We should now be synchronized with the TPG 262, so we now
	 * ask again for the firmware version with the intention of
	 * actually looking at the value this time.
	 */

	mx_status = mxi_tpg262_command( tpg262, "PNR",
					response, sizeof(response),
					MXI_TPG262_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: TPG 262 '%s' firmware version = '%s'",
				fname, record->name, response ));

	/* Ask for the gauge IDs of connected gauges. */

	mx_status = mxi_tpg262_command( tpg262, "TID",
					response, sizeof(response),
					MXI_TPG262_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: TPG 262 '%s' gauge IDs = '%s'",
				fname, record->name, response ));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_tpg262_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_tpg262_resynchronize()";

	MX_TPG262 *tpg262;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	tpg262 = (MX_TPG262 *) record->record_type_struct;

	if ( tpg262 == (MX_TPG262 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TPG262 pointer for record '%s' is NULL.", record->name);
	}

	mx_status = mx_resynchronize_record( tpg262->rs232_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_tpg262_command( MX_TPG262 *tpg262,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_tpg262_command()";

	char local_response_buffer[100];
	char *response_ptr;
	size_t response_ptr_length;
	int command_failed;
	mx_status_type mx_status;

	if ( tpg262 == (MX_TPG262 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_TPG262 pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, tpg262->record->name ));
	}

	/* Send the command. */

	mx_status = mx_rs232_putline( tpg262->rs232_record, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* At this stage, the TPG returns a line of the form <ACK><CR><LF>
	 * if the command succeeded, or <NAK><CR><LF> if it failed.
	 */

	mx_status = mx_rs232_getline( tpg262->rs232_record,
					local_response_buffer,
					sizeof( local_response_buffer ),
					NULL, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( local_response_buffer[0] == MX_NAK ) {
		command_failed = TRUE;
	} else
	if ( local_response_buffer[0] == MX_ACK ) {
		command_failed = FALSE;
	} else {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unexpected command status byte (%#x) seen in response to "
		"the command '%s' sent to TPG 262 controller '%s'.  "
		"The command status byte should have been either an ACK (0x06) "
		"or a NAK (0x15).", local_response_buffer[0], command,
			tpg262->record->name );
	}

	if ( command_failed ) {
		/* If an existing attempt to ask the controller why the
		 * previous command failed has in itself failed, then
		 * we give up here.  Otherwise, we might get an infinite
		 * recursion of command failures.
		 */

		if ( debug_flag & MXF_TPG262_ERR_COMMAND_IN_PROGRESS ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"An 'ERR' command sent to ask the TPG 262 controller '%s' why "
		"a previous command failed has itself failed.  In order to "
		"avoid infinite recursion, we will stop trying to find out "
		"what went wrong at this point.",  tpg262->record->name );
		}

		/* Otherwise, ask the controller why it failed.  The 'ERR'
		 * command uses the same syntax as any other command, so we
		 * must do the same handshaking as for the original command.
		 * However, we set a flag to prevent infinite recursion.
		 */

		mx_status = mxi_tpg262_command( tpg262, "ERR",
					local_response_buffer,
					sizeof( local_response_buffer ),
			( debug_flag | MXF_TPG262_ERR_COMMAND_IN_PROGRESS ) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The command '%s' sent to TPG 262 controller '%s' failed.  "
		"Error status = '%s'",
			command, tpg262->record->name, local_response_buffer );
	}

	/* If we get here, the command did _not_ fail.  We now ask the 
	 * controller to send the real response line by sending it an <ENQ>
	 * character.
	 */

	mx_status = mx_rs232_putchar( tpg262->rs232_record, MX_ENQ, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response into the caller's buffer if provided, or into
	 * a local buffer if the caller did not provide a buffer.
	 */

	if ( response != NULL ) {
		response_ptr = response;
		response_ptr_length = response_buffer_length;
	} else {
		response_ptr = local_response_buffer;
		response_ptr_length = sizeof( local_response_buffer );
	}

	mx_status = mx_rs232_getline( tpg262->rs232_record,
					response_ptr, response_ptr_length,
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response_ptr, tpg262->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

