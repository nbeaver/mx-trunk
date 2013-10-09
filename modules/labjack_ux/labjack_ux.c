/*
 * Name: labjack_ux.c
 *
 * Purpose: Module wrapper for the Labjack U3, U6, and U9 devices.
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

#include "i_labjack_ux.h"
#if 0
#include "d_labjack_ux_ainput.h"
#include "d_labjack_ux_aoutput.h"
#include "d_labjack_ux_dinput.h"
#include "d_labjack_ux_doutput.h"
#include "d_labjack_ux_scaler.h"
#include "d_labjack_ux_timer.h"
#endif

MX_DRIVER labjack_ux_driver_table[] = {

{"labjack_ux", -1,	MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_labjack_ux_record_function_list,
				NULL,
				NULL,
				&mxi_labjack_ux_num_record_fields,
				&mxi_labjack_ux_rfield_def_ptr},

#if 0
{"labjack_ux_ainput", -1, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_labjack_ux_ainput_record_function_list,
				NULL,
				&mxd_labjack_ux_ainput_analog_input_function_list,
				&mxd_labjack_ux_ainput_num_record_fields,
				&mxd_labjack_ux_ainput_rfield_def_ptr},

{"labjack_ux_aoutput", -1, MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_labjack_ux_aoutput_record_function_list,
				NULL,
				&mxd_labjack_ux_aoutput_analog_output_function_list,
				&mxd_labjack_ux_aoutput_num_record_fields,
				&mxd_labjack_ux_aoutput_rfield_def_ptr},

{"labjack_ux_dinput", -1, MXC_DIGITAL_INPUT, MXR_DEVICE,
				&mxd_labjack_ux_dinput_record_function_list,
				NULL,
				&mxd_labjack_ux_dinput_digital_input_function_list,
				&mxd_labjack_ux_dinput_num_record_fields,
				&mxd_labjack_ux_dinput_rfield_def_ptr},

{"labjack_ux_doutput", -1, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_labjack_ux_doutput_record_function_list,
				NULL,
				&mxd_labjack_ux_doutput_digital_output_function_list,
				&mxd_labjack_ux_doutput_num_record_fields,
				&mxd_labjack_ux_doutput_rfield_def_ptr},

{"labjack_ux_scaler", -1, MXC_SCALER, MXR_DEVICE,
				&mxd_labjack_ux_scaler_record_function_list,
				NULL,
				&mxd_labjack_ux_scaler_scaler_function_list,
				&mxd_labjack_ux_scaler_num_record_fields,
				&mxd_labjack_ux_scaler_rfield_def_ptr},

{"labjack_ux_timer", -1, MXC_SCALER, MXR_DEVICE,
				&mxd_labjack_ux_timer_record_function_list,
				NULL,
				&mxd_labjack_ux_timer_timer_function_list,
				&mxd_labjack_ux_timer_num_record_fields,
				&mxd_labjack_ux_timer_rfield_def_ptr},
#endif

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"labjack_ux",
	MX_VERSION,
	labjack_ux_driver_table,
	NULL,
	NULL
};

