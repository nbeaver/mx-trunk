/*
 * Name:    d_wago750_modbus_aout.h
 *
 * Purpose: Header file for a version of the 'modbus_aoutput' driver
 *          modified to read the output of a Wago 750 series analog
 *          output via MODBUS.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_WAGO750_MODBUS_AOUT_H__
#define __D_WAGO750_MODBUS_AOUT_H__

#include "d_modbus_aio.h"

MX_API mx_status_type mxd_wago750_modbus_aout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_wago750_modbus_aout_read( MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_wago750_modbus_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_wago750_modbus_aout_analog_output_function_list;

extern long mxd_wago750_modbus_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_wago750_modbus_aout_rfield_def_ptr;

#endif /* __D_WAGO750_MODBUS_AOUT_H__ */

