/*
 * Name:    vxi_memacc.c
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
#include "i_vxi_memacc.h"

MX_DRIVER vxi_memacc_driver_table[] = {

{"vxi_memacc", -1, MXI_VME, MXR_INTERFACE,
		&mxi_vxi_memacc_record_function_list,
		NULL,
		&mxi_vxi_memacc_vme_function_list,
		&mxi_vxi_memacc_num_record_fields,
		&mxi_vxi_memacc_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"vxi_memacc",
	MX_VERSION,
	vxi_memacc_driver_table,
	NULL,
	NULL
};

