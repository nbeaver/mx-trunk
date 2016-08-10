/*
 * Name: aravis.c
 *
 * Purpose: Module wrapper for the Aravis imaging library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_thread.h"
#include "mx_module.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_aravis.h"
#include "d_aravis_camera.h"

MX_DRIVER aravis_driver_table[] = {

{"aravis", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_aravis_record_function_list,
			NULL,
			NULL,
			&mxi_aravis_num_record_fields,
			&mxi_aravis_rfield_def_ptr},

{"aravis_camera", -1, MXC_VIDEO_INPUT, MXR_DEVICE,
			&mxd_aravis_camera_record_function_list,
			NULL,
			NULL,
			&mxd_aravis_camera_num_record_fields,
			&mxd_aravis_camera_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"aravis",
	MX_VERSION,
	aravis_driver_table,
	NULL,
	NULL
};

