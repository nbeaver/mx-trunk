/*
 * Name:     n_http.c
 *
 * Purpose:  Client interface to an HTTP server.
 *
 * Author:   William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXN_HTTP_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_http.h"
#include "n_http.h"

MX_RECORD_FUNCTION_LIST mxn_http_server_record_function_list = {
	NULL,
	mxn_http_server_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxn_http_server_open
};

MX_RECORD_FIELD_DEFAULTS mxn_http_server_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXN_HTTP_SERVER_STANDARD_FIELDS
};

long mxn_http_server_num_record_fields
		= sizeof( mxn_http_server_record_field_defaults )
			/ sizeof( mxn_http_server_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxn_http_server_rfield_def_ptr
			= &mxn_http_server_record_field_defaults[0];

MX_EXPORT mx_status_type
mxn_http_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxn_http_server_create_record_structures()";

	MX_HTTP_SERVER *http_server;

	http_server = (MX_HTTP_SERVER *) malloc( sizeof(MX_HTTP_SERVER) );

	if ( http_server == (MX_HTTP_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_HTTP_SERVER structure." );
	}

	record->record_class_struct = NULL;
	record->record_type_struct = http_server;
	record->class_specific_function_list = NULL;

	http_server->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_http_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_http_server_open()";

	MX_HTTP_SERVER *http_server = NULL;
	MX_RECORD *rs232_record = NULL;
	size_t num_bytes_read, length;
	char response[80];
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	http_server = (MX_HTTP_SERVER *) record->record_type_struct;

	if ( http_server == (MX_HTTP_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_HTTP_SERVER pointer for record '%s' is NULL.",
			record->name );
	}

	rs232_record = http_server->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for HTTP server '%s' is NULL.",
			record->name );
	}

	/* FIXME: Make sure that the RS232 interface is configured correctly? */


	/* We should have already received a HTTP or ASCII MX server message. */

	mx_status = mx_rs232_getline_with_timeout( rs232_record,
					response, sizeof(response),
					&num_bytes_read,
					MXN_HTTP_DEBUG, 5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: num_bytes_read = %ld", fname, num_bytes_read ));

	length = strlen( response );

	MX_DEBUG(-2,("%s: response = '%s'", fname, response));

	return mx_status;
}

