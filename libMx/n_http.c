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

#define MXN_HTTP_DEBUG_GET	TRUE
#define MXN_HTTP_DEBUG_PUT	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_url.h"
#include "n_http.h"

MX_RECORD_FUNCTION_LIST mxn_http_server_record_function_list = {
	NULL,
	mxn_http_server_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxn_http_server_open
};

MX_URL_FUNCTION_LIST mxn_http_server_url_function_list = {
	NULL,
	mxn_http_server_get,
	NULL,
	NULL,
	mxn_http_server_put
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

/*---*/

static mx_status_type
mxn_http_server_get_pointers( MX_URL_SERVER *url_server,
				MX_HTTP_SERVER **http_server,
				const char *calling_fname )
{
	static const char fname[] = "mxn_http_server_get_pointers()";

	if ( url_server == (MX_URL_SERVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_URL_SERVER pointer passed was NULL." );
	}

	if ( url_server->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_URL_SERVER %p is NULL.",
			url_server );
	}

	if ( http_server == (MX_HTTP_SERVER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HTTP_SERVER pointer passed was NULL." );
	}

	*http_server = (MX_HTTP_SERVER *)
				url_server->record->record_type_struct;

	if ( (*http_server) == (MX_HTTP_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HTTP_SERVER pointer for record '%s' is NULL.",
			url_server->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_http_server_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxn_http_server_create_record_structures()";

	MX_URL_SERVER *url_server = NULL;
	MX_HTTP_SERVER *http_server = NULL;

	url_server = (MX_URL_SERVER *) malloc( sizeof(MX_URL_SERVER) );

	if ( url_server == (MX_URL_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_URL_SERVER structure." );
	}

	http_server = (MX_HTTP_SERVER *) malloc( sizeof(MX_HTTP_SERVER) );

	if ( http_server == (MX_HTTP_SERVER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_HTTP_SERVER structure." );
	}

	record->record_class_struct = url_server;
	record->record_type_struct = http_server;

	record->record_function_list = &mxn_http_server_record_function_list;

	record->class_specific_function_list =
		&mxn_http_server_url_function_list;

	url_server->record = record;
	http_server->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxn_http_server_open( MX_RECORD *record )
{
	static const char fname[] = "mxn_http_server_open()";

	MX_URL_SERVER *url_server = NULL;
	MX_HTTP_SERVER *http_server = NULL;
	MX_URL_FUNCTION_LIST *url_flist = NULL;
	mx_status_type mx_status;

	mx_status = mx_url_get_pointers( record,
				&url_server, &url_flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxn_http_server_get_pointers( url_server,
						&http_server, fname );

	return mx_status;
}

/*---*/

/* FIXME? This parsing routine is not very general. */

static mx_status_type
mxn_http_server_parse_address( char *url,
			char *protocol,
			size_t max_protocol_length,
			char *hostname,
			size_t max_hostname_length,
			char *filename,
			size_t max_filename_length )
{
	static const char fname[] = "mxn_http_server_parse_address()";

	char *protocol_ptr = NULL;
	char *hostname_ptr = NULL;
	char *filename_ptr = NULL;
	char *ptr = NULL;

	size_t protocol_length;
	size_t hostname_length;
	size_t filename_length;

	int i;

	if ( url == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The URL pointer passed was NULL." );
	}
	if ( protocol == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The protocol pointer passed was NULL." );
	}
	if ( hostname == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The hostname pointer passed was NULL." );
	}
	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

#if MX_HTTP_DEBUG_PARSE_ADDRESS
	MX_DEBUG(-2,("%s: url = '%s'", fname, url ));
#endif

	/* The protocol is at the beginning of the URL. */

	protocol_ptr = url;

	/* There should be a ':' just after the end of the protocol. */

	ptr = strchr( protocol_ptr, ':' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not see a protocol like 'http' at the beginning "
		"of the URL '%s'.", url );
	}

	protocol_length = (size_t)( ptr - protocol_ptr + 1 );

	if ( protocol_length > max_protocol_length ) {
		protocol_length = max_protocol_length;
	}

	strlcpy( protocol, protocol_ptr, protocol_length );

#if MX_HTTP_DEBUG_PARSE_ADDRESS
	MX_DEBUG(-2,("%s: protocol = '%s'", fname, protocol ));
#endif

	/* The ':' character should be followed by two '/' characters. */

	for ( i = 0; i < 2; i++ ) {
		ptr++;

		ptr = strchr( ptr, '/' );

		if ( ptr == (char *) NULL ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Did not find the characters '://' between the "
			"URL protocol and the URL hostname in the URL '%s'.",
				url );
		}
	}

	hostname_ptr = ptr + 1;

	/* The hostname should be terminated by the _first_ occurence of
	 * a '/' character after the '://' sequence.
	 */

	ptr = strchr( hostname_ptr, '/' );

	if ( ptr == (char *) NULL ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not find the '/' after the hostname in the URL '%s'.",
			url );
	}

	hostname_length = (size_t)( ptr - hostname_ptr + 1 );

	if ( hostname_length > max_hostname_length ) {
		hostname_length = max_hostname_length;
	}

	strlcpy( hostname, hostname_ptr, hostname_length );

#if MX_HTTP_DEBUG_PARSE_ADDRESS
	MX_DEBUG(-2,("%s: hostname = '%s'", fname, hostname ));
#endif

	/* The filename contains everything after the '/' character
	 * that terminates the hostname.
	 */

	filename_ptr = ptr + 1;

	filename_length = strlen( filename_ptr ) + 1;

	if ( filename_length > max_filename_length ) {
		filename_length = max_filename_length;
	}

	strlcpy( filename, filename_ptr, filename_length );

#if MX_HTTP_DEBUG_PARSE_ADDRESS
	MX_DEBUG(-2,("%s: filename = '%s'", fname, filename ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxn_http_server_get( MX_URL_SERVER *url_server,
		char *url, unsigned long *url_status_code,
		char *content_type, size_t max_content_type_length,
		char *response, size_t max_response_length )
{
	static const char fname[] = "mxn_http_server_get()";

	MX_HTTP_SERVER *http_server = NULL;
	MX_RECORD *rs232_record = NULL;
	char protocol[40];
	char hostname[MXU_HOSTNAME_LENGTH+1];
	char filename[MXU_FILENAME_LENGTH+1];
	char http_message[500];
	unsigned long num_input_bytes_available;
	char local_response[500];
	mx_status_type mx_status;

	mx_status = mxn_http_server_get_pointers( url_server,
						&http_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232_record = http_server->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for HTTP server record '%s' "
		"is NULL.", url_server->record->name );
	}

#if MXN_HTTP_DEBUG_GET
	MX_DEBUG(-2,("%s invoked for URL '%s'.", fname, url));
#endif

	if ( url_status_code != (unsigned long *) NULL ) {
		*url_status_code = 0;
	}

	mx_status = mxn_http_server_parse_address( url,
				protocol, sizeof(protocol),
				hostname, sizeof(hostname),
				filename, sizeof(filename) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXN_HTTP_DEBUG_GET
	MX_DEBUG(-2,("%s: protocol = '%s', hostname = '%s', filename = '%s'.",
		fname, protocol, hostname, filename ));
#endif

	if ( strcmp( protocol, "http" ) != 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The MX 'http.mxo' module does not support the use of "
		"the '%s' protocol requested by URL '%s'.  "
		"Only the 'http' protocol is supported.",
			protocol, url );
	}

	/* Send the initial query to the Web server. */

	snprintf( http_message, sizeof(http_message),
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Accept: */*\r\n"
		"\r\n",
		filename, hostname );

	MX_DEBUG(-2,("%s: http_message = '%s'", fname, http_message));

	mx_status = mx_rs232_putline( rs232_record,
				http_message, NULL, MXN_HTTP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response. */

	mx_msleep(1000);

	while ( TRUE ) {

		mx_status = mx_rs232_num_input_bytes_available( rs232_record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG(-2,("%s: num_input_bytes_available = %ld",
			fname, num_input_bytes_available));

#if 1
		if ( num_input_bytes_available == 0 ) {
			break;
		}
#endif

		mx_status = mx_rs232_getline( rs232_record,
					local_response,
					sizeof(local_response),
					NULL, MXN_HTTP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG(-2,("%s: local_response = '%s'",
			fname, local_response));
	}

#if MXN_HTTP_DEBUG_GET
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxn_http_server_put( MX_URL_SERVER *url_server,
		char *url, unsigned long *url_status_code,
		char *content_type,
		char *sent_data, ssize_t sent_data_length )
{
	static const char fname[] = "mxn_http_server_put()";

	MX_HTTP_SERVER *http_server = NULL;
	MX_RECORD *rs232_record = NULL;
	mx_status_type mx_status;

	mx_status = mxn_http_server_get_pointers( url_server,
						&http_server, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sent_data_length < 0 ) {
		sent_data_length = strlen(sent_data);
	}

	rs232_record = http_server->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for HTTP server record '%s' "
		"is NULL.", url_server->record->name );
	}

#if MXN_HTTP_DEBUG_PUT
	MX_DEBUG(-2,("%s invoked for URL '%s', sent_data = '%s'.",
			fname, url, sent_data));
#endif

#if MXN_HTTP_DEBUG_PUT
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

