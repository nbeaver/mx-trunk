/*
 * Name:    aviex_pccd.c
 *
 * Purpose: Module wrapper for Aviex (later Dexela) PCCD series of detectors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
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
#include "mx_area_detector.h"
#include "d_aviex_pccd.h"

MX_DRIVER aviex_pccd_driver_table[] = {

{"pccd_170170",    -1, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_aviex_pccd_record_function_list,
				NULL,
				&mxd_aviex_pccd_ad_function_list,
				&mxd_aviex_pccd_170170_num_record_fields,
				&mxd_aviex_pccd_170170_rfield_def_ptr},

{"pccd_4824",      -1, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_aviex_pccd_record_function_list,
				NULL,
				&mxd_aviex_pccd_ad_function_list,
				&mxd_aviex_pccd_4824_num_record_fields,
				&mxd_aviex_pccd_4824_rfield_def_ptr},

{"pccd_16080",     -1, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_aviex_pccd_record_function_list,
				NULL,
				&mxd_aviex_pccd_ad_function_list,
				&mxd_aviex_pccd_16080_num_record_fields,
				&mxd_aviex_pccd_16080_rfield_def_ptr},

{"pccd_9785",      -1, MXC_AREA_DETECTOR,  MXR_DEVICE,
				&mxd_aviex_pccd_record_function_list,
				NULL,
				&mxd_aviex_pccd_ad_function_list,
				&mxd_aviex_pccd_9785_num_record_fields,
				&mxd_aviex_pccd_9785_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"aviex_pccd",
	MX_VERSION,
	aviex_pccd_driver_table,
	NULL,
	NULL
};

