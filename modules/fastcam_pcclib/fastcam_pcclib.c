/*
 * Name: fastcam_pcclib.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 C++ libraries
 *          for Photron's FASTCAM PccLib SDK.
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
#include "i_fastcam_pcclib.h"
#include "d_fastcam_pcclib_camera.h"

MX_DRIVER fastcam_pcclib_driver_table[] = {

{"fastcam_pcclib", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_fastcam_pcclib_record_function_list,
			NULL,
			NULL,
			&mxi_fastcam_pcclib_num_record_fields,
			&mxi_fastcam_pcclib_rfield_def_ptr},

{"fastcam_pcclib_camera", -1, MXC_VIDEO_INPUT, MXR_DEVICE,
			&mxd_fastcam_pcclib_camera_record_function_list,
			NULL,
			NULL,
			&mxd_fastcam_pcclib_camera_num_record_fields,
			&mxd_fastcam_pcclib_camera_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"fastcam_pcclib",
	MX_VERSION,
	fastcam_pcclib_driver_table,
	NULL,
	NULL
};

