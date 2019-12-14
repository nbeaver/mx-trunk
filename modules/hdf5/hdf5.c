/*
 * Name:    hdf5.c
 *
 * Purpose: Module wrapper for the hdf5 library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "hdf5.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_module.h"
#include "e_hdf5.h"

MX_EXTENSION mxext_hdf5_extension_table[] = {

{ "hdf5", &mxext_hdf5_extension_function_list },

{"", 0}
};

/*----*/

MX_EXPORT
MX_IMAGE_FUNCTION_LIST mxext_hdf5_image_function_list = {
	NULL
};

/*----*/

static mx_bool_type
hdf5_driver_init( MX_MODULE *module )
{
#if 1
	static const char fname[] = "hdf5_driver_init()";

	MX_DEBUG(-2,("%s invoked for module '%s'", fname, module->name));
#endif

	return TRUE;
}

/*----*/

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"hdf5",
	MX_VERSION,
	NULL,
	mxext_hdf5_extension_table
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT_hdf5__ = hdf5_driver_init;

