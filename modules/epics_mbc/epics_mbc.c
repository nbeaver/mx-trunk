/*
 * Name:    epics_mbc.c
 *
 * Purpose: Module wrapper for EPICS drivers specific to the
 *          Molecular Biology Consortium's beamline 4.2.2
 *          at the Advanced Light Source.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
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

#include "mx_epics.h"

#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_pulse_generator.h"

#include "d_mbc_gsc_trigger.h"
#include "d_mbc_noir.h"
#include "d_mbc_noir_trigger.h"

MX_DRIVER epics_mbc_driver_table[] = {

{"mbc_gsc_trigger", -1, MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_mbc_gsc_trigger_record_function_list,
				NULL,
				&mxd_mbc_gsc_trigger_pulser_function_list,
				&mxd_mbc_gsc_trigger_num_record_fields,
				&mxd_mbc_gsc_trigger_rfield_def_ptr},

{"mbc_noir", -1, MXC_AREA_DETECTOR, MXR_DEVICE,
				&mxd_mbc_noir_record_function_list,
				NULL,
				&mxd_mbc_noir_ad_function_list,
				&mxd_mbc_noir_num_record_fields,
				&mxd_mbc_noir_rfield_def_ptr},

{"mbc_noir_trigger", -1, MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_mbc_noir_trigger_record_function_list,
				NULL,
				&mxd_mbc_noir_trigger_pulser_function_list,
				&mxd_mbc_noir_trigger_num_record_fields,
				&mxd_mbc_noir_trigger_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"epics_mbc",
	MX_VERSION,
	epics_mbc_driver_table,
	NULL,
	NULL
};

