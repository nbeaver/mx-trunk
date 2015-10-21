/*
 * Name:    libtiff.c
 *
 * Purpose: Module wrapper for the libtiff library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tiffio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_module.h"
#include "e_libtiff.h"

MX_EXTENSION mxext_libtiff_extension_table[] = {

{ "libtiff", &mxext_libtiff_extension_function_list },

{"", 0}
};

/*----*/

static mx_bool_type
libtiff_driver_init( MX_MODULE *module )
{
#if 1
	static const char fname[] = "libtiff_driver_init()";

	MX_DEBUG(-2,("%s invoked for module '%s'", fname, module->name));
#endif

	return TRUE;
}

/*----*/

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"libtiff",
	MX_VERSION,
	NULL,
	mxext_libtiff_extension_table
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT_libtiff__ = libtiff_driver_init;

