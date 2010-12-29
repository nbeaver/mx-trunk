/*
 * Name: bnc725_lib.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 C++ libraries
 *          for the BNC725 digital delay generator.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
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
#include "i_bnc725_lib.h"

MX_DRIVER bnc725_lib_driver_table[] = {

{"bnc725_lib",	-1,	MXI_CONTROLLER,	MXR_INTERFACE,
				&mxi_bnc725_lib_record_function_list,
				NULL,
				NULL,
				&mxi_bnc725_lib_num_record_fields,
				&mxi_bnc725_lib_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

unsigned long bnc725_lib_num_drivers =
		sizeof( bnc725_lib_driver_table )
		/ sizeof( bnc725_lib_driver_table[0] );

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"bnc725_lib",
	MX_VERSION,
	bnc725_lib_driver_table,
	NULL,
	NULL
};

