/*
 * Name:    http.c
 *
 * Purpose: Module wrapper for the MX basic HTTP library.
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

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_module.h"
#include "mx_socket.h"
#include "mx_http.h"
#include "e_http.h"
#include "n_http.h"

MX_DRIVER mxext_http_driver_table[] = {

{"http_server", -1, MXN_HTTP, MXR_SERVER,
		&mxn_http_server_record_function_list,
		NULL,
		NULL,
		&mxn_http_server_num_record_fields,
		&mxn_http_server_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXTENSION mxext_http_extension_table[] = {

{ "http", &mxext_http_extension_function_list },

{"", 0}
};

/*----*/

static mx_bool_type
http_driver_init( MX_MODULE *module )
{
#if 1
	static const char fname[] = "http_driver_init()";

	MX_DEBUG(-2,("%s invoked for module '%s'", fname, module->name));
#endif

	return TRUE;
}

/*----*/

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"http",
	MX_VERSION,
	mxext_http_driver_table,
	mxext_http_extension_table
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT_http__ = http_driver_init;

