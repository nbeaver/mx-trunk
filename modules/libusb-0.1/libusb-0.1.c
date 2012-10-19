/*
 * Name: libusb-0.1.c
 *
 * Purpose: Module wrapper for the old deprecated libusb-0.1 API.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
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
#include "mx_usb.h"
#include "i_libusb-0.1.h"

MX_DRIVER libusb_01_driver_table[] = {

{"libusb_01", -1, MXI_USB, MXR_INTERFACE,
			&mxi_libusb_01_record_function_list,
			NULL,
			&mxi_libusb_01_usb_function_list,
			&mxi_libusb_01_num_record_fields,
			&mxi_libusb_01_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"libusb-0.1",
	MX_VERSION,
	libusb_01_driver_table,
	NULL,
	NULL
};

