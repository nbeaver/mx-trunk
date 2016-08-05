/*
 * Name:    cbflib.c
 *
 * Purpose: Module wrapper for the cbflib library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "cbf.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_module.h"
#include "e_cbflib.h"

MX_EXTENSION mxext_cbflib_extension_table[] = {

{ "cbflib", &mxext_cbflib_extension_function_list },

{"", 0}
};

/*----*/

MX_EXPORT
MX_IMAGE_FUNCTION_LIST mxext_cbflib_image_function_list = {
	mxext_cbflib_read_cbf_file,
	mxext_cbflib_write_cbf_file
};

/*----*/

static mx_bool_type
cbflib_driver_init( MX_MODULE *module )
{
#if 1
	static const char fname[] = "cbflib_driver_init()";

	MX_DEBUG(-2,("%s invoked for module '%s'", fname, module->name));

	MX_DEBUG(-2,("WARNING! This module is not complete.  It may reformat "
		"your dog with cat DNA.  You have been warned." ));
#endif

	return TRUE;
}

/*----*/

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"cbflib",
	MX_VERSION,
	NULL,
	mxext_cbflib_extension_table
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT_cbflib__ = cbflib_driver_init;

