/*
 * Name:    esone_camac.c
 *
 * Purpose: Module wrapper for ESONE style CAMAC libraries.
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
#include "mx_camac.h"
#include "i_esone_camac.h"

MX_DRIVER esone_camac_driver_table[] = {

{"esone_camac",  -1,    MXI_PORTIO,  MXR_INTERFACE,
				&mxi_esone_camac_record_function_list,
				NULL,
				&mxi_esone_camac_camac_function_list,
				&mxi_esone_camac_num_record_fields,
				&mxi_esone_camac_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"esone_camac",
	MX_VERSION,
	esone_camac_driver_table,
	NULL,
	NULL
};

