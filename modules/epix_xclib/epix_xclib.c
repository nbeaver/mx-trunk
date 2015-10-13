/*
 * Name:    epix_xclib.c
 *
 * Purpose: Module wrapper for the vendor-provided XCLIB library for video
 *          capture interfaces from EPIX Inc.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include "xcliball.h"	/* Vendor include file. */

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_module.h"
#include "mx_rs232.h"
#include "mx_camera_link.h"
#include "mx_image.h"
#include "mx_video_input.h"

#include "i_epix_xclib.h"
#include "i_epix_rs232.h"
#include "i_epix_camera_link.h"
#include "d_epix_xclib_dio.h"
#include "d_epix_xclib.h"

MX_DRIVER epix_xclib_driver_table[] = {

{"epix_xclib", -1,	MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_epix_xclib_record_function_list,
				NULL,
				NULL,
				&mxi_epix_xclib_num_record_fields,
				&mxi_epix_xclib_rfield_def_ptr},

{"epix_rs232",	-1,	MXI_RS232, MXR_INTERFACE,
				&mxi_epix_rs232_record_function_list,
				NULL,
				&mxi_epix_rs232_rs232_function_list,
				&mxi_epix_rs232_num_record_fields,
				&mxi_epix_rs232_rfield_def_ptr},

{"epix_camera_link", -1, MXI_CAMERA_LINK, MXR_INTERFACE,
				&mxi_epix_camera_link_record_function_list,
				NULL,
				NULL,
				&mxi_epix_camera_link_num_record_fields,
				&mxi_epix_camera_link_rfield_def_ptr},

{"epix_xclib_dinput", -1, MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_epix_xclib_dinput_record_function_list,
				NULL,
			  &mxd_epix_xclib_dinput_digital_input_function_list,
				&mxd_epix_xclib_dinput_num_record_fields,
				&mxd_epix_xclib_dinput_rfield_def_ptr},

{"epix_xclib_doutput", -1, MXC_DIGITAL_OUTPUT, MXR_DEVICE,
				&mxd_epix_xclib_doutput_record_function_list,
				NULL,
			  &mxd_epix_xclib_doutput_digital_output_function_list,
				&mxd_epix_xclib_doutput_num_record_fields,
				&mxd_epix_xclib_doutput_rfield_def_ptr},

{"epix_xclib_video_input", -1, MXC_VIDEO_INPUT, MXR_DEVICE,
				&mxd_epix_xclib_record_function_list,
				NULL,
				NULL,
				&mxd_epix_xclib_num_record_fields,
				&mxd_epix_xclib_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
        "epix_xclib",
        MX_VERSION,
        epix_xclib_driver_table,
        NULL,
        NULL
};

