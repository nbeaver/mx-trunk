/*
 * Name: radicon_helios.c
 *
 * Purpose: Module wrapper for the Radicon Helios 25x20
 *          and 10x10 CMOS detectors.
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
#include "mx_area_detector.h"
#include "d_radicon_helios.h"
#include "d_radicon_helios_trigger.h"

MX_DRIVER radicon_helios_driver_table[] = {

{"radicon_helios", -1,	MXC_AREA_DETECTOR, MXR_DEVICE,
				&mxd_radicon_helios_record_function_list,
				NULL,
				NULL,
				&mxd_radicon_helios_num_record_fields,
				&mxd_radicon_helios_rfield_def_ptr},

{"radicon_helios_trigger", -1, MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_rh_trigger_record_function_list,
				NULL,
				&mxd_rh_trigger_pulser_function_list,
				&mxd_rh_trigger_num_record_fields,
				&mxd_rh_trigger_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"radicon_helios",
	MX_VERSION,
	radicon_helios_driver_table,
	NULL,
	NULL
};

