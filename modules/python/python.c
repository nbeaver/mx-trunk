/*
 * Name:    python.c
 *
 * Purpose: Module wrapper for the MX Python extension.
 *
 *          In other words, the 'python' extension is used by C code to call
 *          into Python, while the 'Mp' Python module is used by Python code
 *          to call into C.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
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
#include "e_python.h"

MX_EXTENSION mxext_python_extension_table[] = {

{ "python", &mxext_python_extension_function_list },

{"", 0}
};

/*----*/

static mx_bool_type
python_driver_init( MX_MODULE *module )
{
	static const char fname[] = "python_driver_init()";

	MX_DEBUG(-2,("%s invoked for module '%s'", fname, module->name));

	return TRUE;
}

/*----*/

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"python",
	MX_VERSION,
	NULL,
	mxext_python_extension_table
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT_python__ = python_driver_init;

