/*
 * Name:    ortec_umcbi.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 libraries for the
 *          EG&G Ortec Unified MCB Interface for 32 Bits programmer's toolkit
 *          (Part # A11-B32) under Microsoft Windows.
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
#include "mx_mca.h"
#include "i_umcbi.h"
#include "d_trump.h"

MX_DRIVER ortec_umcbi_driver_table[] = {

{"umcbi",       -1,    MXI_CONTROLLER,            MXR_INTERFACE,
				&mxi_umcbi_record_function_list,
				NULL,
				&mxi_umcbi_generic_function_list,
				&mxi_umcbi_num_record_fields,
				&mxi_umcbi_rfield_def_ptr},

{"trump_mca",   -1,    MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_trump_record_function_list,
				NULL,
				&mxd_trump_mca_function_list,
				&mxd_trump_num_record_fields,
				&mxd_trump_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"ortec_umcbi",
	MX_VERSION,
	ortec_umcbi_driver_table,
	NULL,
	NULL
};

