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

#define LIBCURL_MODULE_DEBUG_INITIALIZE	TRUE

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
};


/*------*/

MX_EXPORT mx_status_type
mxext_libcurl_initialize( MX_EXTENSION *extension )
{
	static const char fname[] = "mxext_libcurl_initialize()";

	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_ext;
	MX_DYNAMIC_LIBRARY *libcurl_library;
	mx_status_type mx_status;

	if ( extension == (MX_EXTENSION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EXTENSION pointer passed was NULL." );
	}

#if LIBCURL_MODULE_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s invoked for extension '%s'.", fname, extension->name));
#endif

	libcurl_ext = (MX_LIBCURL_EXTENSION_PRIVATE *)
			malloc( sizeof(MX_LIBCURL_EXTENSION_PRIVATE) );

	if ( libcurl_ext == (MX_LIBCURL_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_LIBCURL_EXTENSION_PRIVATE structure." );
	}

	extension->ext_private = libcurl_ext;

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

	libcurl_ext->libcurl_library = libcurl_library;

	/* Put the MX_HTTP_FUNCTION_LIST somewhere that the module 
	 * will be able to find it.
	 */

	libcurl_ext->http_function_list = &mxext_libcurl_http_function_list;

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
	if ( request_code != MXRC_HTTP_GET_FUNCTION_LIST ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported request code (%d) passed for extension 'libcurl'.",
			request_code );
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

	/* Finally we return the MX_HTTP_FUNCTION_LIST for our caller. */

	MX_DEBUG(-2,("%s: libcurl_private->http_function_list = %p",
		fname, libcurl_private->http_function_list));

	argv[0] = (void *) libcurl_private->http_function_list;

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_libcurl_create( MX_HTTP *http )
{
	static const char fname[] = "mxext_libcurl_create()";

	MX_EXTENSION *libcurl_extension = NULL;
	MX_LIBCURL_EXTENSION_PRIVATE *libcurl_private = NULL;

	MX_DEBUG(-2,("%s invoked.", fname));

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

	/* Create an "easy" Curl session. */

	libcurl_private->curl_handle = curl_easy_init();

	if ( libcurl_private->curl_handle == (CURL *) NULL ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The attempt to get a CURL handle for "
		"HTTP extension '%s' failed.",
			libcurl_extension->name );
	}

	MX_DEBUG(-2,("%s: libcurl_private->curl_handle = %p",
		fname, libcurl_private->curl_handle));

	return MX_SUCCESSFUL_RESULT;
}

