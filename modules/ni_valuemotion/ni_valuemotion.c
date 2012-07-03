/*
 * Name:    ni_valuemotion.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 libraries for the
 *          National Instruments ValueMotion series of motor controllers.
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
#include "mx_motor.h"
#include "i_pcmotion32.h"
#include "d_pcmotion32.h"

MX_DRIVER ni_valuemotion_driver_table[] = {

{"pcmotion32",       -1,    MXI_CONTROLLER,        MXR_INTERFACE,
				&mxi_pcmotion32_record_function_list,
				NULL,
				&mxi_pcmotion32_generic_function_list,
				&mxi_pcmotion32_num_record_fields,
				&mxi_pcmotion32_rfield_def_ptr},

{"pcmotion32_motor", -1,    MXC_MOTOR,             MXR_DEVICE,
				&mxd_pcmotion32_record_function_list,
				NULL,
				&mxd_pcmotion32_motor_function_list,
				&mxd_pcmotion32_num_record_fields,
				&mxd_pcmotion32_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"ni_valuemotion",
	MX_VERSION,
	ni_valuemotion_driver_table,
	NULL,
	NULL
};

