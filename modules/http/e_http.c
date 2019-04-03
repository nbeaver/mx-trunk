/*
 * Name:    e_http.c
 *
 * Purpose: A basic MX HTTP extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_HTTP_DEBUG_INITIALIZE	TRUE
#define MX_HTTP_DEBUG_CALL		TRUE
#define MX_HTTP_DEBUG_CREATE		TRUE
#define MX_HTTP_DEBUG_PARSE_ADDRESS	FALSE
#define MX_HTTP_DEBUG_HTTP_GET		TRUE
#define MX_HTTP_DEBUG_HTTP_PUT		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_module.h"
#include "mx_rs232.h"
#include "mx_http.h"
#include "e_http.h"

MX_EXTENSION_FUNCTION_LIST mxext_http_extension_function_list = {
	mxext_http_initialize,
	NULL,
	mxext_http_call
};

/*------*/

MX_HTTP_FUNCTION_LIST mxext_http_http_function_list = {
	mxext_http_create,
	NULL,
	NULL,
	mxext_http_http_get,
	NULL,
	NULL,
	mxext_http_http_put
};

/*------*/

static mx_status_type
mxext_http_get_pointers( MX_HTTP *http,
			MX_EXTENSION **http_extension,
			MX_HTTP_EXTENSION_PRIVATE **http_private,
	       		const char *calling_fname )
{
	static const char fname[] = "mxext_http_get_pointers()";

	MX_EXTENSION *http_extension_ptr = NULL;

	if ( http == (MX_HTTP *) NULL ) {
		if ( calling_fname == (char *) NULL ) {
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_HTTP pointer passed was NULL." );
		} else {
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_HTTP pointer passed by '%s' was NULL.",
				calling_fname );
		}
	}

	http_extension_ptr = http->http_extension;

	if ( http_extension_ptr == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EXTENSION pointer for MX_HTTP pointer %p is NULL.",
			http );
	}

	if ( http_extension != (MX_EXTENSION **) NULL ) {
		*http_extension = http_extension_ptr;
	}

	if ( http_private != (MX_HTTP_EXTENSION_PRIVATE **) NULL ) {
		*http_private = http_extension_ptr->ext_private;

		if ( (*http_private) == (MX_HTTP_EXTENSION_PRIVATE *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_HTTP_EXTENSION_PRIVATE pointer for "
			"extension '%s' is NULL.",
				http_extension_ptr->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*========================================================================*/

MX_EXPORT mx_status_type
mxext_http_initialize( MX_EXTENSION *extension )
{
	static const char fname[] = "mxext_http_initialize()";

	MX_HTTP_EXTENSION_PRIVATE *http_private = NULL;

	if ( extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTENSION pointer passed was NULL." );
	}

#if MX_HTTP_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s invoked for extension '%s'.", fname, extension->name));
#endif

	http_private = (MX_HTTP_EXTENSION_PRIVATE *)
			malloc( sizeof(MX_HTTP_EXTENSION_PRIVATE) );

	if ( http_private == (MX_HTTP_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_HTTP_EXTENSION_PRIVATE structure." );
	}

	http_private->http_function_list = &mxext_http_http_function_list;
	http_private->extension = extension;

	extension->ext_private = http_private;

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_http_call( MX_EXTENSION *extension,
		int request_code,
		int argc, void **argv )
{
	static const char fname[] = "mxext_http_call()";

	MX_HTTP_EXTENSION_PRIVATE *http_private = NULL;

	if ( extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTENSION pointer passed was NULL." );
	}
	if ( argc < 1 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"argc (%d) must be >= 1 for 'http' extension", argc );
	}
	if ( argv == (void **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The argv pointer passed for 'http' is NULL." );
	}

	/* Having validated all the arguments, now let us find the 
	 * MX_HTTP_FUNCTION_LIST.
	 */

	http_private = (MX_HTTP_EXTENSION_PRIVATE *)
				extension->ext_private;

	if ( http_private == (MX_HTTP_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HTTP_EXTENSION_PRIVATE pointer for "
		"extension '%s' is NULL.", extension->name );
	}

	/* Implement the various requests that the caller can make. */

	switch( request_code ) {
	case MXRC_HTTP_SET_HTTP_POINTER:
		/* Save a copy of the MX_HTTP pointer for our future use. */

		http_private->http = (MX_HTTP *) argv[0];

#if MX_HTTP_DEBUG_CALL
		MX_DEBUG(-2,("%s: http_private->http = %p",
			fname, http_private->http));
#endif
		break;

	case MXRC_HTTP_GET_FUNCTION_LIST:
		/* Return the MX_HTTP_FUNCTION_LIST for our caller. */

#if MX_HTTP_DEBUG_CALL
		MX_DEBUG(-2,("%s: http_private->http_function_list = %p",
			fname, http_private->http_function_list));
#endif

		argv[0] = (void *) http_private->http_function_list;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported request code (%d) passed for extension '%s'.",
			request_code, extension->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_http_create( MX_HTTP *http )
{
	static const char fname[] = "mxext_http_create()";

	MX_EXTENSION *http_extension = NULL;
	MX_HTTP_EXTENSION_PRIVATE *http_private = NULL;
	mx_status_type mx_status;

	mx_status = mxext_http_get_pointers( http,
					&http_extension, &http_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_HTTP_DEBUG_CREATE
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

#if 1
	http->http_debug = TRUE;
#endif

	http_private->rs232_record = NULL;

#if MX_HTTP_DEBUG_CREATE
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

/* FIXME? This parsing routine is not very general. */

static mx_status_type
mxext_http_parse_address( char *url,
			char *protocol,
			size_t max_protocol_length,
			char *hostname,
			size_t max_hostname_length,
			char *filename,
			size_t max_filename_length )
{
	static const char fname[] = "mxext_http_parse_address()";

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
mxext_http_http_get( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char *content_type, size_t max_content_type_length,
			char *response, size_t max_response_length )
{
	static const char fname[] = "mxext_http_http_get()";

	MX_EXTENSION *http_extension = NULL;
	MX_HTTP_EXTENSION_PRIVATE *http_private = NULL;
	MX_RECORD *rs232_record = NULL;
	char protocol[40];
	char hostname[MXU_HOSTNAME_LENGTH+1];
	char filename[MXU_FILENAME_LENGTH+1];
	char http_message[500];
	unsigned long num_input_bytes_available;
	char local_response[500];
	mx_status_type mx_status;

	mx_status = mxext_http_get_pointers( http,
				&http_extension, &http_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_HTTP_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s invoked for URL '%s'.", fname, url));
#endif

	if ( http_status_code != (unsigned long *) NULL ) {
		*http_status_code = 0;
	}

	mx_status = mxext_http_parse_address( url,
			protocol, sizeof(protocol),
			hostname, sizeof(hostname),
			filename, sizeof(filename) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_HTTP_DEBUG_HTTP_GET
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

	mx_status = mx_rs232_putline( rs232_record, http_message,
					NULL, http->http_debug );

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
					NULL, http->http_debug );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG(-2,("%s: local_response = '%s'",
			fname, local_response));
	}

#if MX_HTTP_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_http_http_put( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char *content_type,
			char *sent_data, ssize_t sent_data_length )
{
	static const char fname[] = "mxext_http_http_put()";

	MX_EXTENSION *http_extension = NULL;
	MX_HTTP_EXTENSION_PRIVATE *http_private = NULL;
	mx_status_type mx_status;

	mx_status = mxext_http_get_pointers( http,
					&http_extension, &http_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sent_data_length < 0 ) {
		sent_data_length = strlen(sent_data);
	}

#if MX_HTTP_DEBUG_HTTP_PUT
	MX_DEBUG(-2,("%s invoked for URL '%s', sent_data = '%s'.",
			fname, url, sent_data));
#endif

#if MX_HTTP_DEBUG_HTTP_PUT
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

