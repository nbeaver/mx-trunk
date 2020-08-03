/*
 * Name:    zwo_efw.c
 *
 * Purpose: Module wrapper for ZWO Electronic Filter Wheels.
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
#include "i_zwo_efw.h"
#include "d_zwo_efw_motor.h"

MX_DRIVER zwo_efw_driver_table[] = {

{"zwo_efw", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_zwo_efw_record_function_list,
			NULL,
			NULL,
			&mxi_zwo_efw_num_record_fields,
			&mxi_zwo_efw_rfield_def_ptr},

{"zwo_efw_motor", -1, MXC_MOTOR, MXR_DEVICE,
			&mxd_zwo_efw_motor_record_function_list,
			NULL,
			&mxd_zwo_efw_motor_motor_function_list,
			&mxd_zwo_efw_motor_num_record_fields,
			&mxd_zwo_efw_motor_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
        "zwo_efw",
        MX_VERSION,
        zwo_efw_driver_table,
        NULL,
        NULL
};

