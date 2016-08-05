/*
 * Name:    e_cbflib.c
 *
 * Purpose: MX CBFlib extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define CBFLIB_MODULE_DEBUG_INITIALIZE	FALSE

#define CBFLIB_MODULE_DEBUG_FINALIZE	FALSE

#define CBFLIB_MODULE_DEBUG_READ	FALSE

#define CBFLIB_MODULE_DEBUG_WRITE	FALSE

#include <stdio.h>
#include <stdlib.h>
#if 0
#include <stdarg.h>
#endif

/* Include file from CBFlib. */
#include "cbf.h"

#include "mx_util.h"
#include "mx_record.h"
#if 0
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
#endif
#include "e_cbflib.h"

MX_EXTENSION_FUNCTION_LIST mxext_cbflib_extension_function_list = {
	mxext_cbflib_initialize,
};

#if defined(OS_WIN32)
#  define MXP_CBFLIB_LIBRARY_NAME	"cbflib.dll"	/* FIXME ? */
#else
#  define MXP_CBFLIB_LIBRARY_NAME	"libcbf.so.0"
#endif

/*------*/

MX_EXPORT mx_status_type
mxext_cbflib_initialize( MX_EXTENSION *extension )
{
	static const char fname[] = "mxext_cbflib_initialize()";

	MX_CBFLIB_EXTENSION_PRIVATE *cbflib_ext;
	MX_DYNAMIC_LIBRARY *cbflib_library;
	mx_status_type mx_status;

	cbflib_ext = (MX_CBFLIB_EXTENSION_PRIVATE *)
			malloc( sizeof(MX_CBFLIB_EXTENSION_PRIVATE) );

	if ( cbflib_ext == (MX_CBFLIB_EXTENSION_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_CBFLIB_EXTENSION_PRIVATE structure." );
	}

	extension->ext_private = cbflib_ext;

	/* Find and save a copy of the MX_DYNAMIC_LIBRARY pointer for
	 * the cbflib library where other MX routines can find it.
	 * Then, we can use mx_dynamic_library_get_symbol_pointer()
	 * to look for individual cbflib library functions later.
	 */

	mx_status = mx_dynamic_library_open( MXP_CBFLIB_LIBRARY_NAME,
						&cbflib_library, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if CBFLIB_MODULE_DEBUG_INITIALIZE
	MX_DEBUG(-2,("%s: cbflib library name = '%s', cbflib_library = %p",
		fname, MXP_CBFLIB_LIBRARY_NAME, cbflib_library));
#endif

	cbflib_ext->cbflib_library = cbflib_library;

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_cbflib_read_tiff_file( MX_IMAGE_FRAME **image_frame,
				char *datafile_name )
{
	static const char fname[] = "mxext_cbflib_read_tiff_file()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Not yet implemented." );
}

/*------*/

MX_EXPORT mx_status_type
mxext_cbflib_write_tiff_file( MX_IMAGE_FRAME *frame,
				char *datafile_name )
{
	static const char fname[] = "mxext_cbflib_write_tiff_file()";

	MX_DEBUG(-2,("%s invoked for datafile '%s'.", fname, datafile_name));

	return MX_SUCCESSFUL_RESULT;
}

