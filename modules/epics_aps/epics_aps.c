/*
 * Name:    epics_aps.c
 *
 * Purpose: Module wrapper for EPICS drivers specific to
 *          the Advanced Photon Source.
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

#include "mx_amplifier.h"
#include "mx_motor.h"
#include "mx_scan.h"
#include "mx_scan_quick.h"
#include "mx_variable.h"

#include "d_aps_gap.h"
#include "d_aps_quadem_amplifier.h"
#include "sq_aps_id.h"
#include "v_aps_topup.h"

MX_DRIVER epics_aps_driver_table[] = {

{"aps_gap", -1, MXC_MOTOR, MXR_DEVICE,
				&mxd_aps_gap_record_function_list,
				NULL,
				&mxd_aps_gap_motor_function_list,
				&mxd_aps_gap_num_record_fields,
				&mxd_aps_gap_record_field_def_ptr},

{"aps_quadem_amplifier", -1, MXC_AMPLIFIER, MXR_DEVICE,
				&mxd_aps_quadem_record_function_list,
				NULL,
				&mxd_aps_quadem_amplifier_function_list,
				&mxd_aps_quadem_num_record_fields,
				&mxd_aps_quadem_rfield_def_ptr},

{"aps_id_qscan", -1, MXS_QUICK_SCAN, MXR_SCAN,
				&mxs_apsid_quick_scan_record_function_list,
				&mxs_apsid_quick_scan_scan_function_list,
				NULL,
				&mxs_apsid_quick_scan_num_record_fields,
				&mxs_apsid_quick_scan_def_ptr},

{"aps_topup_interlock", -1, MXV_CALC, MXR_VARIABLE,
				&mxv_aps_topup_record_function_list,
				&mxv_aps_topup_variable_function_list,
				NULL,
				&mxv_aps_topup_interlock_num_record_fields,
				&mxv_aps_topup_interlock_field_def_ptr},

{"aps_topup_time", -1, MXV_CALC, MXR_VARIABLE,
				&mxv_aps_topup_record_function_list,
				&mxv_aps_topup_variable_function_list,
				NULL,
				&mxv_aps_topup_time_to_inject_num_record_fields,
				&mxv_aps_topup_time_to_inject_field_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"epics_aps",
	MX_VERSION,
	epics_aps_driver_table,
	NULL,
	NULL
};

MX_EXPORT
MX_MODULE_INIT __MX_MODULE_INIT__ = NULL;

