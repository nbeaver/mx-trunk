/*
 * Name:    xia_handel.c
 *
 * Purpose: Module wrapper for the vendor-provided Handel library for
 *          multichannel analyzers from XIA.
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
#include "mx_net.h"
#include "mx_module.h"
#include "mx_mca.h"
#include "mx_analog_input.h"

#if HAVE_XIA_HANDEL
#include "i_handel.h"
#endif

#include "i_handel_network.h"

#include "d_handel_input.h"
#include "d_handel_mca.h"
#include "d_handel_sum.h"
#include "d_handel_timer.h"

MX_DRIVER xia_handel_driver_table[] = {

#if HAVE_XIA_HANDEL
{"handel",         -1, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_handel_record_function_list,
				NULL,
				NULL,
				&mxi_handel_num_record_fields,
				&mxi_handel_rfield_def_ptr},
#endif /* HAVE_XIA_HANDEL */

{"handel_network", -1, MXI_CONTROLLER, MXR_INTERFACE,
				&mxi_handel_network_record_function_list,
				NULL,
				NULL,
				&mxi_handel_network_num_record_fields,
				&mxi_handel_network_rfield_def_ptr},

{"handel_mca",     -1, MXC_MULTICHANNEL_ANALYZER, MXR_DEVICE,
				&mxd_handel_mca_record_function_list,
				NULL,
				&mxd_handel_mca_mca_function_list,
				&mxd_handel_mca_num_record_fields,
				&mxd_handel_mca_rfield_def_ptr},

{"handel_input",   -1, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_handel_input_record_function_list,
				NULL,
				&mxd_handel_input_analog_input_function_list,
				&mxd_handel_input_num_record_fields,
				&mxd_handel_input_rfield_def_ptr},

{"handel_sum",     -1, MXC_ANALOG_INPUT, MXR_DEVICE,
				&mxd_handel_sum_record_function_list,
				NULL,
				&mxd_handel_sum_analog_input_function_list,
				&mxd_handel_sum_num_record_fields,
				&mxd_handel_sum_rfield_def_ptr},

#if HAVE_XIA_HANDEL
{"handel_timer",   -1,  MXC_TIMER, MXR_DEVICE,
				&mxd_handel_timer_record_function_list,
				NULL,
				&mxd_handel_timer_timer_function_list,
				&mxd_handel_timer_num_record_fields,
				&mxd_handel_timer_rfield_def_ptr},

#endif /* HAVE_XIA_HANDEL */

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"xia_handel",
	MX_VERSION,
	xia_handel_driver_table,
	NULL,
	NULL
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT__ = NULL;

