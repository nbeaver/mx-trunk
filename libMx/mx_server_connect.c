/*
 * Name:    mx_server_connect.c
 *
 * Purpose: This routine sets up a connection to a remote MX server without
 *          the need to explicitly setup an entire MX database.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"

mx_status_type
mx_connect_to_mx_server( MX_RECORD **server_record,
			char *server_name, int server_port,
			int default_display_precision )
{
	static const char fname[] = "mx_connect_to_mx_server()";

	MX_RECORD *record_list;
	MX_LIST_HEAD *list_head_struct;
	static char description[MXU_RECORD_DESCRIPTION_LENGTH + 1];
	int i, max_retries;
	mx_status_type status;

	MX_DEBUG( 2,("%s: server_name = '%s', server_port = %d",
			fname, server_name, server_port ));

	/* Initialize MX device drivers. */

	status = mx_initialize_drivers();

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Set up a record list to put this server record in. */

	record_list = mx_initialize_record_list();

	if ( record_list == NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to setup an MX record list." );
	}

	/* Set the default floating point display precision. */

	list_head_struct = mx_get_record_list_head_struct( record_list );

	if ( list_head_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX record list head structure is NULL." );
	}

	list_head_struct->default_precision = default_display_precision;

	/* Set the MX program name. */

	strlcpy( list_head_struct->program_name, "MX client",
				MXU_PROGRAM_NAME_LENGTH );

	/* Create a record description for this server. */

	sprintf( description,
		"remote_server server network tcpip_server \"\" \"\" 0x0 %s %d",
			server_name, server_port );

	MX_DEBUG( 2,("%s: description = '%s'", fname, description));

	status = mx_create_record_from_description( record_list, description,
							server_record, 0 );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_finish_record_initialization( *server_record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the record list as ready to be used. */

	list_head_struct->list_is_active = TRUE;
	list_head_struct->fixup_records_in_use = FALSE;

	/* Try to connect to the MX server. */

	max_retries = 100;

	for ( i = 0; i < max_retries; i++ ) {
		status = mx_open_hardware( *server_record );

		if ( status.code == MXE_SUCCESS )
			break;                 /* Exit the for() loop. */

		switch( status.code ) {
		case MXE_NETWORK_IO_ERROR:
			break;                 /* Try again. */
		default:
			return status;
			break;
		}
		mx_msleep(1000);
	}

	if ( status.code != MXE_SUCCESS ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
"%d attempts to connect to the MX server '%s' at port %d have failed.  "
"Update process aborting...", max_retries, server_name, server_port );
	}

	return MX_SUCCESSFUL_RESULT;
}

