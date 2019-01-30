/*
 * Name:    libcurl.c
 *
 * Purpose: Module wrapper for the libcurl library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <curl/curl.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_module.h"
#include "mx_http.h"
#include "e_libcurl.h"

MX_EXTENSION mxext_libcurl_extension_table[] = {

{ "libcurl", &mxext_libcurl_extension_function_list },

{"", 0}
};

/*----*/

static mx_bool_type
libcurl_driver_init( MX_MODULE *module )
{
#if 0
	static const char fname[] = "libcurl_driver_init()";

	MX_DEBUG(-2,("%s invoked for module '%s'", fname, module->name));
#endif

	return TRUE;
}

/*----*/

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"libcurl",
	MX_VERSION,
	NULL,
	mxext_libcurl_extension_table
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT_libcurl__ = libcurl_driver_init;

