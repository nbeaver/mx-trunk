/*
 * Name:    u500.c
 *
 * Purpose: Module wrapper for the vendor-provided Win32 libraries
 *          for the Aerotech Unidex 500 motor controller.
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
#include "mx_motor.h"
#include "mx_digital_input.h"
#include "mx_variable.h"
#include "i_u500.h"
#include "i_u500_rs232.h"
#include "d_u500.h"
#include "d_u500_status.h"
#include "v_u500.h"

MX_DRIVER u500_driver_table[] = {

{"u500",           -1,      MXI_CONTROLLER,        MXR_INTERFACE,
				&mxi_u500_record_function_list,
				NULL,
				NULL,
				&mxi_u500_num_record_fields,
				&mxi_u500_rfield_def_ptr},

{"u500_rs232",     -1,      MXI_RS232,      MXR_INTERFACE,
				&mxi_u500_rs232_record_function_list,
				NULL,
				&mxi_u500_rs232_rs232_function_list,
				&mxi_u500_rs232_num_record_fields,
				&mxi_u500_rs232_rfield_def_ptr},

{"u500_motor",     -1,      MXC_MOTOR,          MXR_DEVICE,
				&mxd_u500_record_function_list,
				NULL,
				&mxd_u500_motor_function_list,
				&mxd_u500_num_record_fields,
				&mxd_u500_rfield_def_ptr},

{"u500_status",    -1,      MXC_DIGITAL_INPUT,  MXR_DEVICE,
				&mxd_u500_status_record_function_list,
				NULL,
				&mxd_u500_status_digital_input_function_list,
				&mxd_u500_status_num_record_fields,
				&mxd_u500_status_rfield_def_ptr},

{"u500_variable",  -1,      MXV_U500,   MXR_VARIABLE,
				&mxv_u500_variable_record_function_list,
				&mxv_u500_variable_variable_function_list,
				NULL,
				&mxv_u500_variable_num_record_fields,
				&mxv_u500_variable_rfield_def_ptr},

{"", 0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

MX_EXPORT
MX_MODULE __MX_MODULE__ = {
	"u500",
	MX_VERSION,
	u500_driver_table,
	NULL,
	NULL
};

