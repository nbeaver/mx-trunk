/*
 * Name: daqmx_base.c
 *
 * Purpose: Module wrapper for National Instruments DAQmx Base devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
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
#include "i_daqmx_base.h"
#include "d_daqmx_base_dinput.h"

MX_DRIVER daqmx_base_driver_table[] = {

{"daqmx_base", -1,	MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_daqmx_base_record_function_list,
				NULL,
				NULL,
				&mxi_daqmx_base_num_record_fields,
				&mxi_daqmx_base_rfield_def_ptr},

{"daqmx_base_dinput", -1, MXC_DIGITAL_INPUT, MXR_DEVICE,
				&mxd_daqmx_base_dinput_record_function_list,
				NULL,
			    &mxd_daqmx_base_dinput_digital_input_function_list,
				&mxd_daqmx_base_dinput_num_record_fields,
				&mxd_daqmx_base_dinput_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"daqmx_base",
	MX_VERSION,
	daqmx_base_driver_table,
	NULL,
	NULL
};

