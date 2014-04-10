/*
 * Name:    site_biocat_toast.c
 *
 * Purpose: Module wrapper for BioCAT-specific drivers used with the
 *          18-ID Compumotor 6K "toast" motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
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

#include "v_biocat_6k_joystick.h"

MX_DRIVER site_biocat_toast_driver_table[] = {

{"biocat_6k_joystick", -1, MXV_SPECIAL, MXR_VARIABLE,
		&mxv_biocat_6k_joystick_record_function_list,
		NULL,
		NULL,
		&mxv_biocat_6k_joystick_num_record_fields,
		&mxv_biocat_6k_joystick_rfield_def_ptr},

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

