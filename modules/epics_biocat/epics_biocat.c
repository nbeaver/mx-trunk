/*
 * Name:    epics_biocat.c
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

#include "d_aps_18id.h"

MX_DRIVER epics_biocat_driver_table[] = {

{"aps_18id", -1, MXC_MOTOR, MXR_DEVICE,
				&mxd_aps_18id_record_function_list,
				NULL,
				&mxd_aps_18id_motor_function_list,
				&mxd_aps_18id_num_record_fields,
				&mxd_aps_18id_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"epics_biocat",
	MX_VERSION,
	epics_biocat_driver_table,
	NULL,
	NULL
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT__ = NULL;

