/*
 * Name: pleora_iport.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 C++ libraries
 *          for the Pleora iPORT camera interface.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
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
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"
#include "d_pleora_iport_dio.h"

MX_DRIVER pleora_iport_driver_table[] = {

{"pleora_iport",	-1,	MXI_CONTROLLER,	MXR_INTERFACE,
				&mxi_pleora_iport_record_function_list,
				NULL,
				NULL,
				&mxi_pleora_iport_num_record_fields,
				&mxi_pleora_iport_rfield_def_ptr},

{"pleora_iport_vinput", -1,	MXC_VIDEO_INPUT, MXR_DEVICE,
				&mxd_pleora_iport_vinput_record_function_list,
				NULL,
				NULL,
				&mxd_pleora_iport_vinput_num_record_fields,
				&mxd_pleora_iport_vinput_rfield_def_ptr},

{"pleora_iport_dinput", -1,	MXC_DIGITAL_INPUT, MXR_DEVICE,
				&mxd_pleora_iport_dinput_record_function_list,
				NULL,
			&mxd_pleora_iport_dinput_digital_input_function_list,
				&mxd_pleora_iport_dinput_num_record_fields,
				&mxd_pleora_iport_dinput_rfield_def_ptr},

{"pleora_iport_doutput", -1,	MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_pleora_iport_doutput_record_function_list,
				NULL,
			&mxd_pleora_iport_doutput_digital_output_function_list,
				&mxd_pleora_iport_doutput_num_record_fields,
				&mxd_pleora_iport_doutput_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"pleora_iport",
	MX_VERSION,
	pleora_iport_driver_table,
	NULL,
	NULL
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT__ = NULL;

