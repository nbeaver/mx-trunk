/*
 * Name:    e_libtiff.c
 *
 * Purpose: MX libtiff extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define LIBTIFF_MODULE_DEBUG_INITIALIZE	TRUE

#define LIBTIFF_MODULE_DEBUG_FINALIZE	TRUE

#include "tiffio.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_module.h"
#include "e_libtiff.h"

MX_EXTENSION_FUNCTION_LIST mxext_libtiff_extension_function_list = {
	mxext_libtiff_initialize,
	mxext_libtiff_finalize,
};

/*------*/

MX_EXPORT mx_status_type
mxext_libtiff_initialize( MX_EXTENSION *extension )
{
#if 0
	static const char fname[] = "mxext_libtiff_initialize()";
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxext_libtiff_finalize( MX_EXTENSION *extension )
{
#if 0
	static const char fname[] = "mxext_libtiff_finalize()";
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

