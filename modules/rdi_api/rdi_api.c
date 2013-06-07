/*
 * Name: rdi_api.c
 *
 * Purpose: Module wrapper for RDI's preferred area detector API.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
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
#include "z_rdi_api.h"

MX_DRIVER rdi_api_driver_table[] = {

{"rdi_api", -1,	-1, MXR_SPECIAL,
				&mxz_rdi_api_record_function_list,
				NULL,
				NULL,
				&mxz_rdi_api_num_record_fields,
				&mxz_rdi_api_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"rdi_api",
	MX_VERSION,
	rdi_api_driver_table,
	NULL,
	NULL
};

