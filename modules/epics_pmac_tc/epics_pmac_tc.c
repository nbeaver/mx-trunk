/*
 * Name:    epics_pmac_tc.c
 *
 * Purpose: Module wrapper for EPICS drivers specific to BioCAT
 *          at the APS 18-ID beamline.
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

#include "mx_motor.h"

#include "d_pmac_tc.h"

MX_DRIVER epics_pmac_tc_driver_table[] = {

/* pmac_tc_motor and pmac_bio_motor share the same driver. */

{"pmac_tc_motor",  -1, MXC_MOTOR, MXR_DEVICE,
				&mxd_pmac_tc_motor_record_function_list,
				NULL,
				&mxd_pmac_tc_motor_motor_function_list,
				&mxd_pmac_tc_motor_num_record_fields,
				&mxd_pmac_tc_motor_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"epics_pmac_tc",
	MX_VERSION,
	epics_pmac_tc_driver_table,
	NULL,
	NULL
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT__ = NULL;

