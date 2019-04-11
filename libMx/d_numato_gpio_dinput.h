/*
 * Name:    d_numato_gpio_dinput.h
 *
 * Purpose: Header file for setting the input of Keithley 2400 multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NUMATO_GPIO_DINPUT_H__
#define __D_NUMATO_GPIO_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	MX_RECORD *controller_record;
} MX_NUMATO_GPIO_DINPUT;

MX_API mx_status_type mxd_numato_gpio_dinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_numato_gpio_dinput_open( MX_RECORD *record );
MX_API mx_status_type mxd_numato_gpio_dinput_close( MX_RECORD *record );

MX_API mx_status_type mxd_numato_gpio_dinput_write(MX_DIGITAL_INPUT *dinput);

extern MX_RECORD_FUNCTION_LIST mxd_numato_gpio_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
		mxd_numato_gpio_dinput_digital_input_function_list;

extern long mxd_numato_gpio_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_numato_gpio_dinput_rfield_def_ptr;

#define MXD_NUMATO_GPIO_DINPUT_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NUMATO_GPIO_DINPUT, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_NUMATO_GPIO_DINPUT_H__ */
