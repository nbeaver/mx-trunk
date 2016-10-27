/*
 * Name: bnc725.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 C++ libraries
 *          for the BNC725 digital delay generator.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2012, 2016 Illinois Institute of Technology
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
#include "mx_pulse_generator.h"
#include "i_bnc725.h"
#include "d_bnc725.h"

MX_DRIVER bnc725_driver_table[] = {

{"bnc725",	-1,	MXI_CONTROLLER,	MXR_INTERFACE,
				&mxi_bnc725_record_function_list,
				NULL,
				NULL,
				&mxi_bnc725_num_record_fields,
				&mxi_bnc725_rfield_def_ptr},

{"bnc725_pulser", -1, MXC_PULSE_GENERATOR, MXR_DEVICE,
				&mxd_bnc725_record_function_list,
				NULL,
				&mxd_bnc725_pulse_generator_function_list,
				&mxd_bnc725_num_record_fields,
				&mxd_bnc725_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"bnc725",
	MX_VERSION,
	bnc725_driver_table,
	NULL,
	NULL
};

