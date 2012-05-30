/*
 * Name: radicon_taurus.c
 *
 * Purpose: Module wrapper for the Radicon Taurus series of CMOS detectors.
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
#include "mx_area_detector.h"
#include "d_radicon_taurus.h"

MX_DRIVER radicon_taurus_driver_table[] = {

{"radicon_taurus", -1,	MXC_AREA_DETECTOR, MXR_DEVICE,
				&mxd_radicon_taurus_record_function_list,
				NULL,
				NULL,
				&mxd_radicon_taurus_num_record_fields,
				&mxd_radicon_taurus_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"radicon_taurus",
	MX_VERSION,
	radicon_taurus_driver_table,
	NULL,
	NULL
};

