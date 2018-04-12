/*
 * Name: winusb.c
 *
 * Purpose: Module wrapper for the WinUSB API for Microsoft Windows.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
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
#include "mx_usb.h"
#include "i_winusb.h"

MX_DRIVER winusb_driver_table[] = {

{"winusb", -1, MXI_USB, MXR_INTERFACE,
			&mxi_winusb_record_function_list,
			NULL,
			&mxi_winusb_usb_function_list,
			&mxi_winusb_num_record_fields,
			&mxi_winusb_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"winusb",
	MX_VERSION,
	winusb_driver_table,
	NULL,
	NULL
};

