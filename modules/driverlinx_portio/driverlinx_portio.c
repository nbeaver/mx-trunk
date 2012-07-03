/*
 * Name:    driverlinx_portio.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 libraries for the
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
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
#include "mx_portio.h"
#include "i_driverlinx_portio.h"

MX_DRIVER driverlinx_portio_driver_table[] = {

{"driverlinx_portio",  -1,    MXI_PORTIO,  MXR_INTERFACE,
				&mxi_driverlinx_portio_record_function_list,
				NULL,
				&mxi_driverlinx_portio_portio_function_list,
				&mxi_driverlinx_portio_num_record_fields,
				&mxi_driverlinx_portio_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"driverlinx_portio",
	MX_VERSION,
	driverlinx_portio_driver_table,
	NULL,
	NULL
};

