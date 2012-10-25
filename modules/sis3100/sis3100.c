/*
 * Name:    sis3100.c
 *
 * Purpose: Module wrapper for the Struck
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
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
#include "mx_vme.h"
#include "i_sis3100.h"

MX_DRIVER sis3100_driver_table[] = {

{"sis3100", -1, MXI_VME, MXR_INTERFACE,
		&mxi_sis3100_record_function_list,
		NULL,
		&mxi_sis3100_vme_function_list,
		&mxi_sis3100_num_record_fields,
		&mxi_sis3100_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"sis3100",
	MX_VERSION,
	sis3100_driver_table,
	NULL,
	NULL
};

