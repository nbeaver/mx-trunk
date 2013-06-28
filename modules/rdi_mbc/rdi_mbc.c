/*
 * Name:    rdi_mbc.c
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

#include "v_rdi_mbc_string.h"

MX_DRIVER rdi_mbc_driver_table[] = {

{"rdi_mbc_string", -1, MXV_SPECIAL, MXR_VARIABLE,
		&mxv_rdi_mbc_string_record_function_list,
		NULL,
		NULL,
		&mxv_rdi_mbc_string_num_record_fields,
		&mxv_rdi_mbc_string_rfield_def_ptr},

{"rdi_mbc_filename", -1, MXV_SPECIAL, MXR_VARIABLE,
		&mxv_rdi_mbc_string_record_function_list,
		NULL,
		NULL,
		&mxv_rdi_mbc_string_num_record_fields,
		&mxv_rdi_mbc_string_rfield_def_ptr},

{"rdi_mbc_datafile_prefix", -1, MXV_SPECIAL, MXR_VARIABLE,
		&mxv_rdi_mbc_string_record_function_list,
		NULL,
		NULL,
		&mxv_rdi_mbc_string_num_record_fields,
		&mxv_rdi_mbc_string_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
        "rdi_mbc",
        MX_VERSION,
        rdi_mbc_driver_table,
        NULL,
        NULL
};

