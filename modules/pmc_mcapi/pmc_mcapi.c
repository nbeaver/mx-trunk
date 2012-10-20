/*
 * Name: pmc_mcapi.c
 *
 * Purpose: Module wrapper for the Precision MicroControl Motion Control API
 *          (MCAPI) for their series of motor controllers.
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
#include "i_pmc_mcapi.h"
#include "d_pmc_mcapi.h"
#include "d_pmc_mcapi_aio.h"
#include "d_pmc_mcapi_dio.h"

MX_DRIVER pmc_mcapi_driver_table[] = {

{"pmc_mcapi", -1, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_pmc_mcapi_record_function_list,
				NULL,
				NULL,
				&mxi_pmc_mcapi_num_record_fields,
				&mxi_pmc_mcapi_rfield_def_ptr},

{"pmc_mcapi_motor", -1, MXC_MOTOR, MXR_DEVICE,
				&mxd_pmc_mcapi_record_function_list,
				NULL,
				&mxd_pmc_mcapi_motor_function_list,
				&mxd_pmc_mcapi_num_record_fields,
				&mxd_pmc_mcapi_rfield_def_ptr},

{"pmc_mcapi_ainput", -1, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_pmc_mcapi_ain_record_function_list,
				NULL,
				&mxd_pmc_mcapi_ain_analog_input_function_list,
				&mxd_pmc_mcapi_ain_num_record_fields,
				&mxd_pmc_mcapi_ain_rfield_def_ptr},

{"pmc_mcapi_aoutput", -1, MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_pmc_mcapi_aout_record_function_list,
				NULL,
				&mxd_pmc_mcapi_aout_analog_output_function_list,
				&mxd_pmc_mcapi_aout_num_record_fields,
				&mxd_pmc_mcapi_aout_rfield_def_ptr},

{"pmc_mcapi_dinput", -1, MXC_DIGITAL_INPUT, MXR_DEVICE,
				&mxd_pmc_mcapi_din_record_function_list,
				NULL,
				&mxd_pmc_mcapi_din_digital_input_function_list,
				&mxd_pmc_mcapi_din_num_record_fields,
				&mxd_pmc_mcapi_din_rfield_def_ptr},

{"pmc_mcapi_doutput", -1, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_pmc_mcapi_dout_record_function_list,
				NULL,
			      &mxd_pmc_mcapi_dout_digital_output_function_list,
				&mxd_pmc_mcapi_dout_num_record_fields,
				&mxd_pmc_mcapi_dout_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"pmc_mcapi",
	MX_VERSION,
	pmc_mcapi_driver_table,
	NULL,
	NULL
};

