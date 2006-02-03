/*
 * Name:    d_wago750_modbus_dout.h
 *
 * Purpose: Header file for a version of the 'modbus_doutput' driver
 *          modified to read the output of a Wago 750 series digital
 *          output via MODBUS.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_WAGO750_MODBUS_DOUT_H__
#define __D_WAGO750_MODBUS_DOUT_H__

#include "d_modbus_dio.h"

MX_API mx_status_type mxd_wago750_modbus_dout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_wago750_modbus_dout_read( MX_DIGITAL_OUTPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_wago750_modbus_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_wago750_modbus_dout_digital_output_function_list;

extern mx_length_type mxd_wago750_modbus_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_wago750_modbus_dout_rfield_def_ptr;

#endif /* __D_WAGO750_MODBUS_DOUT_H__ */

