/*
 * Name:    e_libcurl.c
 *
 * Purpose: MX libcurl extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* WARNING: This version of the MX 'libcurl' module uses callbacks 
 * via the Curl Easy Interface rather than the Curl Multi Interface.
 *
 * If we ever want to use multiple threads in this 'libcurl' module,
 * then we will need to rewrite things using the Multi Interface
 * and make sure that everything is thread safe.
 */

#define MX_LIBCURL_DEBUG_INITIALIZE	FALSE

#define MX_LIBCURL_DEBUG_CALL		FALSE

#define MX_LIBCURL_DEBUG_CREATE		FALSE

#define MX_LIBCURL_DEBUG_HEADER		FALSE

#define MX_LIBCURL_DEBUG_READ		TRUE

#define MX_LIBCURL_DEBUG_WRITE		TRUE

#define MX_LIBCURL_DEBUG_HTTP_GET	FALSE

#define MX_LIBCURL_DEBUG_HTTP_PUT	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* Include file for libcurl. */

#include "curl/curl.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_bit.h"
#include "mx_version.h"
#include "mx_time.h"
#include "mx_socket.h"
#include "mx_variable.h"
#include "mx_module.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_video_input.h"
#include "mx_http.h"
#include "e_libcurl.h"

/* FIXME: The following definition of strptime() should not be necessary. */

#if defined(OS_LINUX)
extern char *strptime( const char *, const char *, struct tm * );
#endif

MX_EXTENSION_FUNCTION_LIST mxext_libcurl_extension_function_list = {
	mxext_libcurl_initialize,
	NULL,
	mxext_libcurl_call
};

#if defined(OS_WIN32)
#  define MXP_LIBCURL_LIBRARY_NAME	"libcurl.dll"
#elif defined(OS_MACOSX)
#  define MXP_LIBCURL_LIBRARY_NAME	"libcurl.dylib"
#else
#  define MXP_LIBCURL_LIBRARY_NAME	"libcurl.so"
#endif

/*------*/

MX_HTTP_FUNCTION_LIST mxext_libcurl_http_function_list = {
	mxext_libcurl_create,
	NULL,
	NULL,
	mxext_libcurl_http_get,
	NULL,
	NULL,
	mxext_libcurl_http_put
};


/*------*/

static mx_status_type
mxext_libcurl_set_mx_status( const char *calling_fname,
				CURLcode curl_error_code,
				MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private )
{
	return mx_error( MXE_FUNCTION_FAILED, calling_fname,
		libcurl_private->curl_error_format,
		curl_error_code, curl_easy_strerror( curl_error_code ),
		libcurl_private->curl_error_buffer );
}

/*------*/

static size_t
mxext_libcurl_header_callback( char *buffer,
				size_t size,
				size_t num_items,
				void *userdata )
{
	static const char fname[] = "mxext_libcurl_header_callback()";

	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;
	static const char content_type_label[] = "content-type: ";
	char http_version_string[20];
	char http_version_format[30];
	char *type_ptr = NULL;
	char *end_ptr = NULL;
	unsigned long http_status_code;
	int sscanf_num_items;

	libcurl_private = (MX_LIBCURL_EXTENSION_PRIVATE *) userdata;

#if MX_LIBCURL_DEBUG_HEADER
	MX_DEBUG(-2,("%s invoked for %p.", fname, libcurl_private));
	MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif

	if ( libcurl_private->header_ptr != NULL ) {
		strlcat( libcurl_private->header_ptr, buffer,
			libcurl_private->max_header_length );
	}

	/* Get the HTTP version and response code. */

	if ( strncmp( buffer, "HTTP", 4 ) == 0 ) {
		snprintf( http_version_format, sizeof(http_version_format),
			"HTTP/%%%ds %%lu", (int) sizeof(http_version_string) );

		sscanf_num_items = sscanf( buffer, http_version_format,
				http_version_string, &http_status_code );

		if ( sscanf_num_items != 2 ) {
			(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The HTTP header line returned '%s' has an "
			"unrecognized format.", buffer );

			return 0;
		}

		libcurl_private->http_status_code = http_status_code;

#if MX_LIBCURL_DEBUG_HEADER
		MX_DEBUG(-2,("%s: libcurl_private->http_status_code = %lu",
			fname, libcurl_private->http_status_code));
#endif
	}

	/* Look for the content type, if wanted.  You should normally want it.*/

	if ( libcurl_private->content_type_ptr != NULL ) {
		if ( strncasecmp( buffer,
				content_type_label,
				sizeof(content_type_label)-1 ) == 0 )
		{
			type_ptr = buffer + sizeof(content_type_label) - 1;

			end_ptr = strchr( type_ptr, ' ' );

			if ( end_ptr != NULL ) {
			    *end_ptr = '\0';
			} else {
			    end_ptr = strchr( type_ptr, '\r' );

			    if ( end_ptr != NULL ) {
				*end_ptr = '\0';
			    } else {
				end_ptr = strchr( type_ptr, '\n' );

				if ( end_ptr != NULL ) {
				    *end_ptr = '\0';
				}
			    }
			}

			strlcpy( libcurl_private->content_type_ptr, type_ptr,
				libcurl_private->max_content_type_length );

#if MX_LIBCURL_DEBUG_HEADER
			MX_DEBUG(-2,
			("%s: libcurl_private->content_type_ptr = '%s'",
				fname, libcurl_private->content_type_ptr));
#endif
		}
	}

	return num_items;
}

/*------*/

static size_t
mxext_libcurl_read_callback( char *buffer,
				size_t size,
				size_t num_items,
				void *userdata )
{
	static const char fname[] = "mxext_libcurl_read_callback()";

	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;

	libcurl_private = (MX_LIBCURL_EXTENSION_PRIVATE *) userdata;

	MX_DEBUG(-2,("%s invoked for %p.", fname, libcurl_private));

	return 0;
}

/*------*/

static size_t
mxext_libcurl_write_callback( char *buffer,
				size_t size,
				size_t num_items,
				void *userdata )
{
#if MX_LIBCURL_DEBUG_WRITE
	static const char fname[] = "mxext_libcurl_write_callback()";
#endif

	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;
	size_t maximum_bytes, bytes_so_far, new_bytes_so_far, new_bytes;
	size_t num_bytes_received;

	libcurl_private = (MX_LIBCURL_EXTENSION_PRIVATE *) userdata;

	/* Try to make sure that the received buffer is NUL terminated. */

	num_bytes_received = size * num_items;

	if ( buffer[ num_bytes_received ] != '\0' ) {
		mx_warning(
		    "The received buffer was not null terminated. Fixing..." );

		/* NOTE: It is possible that the following line will cause
		 * a segmentation fault or access violation due to an attempt
		 * to access beyond the end of 'buffer'.  We are crossing
		 * our fingers and hoping that 'libcurl' itself allocates
		 * enough memory that this is not a problem.
		 */

		buffer[ num_bytes_received ] = '\0';
	}

#if MX_LIBCURL_DEBUG_WRITE
	MX_DEBUG(-2,("%s invoked for %p.", fname, libcurl_private));
	MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
	MX_DEBUG(-2,("%s: num_bytes_received = %lu",
			fname, num_bytes_received));
#endif

	maximum_bytes = libcurl_private->max_response_length;
	bytes_so_far = libcurl_private->data_bytes_received;

	new_bytes_so_far = strlcat( libcurl_private->response_ptr,
					buffer, maximum_bytes );

	if ( new_bytes_so_far >= maximum_bytes ) {
		new_bytes = maximum_bytes - bytes_so_far;

		libcurl_private->data_bytes_received = maximum_bytes;
	} else {
		new_bytes = new_bytes_so_far - bytes_so_far;

		libcurl_private->data_bytes_received = new_bytes_so_far;
	}

#if MX_LIBCURL_DEBUG_WRITE
	MX_DEBUG(-2,("%s: new_bytes = %ld", fname, (long) new_bytes));
#endif

	return new_bytes;
}

/*========================================================================*/

MX_EXPORT mx_status_type
mxext_libcurl_initialize( MX_EXTENSION *extension )
{
	static const char fname[] = "mxext_libcurl_initialize()";

	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;
	MX_DYNAMIC_LIBRARY *libcurl_library = NULL;
	mx_status_type mx_status;

	if ( extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTENSION pointer passed was NULL." );
	}

#if MX_LIBCURL_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s invoked for extension '%s'.", fname, extension->name));
#endif

	libcurl_private = (MX_LIBCURL_EXTENSION_PRIVATE *)
			malloc( sizeof(MX_LIBCURL_EXTENSION_PRIVATE) );

	if ( libcurl_private == (MX_LIBCURL_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_LIBCURL_EXTENSION_PRIVATE structure." );
	}

	libcurl_private->extension = extension;

	extension->ext_private = libcurl_private;

	/* Find and save a copy of the MX_DYNAMIC_LIBRARY pointer for
	 * the libcurl library where other MX routines can find it.
	 * Then, we can use mx_dynamic_library_get_symbol_pointer()
	 * to look for individual libcurl library functions later.
	 */

	mx_status = mx_dynamic_library_open( MXP_LIBCURL_LIBRARY_NAME,
						&libcurl_library, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_LIBCURL_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s: libcurl library name = '%s', libcurl_library = %p",
		fname, MXP_LIBCURL_LIBRARY_NAME, libcurl_library));
#endif

	libcurl_private->libcurl_library = libcurl_library;

	/* Put the MX_HTTP_FUNCTION_LIST somewhere that the module 
	 * will be able to find it.
	 */

	libcurl_private->http_function_list = &mxext_libcurl_http_function_list;

	/* WARNING: Do _NOT_ use the flags CURL_GLOBAL_WIN32 or CURL_GLOBAL_ALL
	 * below in curl_global_init()!  Platform-specific initialization has
	 * already been done in libMx by the function mx_socket_initialize().
	 * Doing socket initialization more than once can mess things up.
	 */

	curl_global_init( CURL_GLOBAL_SSL );

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_libcurl_call( MX_EXTENSION *extension,
		int request_code,
		int argc, void **argv )
{
	static const char fname[] = "mxext_libcurl_call()";

	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;

	if ( extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTENSION pointer passed was NULL." );
	}
	if ( argc < 1 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"argc (%d) must be >= 1 for 'libcurl' extension", argc );
	}
	if ( argv == (void **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The argv pointer passed for 'libcurl' is NULL." );
	}

	/* Having validated all the arguments, now let us find the 
	 * MX_HTTP_FUNCTION_LIST.
	 */

	libcurl_private = (MX_LIBCURL_EXTENSION_PRIVATE *)
				extension->ext_private;

	if ( libcurl_private == (MX_LIBCURL_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIBCURL_EXTENSION_PRIVATE pointer for "
		"extension '%s' is NULL.", extension->name );
	}

	/* Implement the various requests that the caller can make. */

	switch( request_code ) {
	case MXRC_HTTP_SET_HTTP_POINTER:
		/* Save a copy of the MX_HTTP pointer for our future use. */

		libcurl_private->http = (MX_HTTP *) argv[0];

#if MX_LIBCURL_DEBUG_CALL
		MX_DEBUG(-2,("%s: libcurl_private->http = %p",
			fname, libcurl_private->http));
#endif
		break;

	case MXRC_HTTP_GET_FUNCTION_LIST:
		/* Return the MX_HTTP_FUNCTION_LIST for our caller. */

#if MX_LIBCURL_DEBUG_CALL
		MX_DEBUG(-2,("%s: libcurl_private->http_function_list = %p",
			fname, libcurl_private->http_function_list));
#endif

		argv[0] = (void *) libcurl_private->http_function_list;
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
mxext_libcurl_create( MX_HTTP *http )
{
	static const char fname[] = "mxext_libcurl_create()";

	MX_EXTENSION *libcurl_extension = NULL;
	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;
	CURL *curl_handle = NULL;
	CURLcode curl_status;

#if MX_LIBCURL_DEBUG_CREATE
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( http == (MX_HTTP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HTTP pointer passed was NULL." );
	}

#if 1
	http->http_debug = TRUE;
#endif

	libcurl_extension = http->http_extension;

	if ( libcurl_extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EXTENSION pointer for MX_HTTP %p is NULL.", http );
	}

	libcurl_private = (MX_LIBCURL_EXTENSION_PRIVATE *)
				libcurl_extension->ext_private;

	if ( libcurl_private == (MX_LIBCURL_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ext_private pointer for MX extension '%s' is NULL.",
			libcurl_extension->name );
	}

	/* Before we start using Curl, we initialize curl_error_buffer
	 * and curl_error_format.
	 */

	libcurl_private->curl_error_buffer[0] = '\0';

	snprintf( libcurl_private->curl_error_format,
			MXU_MAXIMUM_CURL_FORMAT_LENGTH,
			"Curl error (%%ld) '%%40s'.  Error message = '%%%ds'",
			CURL_ERROR_SIZE );

#if 0
	MX_DEBUG(-2,("%s: curl_error_format = '%s'",
		fname, libcurl_private->curl_error_format ));
#endif

	/* Create an "easy" Curl session. */

	curl_handle = curl_easy_init();

	if ( curl_handle == (CURL *) NULL ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to get a CURL handle for "
		"HTTP extension '%s' failed.",
			libcurl_extension->name );
	}

	libcurl_private->curl_handle = curl_handle;

#if MX_LIBCURL_DEBUG_CREATE
	MX_DEBUG(-2,("%s: curl_handle = %p", fname, curl_handle));
#endif
	/* Arrange for libcurl errors to be written to our error buffer. */

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_ERRORBUFFER,
					libcurl_private->curl_error_buffer );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	/* Do we want debugging messages from libcurl? */

	if ( http->http_debug ) {
		curl_status = curl_easy_setopt( curl_handle,
						CURLOPT_VERBOSE, 1 );
		if ( curl_status != CURLE_OK ) {
		    return mxext_libcurl_set_mx_status( fname, curl_status,
							    libcurl_private );
		}
	}

	/* Tell libcurl that we want to follow redirection. */

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_FOLLOWLOCATION, 1L );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	/* Pass the libcurl_private pointer to callbacks. */

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_WRITEDATA,
					libcurl_private );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_READDATA,
					libcurl_private );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_HEADERDATA,
					libcurl_private );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	/* Set up the callbacks. */

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_HEADERFUNCTION,
					mxext_libcurl_header_callback );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_READFUNCTION,
					mxext_libcurl_read_callback );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_WRITEFUNCTION,
					mxext_libcurl_write_callback );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

#if MX_LIBCURL_DEBUG_CREATE
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_libcurl_http_get( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char *content_type, size_t max_content_type_length,
			char *response, size_t max_response_length )
{
	static const char fname[] = "mxext_libcurl_http_get()";

	MX_EXTENSION *libcurl_extension = NULL;
	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;
	CURL *curl_handle = NULL;
	CURLcode curl_status;

	if ( http == (MX_HTTP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HTTP pointer passed was NULL." );
	}

	libcurl_extension = http->http_extension;

	if ( libcurl_extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EXTENSION pointer for MX_HTTP %p is NULL.", http );
	}

	libcurl_private = (MX_LIBCURL_EXTENSION_PRIVATE *)
				libcurl_extension->ext_private;

	if ( libcurl_private == (MX_LIBCURL_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ext_private pointer for MX extension '%s' is NULL.",
			libcurl_extension->name );
	}

#if MX_LIBCURL_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s invoked for URL '%s'.", fname, url));
#endif

	curl_handle = libcurl_private->curl_handle;

	if ( curl_handle == (CURL *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No CURL handle has been acquired for HTTP extension '%s'.",
		http->http_extension->name );
	}

	MX_LIBCURL_INITIALIZE_CONTEXT( libcurl_private );

	libcurl_private->content_type_ptr = content_type;
	libcurl_private->content_type_ptr[0] = '\0';
	libcurl_private->max_content_type_length = max_content_type_length;

	libcurl_private->response_ptr = response;
	libcurl_private->response_ptr[0] = '\0';
	libcurl_private->max_response_length = max_response_length;

	/* Tell libcurl the URL to read from. */

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_URL, url );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	/* Tell libcurl that we want to do a GET. */

	curl_status = curl_easy_setopt( curl_handle, CURLOPT_PUT, 0 );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

#if MX_LIBCURL_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s: Now performing the request.", fname));
#endif

	/* Tell libcurl to actually perform the request. */

	curl_status = curl_easy_perform( curl_handle );

#if MX_LIBCURL_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s: libcurl_private->content_type_ptr = '%s'",
			fname, libcurl_private->content_type_ptr ));
#endif

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

#if MX_LIBCURL_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s: libcurl_private->response_ptr = '%s'",
			fname, libcurl_private->response_ptr ));
#endif

	if ( http_status_code != (unsigned long *) NULL ) {
		*http_status_code = libcurl_private->http_status_code;

#if MX_LIBCURL_DEBUG_HTTP_GET
		MX_DEBUG(-2,("%s: *http_status_code = %lu",
			fname, *http_status_code));
#endif
	}

#if MX_LIBCURL_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_libcurl_http_put( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char *content_type,
			char *sent_data, ssize_t sent_data_length )
{
	static const char fname[] = "mxext_libcurl_http_put()";

	MX_EXTENSION *libcurl_extension = NULL;
	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;
	CURL *curl_handle = NULL;
	CURLcode curl_status;

	if ( http == (MX_HTTP *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HTTP pointer passed was NULL." );
	}

	libcurl_extension = http->http_extension;

	if ( libcurl_extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EXTENSION pointer for MX_HTTP %p is NULL.", http );
	}

	libcurl_private = (MX_LIBCURL_EXTENSION_PRIVATE *)
				libcurl_extension->ext_private;

	if ( libcurl_private == (MX_LIBCURL_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The ext_private pointer for MX extension '%s' is NULL.",
			libcurl_extension->name );
	}

	if ( sent_data_length < 0 ) {
		sent_data_length = strlen(sent_data);
	}

#if MX_LIBCURL_DEBUG_HTTP_PUT
	MX_DEBUG(-2,("%s invoked for URL '%s', sent_data = '%s'.",
			fname, url, sent_data));
#endif

	curl_handle = libcurl_private->curl_handle;

	if ( curl_handle == (CURL *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No CURL handle has been acquired for HTTP extension '%s'.",
		http->http_extension->name );
	}

	MX_LIBCURL_INITIALIZE_CONTEXT( libcurl_private );

	libcurl_private->content_type_ptr = content_type;
	libcurl_private->max_content_type_length = strlen(content_type);

	libcurl_private->response_ptr = NULL;
	libcurl_private->max_response_length = 0;

	/* Tell libcurl the URL to write to. */

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_URL, url );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	/* Tell libcurl that we want to do a PUT. */

	curl_status = curl_easy_setopt( curl_handle, CURLOPT_PUT, 1 );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

	/* Give libcurl a pointer to the data we want to PUT. */

	libcurl_private->send_ptr = sent_data;
	libcurl_private->max_send_length = sent_data_length;

#if 0
	/* FIXME: Add "Expect: " header to the PUT. */

	curl_status = curl_easy_setopt( curl_handle,
			CURLOPT_HTTPHEADER, "Expect: " );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}
#endif

#if MX_LIBCURL_DEBUG_HTTP_PUT
	MX_DEBUG(-2,("%s: Now performing the request.", fname));
#endif

	/* Tell libcurl to actually perform the request. */

	curl_status = curl_easy_perform( curl_handle );

#if MX_LIBCURL_DEBUG_HTTP_PUT
	MX_DEBUG(-2,("%s: libcurl_private->content_type_ptr = '%s'",
			fname, libcurl_private->content_type_ptr ));
#endif

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

#if MX_LIBCURL_DEBUG_HTTP_PUT
	MX_DEBUG(-2,("%s: libcurl_private->response_ptr = '%s'",
			fname, libcurl_private->response_ptr ));
#endif

	if ( http_status_code != (unsigned long *) NULL ) {
		*http_status_code = libcurl_private->http_status_code;

#if MX_LIBCURL_DEBUG_HTTP_PUT
		MX_DEBUG(-2,("%s: *http_status_code = %lu",
			fname, *http_status_code));
#endif
	}

#if 0
	/* FIXME: Disable sending "Expect: " header to the PUT. */

	curl_status = curl_easy_setopt( curl_handle,
			CURLOPT_HTTPHEADER, NULL );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}
#endif

#if MX_LIBCURL_DEBUG_HTTP_PUT
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

