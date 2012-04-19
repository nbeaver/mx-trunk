/*
 * Name: powerpmac.c
 *
 * Purpose: Module wrapper for the Delta Tau PowerPMAC series
 *          of motion controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
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
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_variable.h"
#include "i_powerpmac.h"
#include "d_powerpmac.h"
#include "d_powerpmac_aio.h"
#include "d_powerpmac_dio.h"
#include "v_powerpmac.h"

MX_DRIVER powerpmac_driver_table[] = {

{"powerpmac", -1, MXI_CONTROLLER, MXR_INTERFACE,
                                &mxi_powerpmac_record_function_list,
                                NULL,
                                NULL,
				&mxi_powerpmac_num_record_fields,
				&mxi_powerpmac_rfield_def_ptr},

{"powerpmac_motor", -1,  MXC_MOTOR,  MXR_DEVICE,
				&mxd_powerpmac_record_function_list,
				NULL,
				&mxd_powerpmac_motor_function_list,
				&mxd_powerpmac_num_record_fields,
				&mxd_powerpmac_rfield_def_ptr},

{"powerpmac_ainput", -1, MXC_ANALOG_INPUT,   MXR_DEVICE,
				&mxd_powerpmac_ain_record_function_list,
				NULL,
				&mxd_powerpmac_ain_analog_input_function_list,
				&mxd_powerpmac_ain_num_record_fields,
				&mxd_powerpmac_ain_rfield_def_ptr},

{"powerpmac_aoutput", -1, MXC_ANALOG_OUTPUT,  MXR_DEVICE,
				&mxd_powerpmac_aout_record_function_list,
				NULL,
				&mxd_powerpmac_aout_analog_output_function_list,
				&mxd_powerpmac_aout_num_record_fields,
				&mxd_powerpmac_aout_rfield_def_ptr},

{"powerpmac_dinput", -1, MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_powerpmac_din_record_function_list,
				NULL,
				&mxd_powerpmac_din_digital_input_function_list,
				&mxd_powerpmac_din_num_record_fields,
				&mxd_powerpmac_din_rfield_def_ptr},

{"powerpmac_doutput", -1, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_powerpmac_dout_record_function_list,
				NULL,
				&mxd_powerpmac_dout_digital_output_function_list,
				&mxd_powerpmac_dout_num_record_fields,
				&mxd_powerpmac_dout_rfield_def_ptr},

{"powerpmac_long", -1, MXV_POWERPMAC,	  MXR_VARIABLE,
				&mxv_powerpmac_record_function_list,
				&mxv_powerpmac_variable_function_list,
				NULL,
				&mxv_powerpmac_long_num_record_fields,
				&mxv_powerpmac_long_rfield_def_ptr},

{"powerpmac_ulong", -1, MXV_POWERPMAC,	  MXR_VARIABLE,
				&mxv_powerpmac_record_function_list,
				&mxv_powerpmac_variable_function_list,
				NULL,
				&mxv_powerpmac_ulong_num_record_fields,
				&mxv_powerpmac_ulong_rfield_def_ptr},

{"powerpmac_double", -1, MXV_POWERPMAC,	  MXR_VARIABLE,
				&mxv_powerpmac_record_function_list,
				&mxv_powerpmac_variable_function_list,
				NULL,
				&mxv_powerpmac_double_num_record_fields,
				&mxv_powerpmac_double_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"powerpmac",
	MX_VERSION,
	powerpmac_driver_table,
	NULL,
	NULL
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT__ = NULL;

