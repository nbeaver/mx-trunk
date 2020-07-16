/*
 * Name:    xglab_dante.c
 *
 * Purpose: Module wrapper for the XGLab DANTE MCA.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
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
#include "mx_mcs.h"
#include "mx_mca.h"

#include "i_dante.h"
#include "d_dante_mcs.h"
#include "d_dante_mca.h"

MX_DRIVER xglab_dante_driver_table[] = {

{"dante",         -1, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_dante_record_function_list,
				NULL,
				NULL,
				&mxi_dante_num_record_fields,
				&mxi_dante_rfield_def_ptr},

{"dante_mca",     -1, MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_dante_mca_record_function_list,
				NULL,
				&mxd_dante_mca_mca_function_list,
				&mxd_dante_mca_num_record_fields,
				&mxd_dante_mca_rfield_def_ptr},

{"dante_mcs",     -1, MXC_MULTICHANNEL_SCALER, MXR_DEVICE,
				&mxd_dante_mcs_record_function_list,
				NULL,
				&mxd_dante_mcs_mcs_function_list,
				&mxd_dante_mcs_num_record_fields,
				&mxd_dante_mcs_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"xglab_dante",
	MX_VERSION,
	xglab_dante_driver_table,
	NULL,
	NULL
};

