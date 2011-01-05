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
 * Copyright 2010 Illinois Institute of Technology
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
#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"

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

