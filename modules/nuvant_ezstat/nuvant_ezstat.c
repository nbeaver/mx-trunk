/*
 * Name: nuvant_ezstat.c
 *
 * Purpose: Module wrapper for the MX drivers for Nuvant EZstat potentiostats
 *          and galvanostats.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
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
#include "mx_motor.h"
#include "i_nuvant_ezstat.h"

MX_DRIVER nuvant_ezstat_driver_table[] = {

{"nuvant_ezstat", -1, MXI_CONTROLLER, MXR_INTERFACE,
			&mxi_nuvant_ezstat_record_function_list,
			NULL,
			NULL,
			&mxi_nuvant_ezstat_num_record_fields,
			&mxi_nuvant_ezstat_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"nuvant_ezstat",
	MX_VERSION,
	nuvant_ezstat_driver_table,
	NULL,
	NULL
};

