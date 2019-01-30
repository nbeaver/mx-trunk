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

#define LIBCURL_MODULE_DEBUG_INITIALIZE	FALSE

#define LIBCURL_MODULE_DEBUG_CALL	FALSE

#define LIBCURL_MODULE_DEBUG_CREATE	FALSE

#define LIBCURL_MODULE_DEBUG_HTTP_GET	TRUE

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

#if LIBCURL_MODULE_DEBUG_INITIALIZE
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

#if LIBCURL_MODULE_DEBUG_INITIALIZE
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

#if LIBCURL_MODULE_DEBUG_CALL
		MX_DEBUG(-2,("%s: libcurl_private->http = %p",
			fname, libcurl_private->http));
#endif
		break;

	case MXRC_HTTP_GET_FUNCTION_LIST:
		/* Return the MX_HTTP_FUNCTION_LIST for our caller. */

#if LIBCURL_MODULE_DEBUG_CALL
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

#if LIBCURL_MODULE_DEBUG_CREATE
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

#if LIBCURL_MODULE_DEBUG_CREATE
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

	/* Setup the default Curl options. */

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

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_libcurl_http_get( MX_HTTP *http, char *url,
			unsigned long *http_status_code,
			char **received_data_ptr, size_t *received_length )
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

#if LIBCURL_MODULE_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s invoked for URL '%s'.", fname, url));
#endif

	curl_handle = libcurl_private->curl_handle;

	if ( curl_handle == (CURL *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No CURL handle has been acquired for HTTP extension '%s'.",
		http->http_extension->name );
	}

	/* Tell libcurl the URL to read from. */

	curl_status = curl_easy_setopt( curl_handle,
					CURLOPT_URL, url );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

#if LIBCURL_MODULE_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s: Now performing the request.", fname));
#endif

	/* Tell libcurl to actually perform the request. */

	curl_status = curl_easy_perform( curl_handle );

	if ( curl_status != CURLE_OK ) {
		return mxext_libcurl_set_mx_status( fname, curl_status,
							libcurl_private );
	}

#if LIBCURL_MODULE_DEBUG_HTTP_GET
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

