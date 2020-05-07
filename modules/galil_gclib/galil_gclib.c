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
#include "d_galil_gclib_motor.h"
#include "d_galil_gclib_ainput.h"
#include "d_galil_gclib_aoutput.h"
#include "d_galil_gclib_dinput.h"
#include "d_galil_gclib_doutput.h"

MX_DRIVER galil_gclib_driver_table[] = {

{"galil_gclib", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_galil_gclib_record_function_list,
			NULL,
			NULL,
			&mxi_galil_gclib_num_record_fields,
			&mxi_galil_gclib_rfield_def_ptr},

{"galil_gclib_motor", -1, MXC_MOTOR, MXR_DEVICE,
			&mxd_galil_gclib_motor_record_function_list,
			NULL,
			&mxd_galil_gclib_motor_motor_function_list,
			&mxd_galil_gclib_motor_num_record_fields,
			&mxd_galil_gclib_motor_rfield_def_ptr},

{"galil_gclib_ainput", -1, MXC_ANALOG_INPUT, MXR_DEVICE,
			&mxd_galil_gclib_ainput_record_function_list,
			NULL,
			&mxd_galil_gclib_ainput_analog_input_function_list,
			&mxd_galil_gclib_ainput_num_record_fields,
			&mxd_galil_gclib_ainput_rfield_def_ptr},

{"galil_gclib_aoutput", -1, MXC_ANALOG_OUTPUT, MXR_DEVICE,
			&mxd_galil_gclib_aoutput_record_function_list,
			NULL,
			&mxd_galil_gclib_aoutput_analog_output_function_list,
			&mxd_galil_gclib_aoutput_num_record_fields,
			&mxd_galil_gclib_aoutput_rfield_def_ptr},

{"galil_gclib_dinput", -1, MXC_DIGITAL_INPUT, MXR_DEVICE,
			&mxd_galil_gclib_dinput_record_function_list,
			NULL,
			&mxd_galil_gclib_dinput_digital_input_function_list,
			&mxd_galil_gclib_dinput_num_record_fields,
			&mxd_galil_gclib_dinput_rfield_def_ptr},

{"galil_gclib_doutput", -1, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
			&mxd_galil_gclib_doutput_record_function_list,
			NULL,
			&mxd_galil_gclib_doutput_digital_output_function_list,
			&mxd_galil_gclib_doutput_num_record_fields,
			&mxd_galil_gclib_doutput_rfield_def_ptr},

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

