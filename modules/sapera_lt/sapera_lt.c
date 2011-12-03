/*
 * Name: sapera_lt.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 C++ libraries
 *          for DALSA's Sapera LT camera interface.
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
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_sapera_lt.h"
#include "d_sapera_lt_frame_grabber.h"

MX_DRIVER sapera_lt_driver_table[] = {

{"sapera_lt", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_sapera_lt_record_function_list,
			NULL,
			NULL,
			&mxi_sapera_lt_num_record_fields,
			&mxi_sapera_lt_rfield_def_ptr},

{"sapera_lt_frame_grabber", -1,	MXC_VIDEO_INPUT, MXR_DEVICE,
			&mxd_sapera_lt_frame_grabber_record_function_list,
			NULL,
			NULL,
			&mxd_sapera_lt_frame_grabber_num_record_fields,
			&mxd_sapera_lt_frame_grabber_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"sapera_lt",
	MX_VERSION,
	sapera_lt_driver_table,
	NULL,
	NULL
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT__ = NULL;

