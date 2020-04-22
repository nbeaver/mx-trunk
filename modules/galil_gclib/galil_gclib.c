/*
 * Name:    galil_gclib.c
 *
 * Purpose: Module wrapper for Galil motor controllers supported by
 *          the gclib library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
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
#include "mx_motor.h"
#include "i_galil_gclib.h"
#include "d_galil_gclib.h"

MX_DRIVER galil_gclib_driver_table[] = {

{"galil_gclib", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_galil_gclib_record_function_list,
			NULL,
			NULL,
			&mxi_galil_gclib_num_record_fields,
			&mxi_galil_gclib_rfield_def_ptr},

{"galil_gclib_motor", -1, MXC_MOTOR, MXR_DEVICE,
			&mxd_galil_gclib_record_function_list,
			NULL,
			&mxd_galil_gclib_motor_function_list,
			&mxd_galil_gclib_num_record_fields,
			&mxd_galil_gclib_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
        "galil_gclib",
        MX_VERSION,
        galil_gclib_driver_table,
        NULL,
        NULL
};

