/*
 * Name:    newport_xps.c
 *
 * Purpose: Module wrapper for the Newport XPS-C and XPS-Q series of
 *          motion controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
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
#include "mx_motor.h"
#include "i_newport_xps.h"
#include "d_newport_xps.h"

MX_DRIVER newport_xps_driver_table[] = {

{"newport_xps",    MXI_CTRL_NEWPORT_XPS, MXI_CONTROLLER,      MXR_INTERFACE,
			&mxi_newport_xps_record_function_list,
			NULL,
			NULL,
			&mxi_newport_xps_num_record_fields,
			&mxi_newport_xps_rfield_def_ptr},

{"newport_xps_motor", MXT_MTR_NEWPORT_XPS, MXC_MOTOR,     MXR_DEVICE,
			&mxd_newport_xps_record_function_list,
			NULL,
			&mxd_newport_xps_motor_function_list,
			&mxd_newport_xps_num_record_fields,
			&mxd_newport_xps_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
        "newport_xps",
        MX_VERSION,
        newport_xps_driver_table,
        NULL,
        NULL
};

