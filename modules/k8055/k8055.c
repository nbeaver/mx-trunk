/*
 * Name: k8055.c
 *
 * Purpose: Module wrapper for the Velleman K8055
 *          USB Experiment Interface Board.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
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
#include "i_k8055.h"
#include "d_k8055_ainput.h"
#include "d_k8055_aoutput.h"
#include "d_k8055_dinput.h"
#include "d_k8055_doutput.h"
#include "d_k8055_scaler.h"
#include "d_k8055_timer.h"

MX_DRIVER k8055_driver_table[] = {

{"k8055", -1,	MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_k8055_record_function_list,
				NULL,
				NULL,
				&mxi_k8055_num_record_fields,
				&mxi_k8055_rfield_def_ptr},

{"k8055_ainput", -1, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_k8055_ainput_record_function_list,
				NULL,
				&mxd_k8055_ainput_analog_input_function_list,
				&mxd_k8055_ainput_num_record_fields,
				&mxd_k8055_ainput_rfield_def_ptr},

{"k8055_aoutput", -1, MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_k8055_aoutput_record_function_list,
				NULL,
				&mxd_k8055_aoutput_analog_output_function_list,
				&mxd_k8055_aoutput_num_record_fields,
				&mxd_k8055_aoutput_rfield_def_ptr},

{"k8055_dinput", -1, MXC_DIGITAL_INPUT, MXR_DEVICE,
				&mxd_k8055_dinput_record_function_list,
				NULL,
				&mxd_k8055_dinput_digital_input_function_list,
				&mxd_k8055_dinput_num_record_fields,
				&mxd_k8055_dinput_rfield_def_ptr},

{"k8055_doutput", -1, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_k8055_doutput_record_function_list,
				NULL,
				&mxd_k8055_doutput_digital_output_function_list,
				&mxd_k8055_doutput_num_record_fields,
				&mxd_k8055_doutput_rfield_def_ptr},

{"k8055_scaler", -1, MXC_SCALER, MXR_DEVICE,
				&mxd_k8055_scaler_record_function_list,
				NULL,
				&mxd_k8055_scaler_scaler_function_list,
				&mxd_k8055_scaler_num_record_fields,
				&mxd_k8055_scaler_rfield_def_ptr},

{"k8055_timer", -1, MXC_SCALER, MXR_DEVICE,
				&mxd_k8055_timer_record_function_list,
				NULL,
				&mxd_k8055_timer_timer_function_list,
				&mxd_k8055_timer_num_record_fields,
				&mxd_k8055_timer_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"k8055",
	MX_VERSION,
	k8055_driver_table,
	NULL,
	NULL
};

