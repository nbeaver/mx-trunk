/*
 * Name:    linux_portio.c
 *
 * Purpose: Module wrapper for the 'linux_portio' driver.
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
#include "i_linux_portio.h"

MX_DRIVER linux_portio_driver_table[] = {

{"linux_portio", -1, MXI_PORTIO,  MXR_INTERFACE,
		&mxi_linux_portio_record_function_list,
		NULL,
		&mxi_linux_portio_portio_function_list,
		&mxi_linux_portio_num_record_fields,
		&mxi_linux_portio_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"linux_portio",
	MX_VERSION,
	linux_portio_driver_table,
	NULL,
	NULL
};

