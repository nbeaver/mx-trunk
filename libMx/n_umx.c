/*
 * Name:     n_umx.c
 *
 * Purpose:  Client interface to a UMX microcontroller-based server.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2019-2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXN_UMX_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_umx.h"
#include "n_umx.h"

MX_RECORD_FUNCTION_LIST mxn_umx_server_record_function_list = {
	NULL,
	mxn_umx_server_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxn_umx_server_open
};

MX_RECORD_FIELD_DEFAULTS mxn_umx_server_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXN_UMX_SERVER_STANDARD_FIELDS
};

long mxn_umx_server_num_record_fields
		= sizeof( mxn_umx_server_record_field_defaults )
			/ sizeof( mxn_umx_server_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_umx_server_rfield_def_ptr
			= &mxn_umx_server_record_field_defaults[0];

MX_EXPORT mx_status_type
mxn_umx_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxn_umx_server_create_record_structures()";

	MX_UMX_SERVER *umx_server;

	umx_server = (MX_UMX_SERVER *) malloc( sizeof(MX_UMX_SERVER) );

	if ( umx_server == (MX_UMX_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_UMX_SERVER structure." );
	}

	record->record_class_struct = NULL;
	record->record_type_struct = umx_server;
	record->class_specific_function_list = NULL;

	umx_server->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_umx_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_umx_server_open()";

	MX_UMX_SERVER *umx_server = NULL;
	MX_RECORD *rs232_record = NULL;
	size_t num_bytes_read;
	char response[80];
	unsigned long umx_flags;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	umx_server = (MX_UMX_SERVER *) record->record_type_struct;

	if ( umx_server == (MX_UMX_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMX_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	rs232_record = umx_server->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for UMX server '%s' is NULL.",
			record->name );
	}

	/* FIXME: Make sure that the RS232 interface is configured correctly? */


	/* We should have already received a UMX or ASCII MX server message. */

	umx_flags = umx_server->umx_flags;

	if ( (umx_flags & MXF_UMX_SERVER_SKIP_STARTUP_MESSAGE) == 0 ) {

		mx_status = mx_rs232_getline_with_timeout( rs232_record,
						response, sizeof(response),
						&num_bytes_read,
						MXN_UMX_DEBUG, 5.0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG(-2,("%s: num_bytes_read = %ld",
			fname, (long)num_bytes_read ));

		MX_DEBUG(-2,("%s: response = '%s'", fname, response));
	}

	return mx_status;
}

