/*
 * Name:    v4l2_input.c
 *
 * Purpose: Module wrapper for the Linux-only Video4Linux version 2
 *          video capture API.
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
#include "mx_image.h"
#include "mx_video_input.h"
#include "d_v4l2_input.h"

MX_DRIVER v4l2_input_driver_table[] = {

{"v4l2_input", -1, MXC_VIDEO_INPUT,  MXR_DEVICE,
		&mxd_v4l2_input_record_function_list,
		NULL,
		NULL,
		&mxd_v4l2_input_num_record_fields,
		&mxd_v4l2_input_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"v4l2_input",
	MX_VERSION,
	v4l2_input_driver_table,
	NULL,
	NULL
};

