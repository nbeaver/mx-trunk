/*
 * Name: ni_daqmx.c
 *
 * Purpose: Module wrapper for National Instruments DAQmx devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2011-2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_NI_DAQMX_DEBUG_DLL	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_module.h"
#include "i_ni_daqmx.h"
#include "d_ni_daqmx_ainput.h"
#include "d_ni_daqmx_aoutput.h"
#include "d_ni_daqmx_dinput.h"
#include "d_ni_daqmx_doutput.h"
#include "d_ni_daqmx_thermocouple.h"

MX_DRIVER ni_daqmx_driver_table[] = {

{"ni_daqmx", -1,	MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_ni_daqmx_record_function_list,
				NULL,
				NULL,
				&mxi_ni_daqmx_num_record_fields,
				&mxi_ni_daqmx_rfield_def_ptr},

{"ni_daqmx_ainput", -1, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_ni_daqmx_ainput_record_function_list,
				NULL,
			   &mxd_ni_daqmx_ainput_analog_input_function_list,
				&mxd_ni_daqmx_ainput_num_record_fields,
				&mxd_ni_daqmx_ainput_rfield_def_ptr},

{"ni_daqmx_aoutput", -1, MXC_ANALOG_OUTPUT, MXR_DEVICE,
				&mxd_ni_daqmx_aoutput_record_function_list,
				NULL,
			   &mxd_ni_daqmx_aoutput_analog_output_function_list,
				&mxd_ni_daqmx_aoutput_num_record_fields,
				&mxd_ni_daqmx_aoutput_rfield_def_ptr},

{"ni_daqmx_dinput", -1, MXC_DIGITAL_INPUT, MXR_DEVICE,
				&mxd_ni_daqmx_dinput_record_function_list,
				NULL,
			   &mxd_ni_daqmx_dinput_digital_input_function_list,
				&mxd_ni_daqmx_dinput_num_record_fields,
				&mxd_ni_daqmx_dinput_rfield_def_ptr},

{"ni_daqmx_doutput", -1, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_ni_daqmx_doutput_record_function_list,
				NULL,
			   &mxd_ni_daqmx_doutput_digital_output_function_list,
				&mxd_ni_daqmx_doutput_num_record_fields,
				&mxd_ni_daqmx_doutput_rfield_def_ptr},

{"ni_daqmx_thermocouple", -1, MXC_ANALOG_INPUT, MXR_DEVICE,
			&mxd_ni_daqmx_thermocouple_record_function_list,
			NULL,
			&mxd_ni_daqmx_thermocouple_analog_input_function_list,
			&mxd_ni_daqmx_thermocouple_num_record_fields,
			&mxd_ni_daqmx_thermocouple_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"ni_daqmx",
	MX_VERSION,
	ni_daqmx_driver_table,
	NULL,
	NULL
};

