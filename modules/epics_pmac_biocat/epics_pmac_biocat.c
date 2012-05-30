/*
 * Name:    epics_pmac_biocat.c
 *
 * Purpose: Module wrapper for drivers for the BioCAT version of
 *          Tom Coleman's EPICS PMAC database.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012 Illinois Institute of Technology
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

#include "d_epics_pmac_biocat.h"

MX_DRIVER epics_pmac_biocat_driver_table[] = {

{"epics_pmac_biocat",  -1,  MXC_MOTOR,  MXR_DEVICE,
			&mxd_epics_pmac_biocat_record_function_list,
			NULL,
			&mxd_epics_pmac_biocat_motor_function_list,
			&mxd_epics_pmac_biocat_num_record_fields,
			&mxd_epics_pmac_biocat_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"epics_pmac_biocat",
	MX_VERSION,
	epics_pmac_biocat_driver_table,
	NULL,
	NULL
};

