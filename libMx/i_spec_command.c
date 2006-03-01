/*
 * Name:    i_spec_command.c
 *
 * Purpose: MX driver for sending to and receiving from the Spec command
 *          line as if it were an RS-232 device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SPEC_COMMAND_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_spec.h"
#include "n_spec.h"
#include "i_spec_command.h"

MX_RECORD_FUNCTION_LIST mxi_spec_command_record_function_list = {
	NULL,
	mxi_spec_command_create_record_structures,
	mxi_spec_command_finish_record_initialization
};

MX_RS232_FUNCTION_LIST mxi_spec_command_rs232_function_list = {
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_spec_command_getline,
	mxi_spec_command_putline,
	mxi_spec_command_num_input_bytes_available,
	mxi_spec_command_discard_unread_input
};

MX_RECORD_FIELD_DEFAULTS mxi_spec_command_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_SPEC_COMMAND_STANDARD_FIELDS
};

long mxi_spec_command_num_record_fields
		= sizeof( mxi_spec_command_record_field_defaults )
			/ sizeof( mxi_spec_command_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_spec_command_rfield_def_ptr
			= &mxi_spec_command_record_field_defaults[0];

/* ---- */

static mx_status_type
mxi_spec_command_get_pointers( MX_RS232 *rs232,
			MX_SPEC_COMMAND **spec_command,
			MX_SPEC_SERVER **spec_server,
			const char *calling_fname )
{
	const char fname[] = "mxi_spec_command_get_pointers()";

	MX_RECORD *spec_command_record, *spec_server_record;
	MX_SPEC_COMMAND *spec_command_ptr;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	spec_command_record = rs232->record;

	if ( spec_command_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The spec_command_record pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	spec_command_ptr = (MX_SPEC_COMMAND *)
				spec_command_record->record_type_struct;


	if ( spec_command_ptr == (MX_SPEC_COMMAND *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SPEC_COMMAND pointer for record '%s' is NULL.",
			spec_command_record->name );
	}

	if ( spec_command != (MX_SPEC_COMMAND **) NULL ) {
		*spec_command = spec_command_ptr;
	}

	if ( spec_server != (MX_SPEC_SERVER **) NULL ) {
		spec_server_record = spec_command_ptr->spec_server_record;

		if ( spec_server_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The spec_server_record pointer for record '%s' is NULL.",
				spec_command_record->name );
		}

		*spec_server = (MX_SPEC_SERVER *)
				spec_server_record->record_type_struct;

		if ( *spec_server == (MX_SPEC_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SPEC_SERVER pointer for record '%s' is NULL.",
				spec_server_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/


MX_EXPORT mx_status_type
mxi_spec_command_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_spec_command_create_record_structures()";

	MX_RS232 *rs232;
	MX_SPEC_COMMAND *spec_command;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	spec_command = (MX_SPEC_COMMAND *) malloc( sizeof(MX_SPEC_COMMAND) );

	if ( spec_command == (MX_SPEC_COMMAND *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SPEC_COMMAND structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = spec_command;
	record->class_specific_function_list =
				&mxi_spec_command_rs232_function_list;

	rs232->record = record;
	spec_command->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_spec_command_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	/* Check to see if the RS-232 parameters are valid. */

	mx_status = mx_rs232_check_port_parameters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the SPEC_COMMAND device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_spec_command_getline( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read )
{
	static const char fname[] = "mxi_spec_command_getline()";

	MX_SPEC_SERVER *spec_server;
	mx_status_type mx_status;

	mx_status = mxi_spec_command_get_pointers( rs232,
						NULL, &spec_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_spec_receive_response_line( spec_server->record,
					buffer, (long) max_bytes_to_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != NULL ) {
		*bytes_read = strlen( buffer );
	}

#if MXI_SPEC_COMMAND_DEBUG
	MX_DEBUG(-2,("%s read '%s' from spec server '%s'",
		fname, buffer, spec_server->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_spec_command_putline( MX_RS232 *rs232,
			char *buffer,
			size_t *bytes_written )
{
	static const char fname[] = "mxi_spec_command_putline()";

	MX_SPEC_SERVER *spec_server;
	mx_status_type mx_status;

	mx_status = mxi_spec_command_get_pointers( rs232,
						NULL, &spec_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SPEC_COMMAND_DEBUG
	MX_DEBUG(-2,("%s: sending '%s' to spec server '%s'.",
			fname, buffer, spec_server->record->name));
#endif
	mx_status = mx_spec_send_command( spec_server->record,
					SV_CMD_WITH_RETURN, buffer );

	if ( bytes_written != NULL ) {
		*bytes_written = strlen(buffer);
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_spec_command_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_spec_command_num_input_bytes_available()";

	MX_SPEC_COMMAND *spec_command;
	MX_SPEC_SERVER *spec_server;
	long num_input_bytes_available;
	mx_status_type mx_status;

	mx_status = mxi_spec_command_get_pointers( rs232,
					&spec_command, &spec_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_spec_num_response_bytes_available( spec_server->record,
						&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->num_input_bytes_available = num_input_bytes_available;

#if MXI_SPEC_COMMAND_DEBUG
	MX_DEBUG(-2,("%s: num_input_bytes_available = %d",
			fname, rs232->num_input_bytes_available));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_spec_command_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_spec_command_discard_unread_input()";

	MX_SPEC_SERVER *spec_server;
	mx_status_type mx_status;

	mx_status = mxi_spec_command_get_pointers( rs232,
						NULL, &spec_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SPEC_COMMAND_DEBUG
	MX_DEBUG(-2,("%s: discarding unread input for record '%s'.",
		fname, rs232->record->name ));
#endif

	mx_status = mx_spec_discard_unread_responses( spec_server->record );

	return mx_status;
}

