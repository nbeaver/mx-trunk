/*
 * Name: dalsa_gev.c
 *
 * Purpose: Module wrapper for the vendor-provided DALSA Gev API
 *          for DALSA GigE-Vision cameras.
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
#include "mx_module.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "i_dalsa_gev.h"
#include "d_dalsa_gev_camera.h"

MX_DRIVER dalsa_gev_driver_table[] = {

{"dalsa_gev", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_dalsa_gev_record_function_list,
			NULL,
			NULL,
			&mxi_dalsa_gev_num_record_fields,
			&mxi_dalsa_gev_rfield_def_ptr},

{"dalsa_gev_camera", -1, MXC_VIDEO_INPUT, MXR_DEVICE,
			&mxd_dalsa_gev_camera_record_function_list,
			NULL,
			NULL,
			&mxd_dalsa_gev_camera_num_record_fields,
			&mxd_dalsa_gev_camera_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"dalsa_gev",
	MX_VERSION,
	dalsa_gev_driver_table,
	NULL,
	NULL
};

