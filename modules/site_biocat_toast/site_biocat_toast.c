/*
 * Name:    site_biocat_toast.c
 *
 * Purpose: Module wrapper for custom additions for RDI detectors
 *          at the Molecular Biology Consortium.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
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
#include "mx_variable.h"
#include "mx_operation.h"

#include "v_biocat_toast_joystick.h"

MX_DRIVER site_biocat_toast_driver_table[] = {

{"biocat_toast_joystick", -1, MXV_SPECIAL, MXR_VARIABLE,
		&mxv_biocat_toast_joystick_record_function_list,
		NULL,
		NULL,
		&mxv_biocat_toast_joystick_num_record_fields,
		&mxv_biocat_toast_joystick_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
        "site_biocat_toast",
        MX_VERSION,
        site_biocat_toast_driver_table,
        NULL,
        NULL
};

