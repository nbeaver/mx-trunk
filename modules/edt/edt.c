/*
 * Name:    edt.c
 *
 * Purpose: Module wrapper for the EDT series of video capture cards.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
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
#include "mx_rs232.h"
#include "mx_image.h"
#include "mx_video_input.h"

#include "edtinc.h"	/* Vendor include file. */

#include "i_edt.h"
#include "i_edt_rs232.h"
#include "d_edt.h"

MX_DRIVER edt_driver_table[] = {

{"edt", -1, MXI_CONTROLLER, MXR_INTERFACE,
		&mxi_edt_record_function_list,
		NULL,
		NULL,
		&mxi_edt_num_record_fields,
		&mxi_edt_rfield_def_ptr},

{"edt_rs232", -1, MXI_RS232, MXR_INTERFACE,
		&mxi_edt_rs232_record_function_list,
		NULL,
		&mxi_edt_rs232_rs232_function_list,
		&mxi_edt_rs232_num_record_fields,
		&mxi_edt_rs232_rfield_def_ptr},

{"edt_video_input", -1, MXC_VIDEO_INPUT, MXR_DEVICE,
		&mxd_edt_record_function_list,
		NULL,
		NULL,
		&mxd_edt_num_record_fields,
		&mxd_edt_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
        "edt",
        MX_VERSION,
        edt_driver_table,
        NULL,
        NULL
};

