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

#define MXN_HTTP_DEBUG			FALSE

#define MXN_HTTP_DEBUG_GET		FALSE
#define MXN_HTTP_DEBUG_PUT		TRUE

#define MXN_HTTP_DEBUG_GET_DETAILS	FALSE
#define MXN_HTTP_DEBUG_PUT_DETAILS	TRUE

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
	size_t num_bytes_to_write, num_bytes_written;
	size_t num_bytes_to_read, num_bytes_read;
	int num_items;
	double wait_timeout;
	char *ptr, *blank_ptr, *body_ptr;
	char *content_type_ptr, *end_of_string_ptr;
	mx_status_type mx_status;

#if MXN_HTTP_DEBUG_GET
	MX_DEBUG(-2,("******** %s invoked ********", fname));
#endif

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

#if MXN_HTTP_DEBUG_GET_DETAILS
	MX_DEBUG(-2,("%s: http_message = '%s'", fname, http_message));
#endif

	num_bytes_to_write = strlen(http_message);

#if MXN_HTTP_DEBUG_GET_DETAILS
	MX_DEBUG(-2,("%s: num_bytes_to_write = %ld",
				fname, (long) num_bytes_to_write ));
#endif

	mx_status = mx_rs232_write( rs232_record,
				http_message,
				num_bytes_to_write,
				&num_bytes_written,
				MXN_HTTP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXN_HTTP_DEBUG_GET_DETAILS
	MX_DEBUG(-2,("%s: num_bytes_written = %ld",
			fname, (long) num_bytes_written));
#endif

	/* Wait a while for a response to arrive. */

	wait_timeout = 5.0;	/* in seconds */

	mx_status = mx_rs232_wait_for_input_available( rs232_record,
						&num_input_bytes_available,
						wait_timeout );

	if ( mx_status.code == MXE_SUCCESS ) {
		/* A response has arrived. */
	} else
	if ( mx_status.code == MXE_TIMED_OUT ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for a response "
		"from the HTTP server to a GET for '%s'.",
			wait_timeout, url );
	} else {
		return mx_status;
	}

	/* Read the response. */

	while ( TRUE ) {

		mx_status = mx_rs232_num_input_bytes_available( rs232_record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXN_HTTP_DEBUG_GET_DETAILS
		MX_DEBUG(-2,("%s: num_input_bytes_available = %ld",
			fname, num_input_bytes_available));
#endif

		if ( num_input_bytes_available == 0 ) {
			break;
		}

		if ( num_input_bytes_available > sizeof(local_response) ) {
			num_bytes_to_read = sizeof(local_response);
		} else {
			num_bytes_to_read = num_input_bytes_available;
		}

#if MXN_HTTP_DEBUG_GET_DETAILS
		MX_DEBUG(-2,("%s: num_bytes_to_read = %ld",
			fname, (long) num_bytes_to_read));
#endif

		mx_status = mx_rs232_read( rs232_record,
					local_response,
					num_bytes_to_read,
					&num_bytes_read,
					MXN_HTTP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXN_HTTP_DEBUG_GET_DETAILS
		MX_DEBUG(-2,("%s: num_bytes_read = %ld",
			fname, (long) num_bytes_read));
#endif

#if MXN_HTTP_DEBUG_GET_DETAILS
		MX_DEBUG(-2,("%s: local_response = '%s'",
			fname, local_response));
#endif

		/* FIXME: The parsing of the HTTP server's response
		 * is inefficient and crufty.
		 */

		/* The end of the HTTP header is a blank line. */

		blank_ptr = strstr( local_response, "\r\n\r\n" );

		if ( blank_ptr == (char *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"Did not find the end of the HTTP header in "
			"response '%s'.", response );
		}

		body_ptr = blank_ptr + strlen("\r\n\r\n");

		strlcpy( response, body_ptr, max_response_length );

		/* Get the HTTP status code that was sent to us. */

		if ( url_status_code != (unsigned long *) NULL ) {
			num_items = sscanf( local_response,
					"HTTP/1.1 %lu", url_status_code );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Could not find the HTTP status code in the "
				"response '%s' from the HTTP server to the "
				"url '%s'.", local_response, url );
			}

#if MXN_HTTP_DEBUG_GET_DETAILS
			MX_DEBUG(-2,("%s: HTTP status code = %lu",
				fname, *url_status_code ));
#endif
		}

		/* Get the content type. */

		ptr = strcasestr( local_response, "Content-Type: " );

		if ( ptr == (char *) NULL ) {
			mx_warning( "No Content-Type: seen in response '%s'.",
				local_response );
		} else {
			ptr = strchr( ptr, ' ' );

			content_type_ptr = ptr + 1;

			end_of_string_ptr = strchr( ptr, '\r' );

			if ( end_of_string_ptr != NULL ) {
				*end_of_string_ptr = '\0';
			}

			strlcpy( content_type, content_type_ptr,
					max_content_type_length );

#if MXN_HTTP_DEBUG_GET_DETAILS
			MX_DEBUG(-2,("%s: content type = '%s'",
				fname, content_type));
#endif
		}

	}

#if MXN_HTTP_DEBUG_GET
	MX_DEBUG(-2,("******** %s complete ********", fname));
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

	char protocol[40];
	char hostname[MXU_HOSTNAME_LENGTH+1];
	char filename[MXU_FILENAME_LENGTH+1];

	char http_message[5000];
	unsigned long num_input_bytes_available;
	char local_response[500];
	size_t num_bytes_to_write, num_bytes_written;
	size_t num_bytes_to_read, num_bytes_read;
	int num_items;
	double wait_timeout;
	mx_status_type mx_status;

#if MXN_HTTP_DEBUG_PUT
	MX_DEBUG(-2,("******** %s invoked ********", fname));
#endif

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

#if MXN_HTTP_DEBUG_PUT
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

#if MXN_HTTP_DEBUG_PUT
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

	/* Send the PUT header to the Web server. */

	snprintf( http_message, sizeof(http_message),
		"PUT %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Content-Type: %s\r\n"
		"\r\n"
		"%s\r\n",
		filename, hostname, content_type, sent_data );

#if MXN_HTTP_DEBUG_PUT_DETAILS
	MX_DEBUG(-2,("%s: http_message = '%s'", fname, http_message));
#endif

	num_bytes_to_write = strlen(http_message);

#if MXN_HTTP_DEBUG_PUT_DETAILS
	MX_DEBUG(-2,("%s: num_bytes_to_write = %ld",
				fname, (long) num_bytes_to_write ));
#endif

	mx_status = mx_rs232_write( rs232_record,
				http_message,
				num_bytes_to_write,
				&num_bytes_written,
				MXN_HTTP_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXN_HTTP_DEBUG_PUT_DETAILS
	MX_DEBUG(-2,("%s: num_bytes_written = %ld",
			fname, (long) num_bytes_written));
#endif

	/* Wait a while for a response to arrive. */

	wait_timeout = 5.0;	/* in seconds */

	mx_status = mx_rs232_wait_for_input_available( rs232_record,
						&num_input_bytes_available,
						wait_timeout );

	if ( mx_status.code == MXE_SUCCESS ) {
		/* A response has arrived. */
	} else
	if ( mx_status.code == MXE_TIMED_OUT ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for a response "
		"from the HTTP server to a PUT for '%s'.",
			wait_timeout, url );
	} else {
		return mx_status;
	}

	/* Read the response. */

	while ( TRUE ) {

		mx_status = mx_rs232_num_input_bytes_available( rs232_record,
						&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXN_HTTP_DEBUG_PUT_DETAILS
		MX_DEBUG(-2,("%s: num_input_bytes_available = %ld",
			fname, num_input_bytes_available));
#endif

		if ( num_input_bytes_available == 0 ) {
			break;
		}

		if ( num_input_bytes_available > sizeof(local_response) ) {
			num_bytes_to_read = sizeof(local_response);
		} else {
			num_bytes_to_read = num_input_bytes_available;
		}

#if MXN_HTTP_DEBUG_PUT_DETAILS
		MX_DEBUG(-2,("%s: num_bytes_to_read = %ld",
			fname, (long) num_bytes_to_read));
#endif

		mx_status = mx_rs232_read( rs232_record,
					local_response,
					num_bytes_to_read,
					&num_bytes_read,
					MXN_HTTP_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXN_HTTP_DEBUG_PUT_DETAILS
		MX_DEBUG(-2,("%s: num_bytes_read = %ld",
			fname, (long) num_bytes_read));
#endif

#if MXN_HTTP_DEBUG_PUT_DETAILS
		MX_DEBUG(-2,("%s: local_response = '%s'",
			fname, local_response));
#endif

		/* Get the HTTP status code that was sent to us. */

		if ( url_status_code != (unsigned long *) NULL ) {
			num_items = sscanf( local_response,
					"HTTP/1.1 %lu", url_status_code );

			if ( num_items != 1 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"Could not find the HTTP status code in the "
				"response '%s' from the HTTP server to the "
				"url '%s'.", local_response, url );
			}

#if MXN_HTTP_DEBUG_PUT_DETAILS
			MX_DEBUG(-2,("%s: HTTP status code = %lu",
				fname, *url_status_code ));
#endif
		}
	}

#if MXN_HTTP_DEBUG_PUT
	MX_DEBUG(-2,("******** %s complete ********", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

