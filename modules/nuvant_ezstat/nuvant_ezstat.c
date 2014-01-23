/*
 * Name: nuvant_ezstat.c
 *
 * Purpose: Module wrapper for the MX drivers for NuVant EZstat potentiostats
 *          and galvanostats.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
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
#include "i_nuvant_ezstat.h"
#include "d_nuvant_ezstat_ainput.h"
#include "d_nuvant_ezstat_aoutput.h"
#include "d_nuvant_ezstat_doutput.h"

MX_DRIVER nuvant_ezstat_driver_table[] = {

{"nuvant_ezstat", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_nuvant_ezstat_record_function_list,
			NULL,
			NULL,
			&mxi_nuvant_ezstat_num_record_fields,
			&mxi_nuvant_ezstat_rfield_def_ptr},

{"nuvant_ezstat_ainput", -1, MXC_ANALOG_INPUT, MXR_DEVICE,
			&mxd_nuvant_ezstat_ainput_record_function_list,
			NULL,
			&mxd_nuvant_ezstat_ainput_analog_input_function_list,
			&mxd_nuvant_ezstat_ainput_num_record_fields,
			&mxd_nuvant_ezstat_ainput_rfield_def_ptr},

{"nuvant_ezstat_aoutput", -1, MXC_ANALOG_OUTPUT, MXR_DEVICE,
			&mxd_nuvant_ezstat_aoutput_record_function_list,
			NULL,
			&mxd_nuvant_ezstat_aoutput_analog_output_function_list,
			&mxd_nuvant_ezstat_aoutput_num_record_fields,
			&mxd_nuvant_ezstat_aoutput_rfield_def_ptr},

{"nuvant_ezstat_doutput", -1, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
			&mxd_nuvant_ezstat_doutput_record_function_list,
			NULL,
			&mxd_nuvant_ezstat_doutput_digital_output_function_list,
			&mxd_nuvant_ezstat_doutput_num_record_fields,
			&mxd_nuvant_ezstat_doutput_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"nuvant_ezstat",
	MX_VERSION,
	nuvant_ezstat_driver_table,
	NULL,
	NULL
};

