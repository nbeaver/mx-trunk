/*
 * Name:    d_numato_gpio_ainput.h
 *
 * Purpose: Header file for Numato Lab GPIO analog inputs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NUMATO_GPIO_AINPUT_H__
#define __D_NUMATO_GPIO_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *numato_gpio_record;
	long channel_number;
} MX_NUMATO_GPIO_AINPUT;

MX_API mx_status_type mxd_numato_gpio_ainput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_numato_gpio_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_numato_gpio_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
		mxd_numato_gpio_ainput_analog_input_function_list;

extern long mxd_numato_gpio_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_numato_gpio_ainput_rfield_def_ptr;

#define MXD_NUMATO_GPIO_AINPUT_STANDARD_FIELDS \
  {-1, -1, "numato_gpio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_NUMATO_GPIO_AINPUT, numato_gpio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUMATO_GPIO_AINPUT,channel_number),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_NUMATO_GPIO_AINPUT_H__ */
